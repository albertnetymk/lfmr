#include "test.h"
#include "dw.h"
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <stdio.h>

#define void_assert(e) { (void)(e) ; assert(e); }

#define _atomic_sub(PTR, VAL) \
  (__atomic_fetch_sub(PTR, VAL, __ATOMIC_RELEASE))

#define _atomic_add(PTR, VAL) \
  (__atomic_fetch_add(PTR, VAL, __ATOMIC_RELEASE))

#define _atomic_load_relaxed(PTR) \
  __atomic_load_n(PTR, __ATOMIC_RELAXED)

#define _atomic_load(PTR) \
  __atomic_load_n(PTR, __ATOMIC_ACQUIRE)

#define _atomic_store(PTR, VAL) \
  __atomic_store_n(PTR, VAL, __ATOMIC_RELEASE)

#define _atomic_exchange(PTR, VAL) \
  __atomic_exchange_n(PTR, VAL, __ATOMIC_RELAXED)

#define _atomic_store_relaxed(PTR, VAL) \
  __atomic_store_n(PTR, VAL, __ATOMIC_RELAXED)

#define _atomic_cas(PTR, EXPP, VAL) \
  __atomic_compare_exchange_n(PTR, EXPP, VAL, 0, \
    __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE)

#define POOL_ALLOC(t) malloc(sizeof(t))
#define POOL_FREE(t, p) free(p)

typedef __int128_t dw_t;
typedef struct duration_t duration_t;

// typedef struct trace_address_list {
//   void *address;
//   struct trace_address_list *next;
// } trace_address_list;

typedef struct item_list_t {
  duration_t *duration;
  node_t *address;
} item_list_t;

typedef struct duration_spscq_t {
  duration_t *head;
  duration_t *tail;
} duration_spscq_t;

__attribute__ ((__aligned__(16)))
typedef struct dwcas_t {
  union {
    struct {
      uintptr_t aba;
      duration_t *current;
    };
    dw_t dw;
  };
} dwcas_t;

typedef struct {
  union {
    struct {
      uint32_t aba;
      uint32_t entry;
    };
    uint64_t dw;
  };
} aba_entry_t;
typedef struct {
  union {
    struct {
      uint32_t pending;
      uint32_t lock;
    };
    uint64_t dw;
  };
} pending_lock_t;

typedef struct dw_gc_t {
  aba_entry_t aba_entry;
  duration_t *current_d;
  duration_spscq_t duration_q;
  pending_lock_t pending_lock;
} dw_gc_t;

typedef struct duration_t {
  item_list_t *head;
  uint32_t entry;
  uint32_t exit;
  bool collectible;
  // may be removed for optimization
  bool closed;
  struct duration_t *next;
} duration_t;

static inline void relax(void)
{
#if defined(PLATFORM_IS_X86) && !defined(PLATFORM_IS_VISUAL_STUDIO)
    asm volatile("pause" ::: "memory");
#endif
}

#define duration_pool_size 100
static __thread duration_t *duration_pool[duration_pool_size];
static __thread unsigned int duration_pool_counter;

static duration_t *new_duration()
{
  duration_t *new;
  if (duration_pool_counter > 0) {
    new = duration_pool[--duration_pool_counter];
  } else {
    new = POOL_ALLOC(duration_t);
  }
  new->head = NULL;
  // entry is initialized on duration closing
  new->exit = 0;
  new->collectible = false;
  new->closed = true;
  new->next = NULL;
  return new;
}

static void free_duration(duration_t *d)
{
  if (duration_pool_counter == duration_pool_size) {
    duration_pool_counter = 0;
    for (unsigned int i = 0; i < duration_pool_size; ++i) {
      POOL_FREE(duration_t, duration_pool[i]);
    }
  }
  duration_pool[duration_pool_counter++] = d;
}

#define item_list_pool_size 100
static __thread item_list_t *item_list_pool[item_list_pool_size];
static __thread unsigned int item_list_pool_counter;

static item_list_t *new_item_list()
{
  item_list_t *new;
  if (item_list_pool_counter > 0) {
    new = item_list_pool[--item_list_pool_counter];
  } else {
    new = POOL_ALLOC(item_list_t);
  }
  return new;
}

static void free_item_list(item_list_t *d)
{
  if (item_list_pool_counter == item_list_pool_size) {
    item_list_pool_counter = 0;
    for (unsigned int i = 0; i < item_list_pool_size; ++i) {
      POOL_FREE(item_list_t, item_list_pool[i]);
    }
  }
  item_list_pool[item_list_pool_counter++] = d;
}

static void duration_spscq_init(duration_spscq_t *q)
{
  duration_t *dummy = POOL_ALLOC(duration_t);
  dummy->next = NULL;
  q->head = q->tail = dummy;
}

__attribute__((unused))
static void duration_spscq_destroy(duration_spscq_t *q)
{
  assert(_atomic_load(&q->head) == _atomic_load(&q->tail));
  POOL_FREE(duration_t, q->tail);
}

__attribute__((unused))
static void duration_spscq_push_atomic(duration_spscq_t *q, duration_t *d)
{
  duration_t *prev = _atomic_exchange(&q->head, d);
  _atomic_store(&prev->next, d);
}

__attribute__((unused))
static void duration_spscq_push(duration_spscq_t *q, duration_t *d)
{
  duration_t *prev = q->head;
  q->head = d;
  _atomic_store(&prev->next, d);
}

static duration_t* duration_spscq_pop(duration_spscq_t *q)
{
  duration_t *tail = q->tail;
  duration_t *next = _atomic_load(&tail->next);

  if (next == NULL) {
    return NULL;
  }

  free_duration(tail);
  q->tail = next;
  return next;
}

__attribute__((unused))
static duration_t* duration_spscq_peek(duration_spscq_t *q)
{
  duration_t *tail = q->tail;
  assert(tail);
  return tail->next;
}

static void clean_one(dw_gc_t *dw_gc, item_list_t *item)
{
  assert(item);
  if (item->address) {
    node_t *cur = item->address;
    node_t *pre;
    while (cur) {
      pre = cur->mr_next;
      POOL_FREE(node_t, cur);
      cur = pre;
    }
    item->address = NULL;
  }
  free_item_list(item);
}

static void collect(dw_gc_t *dw_gc)
{
  // multithread
  if (_atomic_add(&dw_gc->pending_lock.pending, 1) != 0) { return; }
  uint32_t lock = 0;
  if (!_atomic_cas(&dw_gc->pending_lock.lock, &lock, 1)) { return; }
  duration_t *d;
  pending_lock_t pending_lock = (pending_lock_t) {.lock = 1};
  pending_lock_t new_pending_lock = (pending_lock_t) {};
  if (_atomic_load(&dw_gc->pending_lock.pending) == 0
      && _atomic_cas(&dw_gc->pending_lock.dw, &pending_lock.dw,
        new_pending_lock.dw)) {
    return;
  }
  do {
    assert(_atomic_load(&dw_gc->pending_lock.pending) > 0);
    _atomic_sub(&dw_gc->pending_lock.pending, 1);
    while ((d = duration_spscq_peek(&dw_gc->duration_q))) {
      if (!_atomic_load(&d->collectible)) { break; }
      clean_one(dw_gc, d->head);
      duration_t *_d = duration_spscq_pop(&dw_gc->duration_q);
      void_assert(d == _d);
    }
    if (dw_gc->pending_lock.pending > 0) { continue; }
    pending_lock.pending = 0;
    if (_atomic_cas(&dw_gc->pending_lock.dw, &pending_lock.dw,
          new_pending_lock.dw)) {
        break;
    }
  } while (true);
}

static void set_collectible(dw_gc_t *dw_gc, duration_t *d)
{
  // multithread entry
  // called by any thread on exiting
  assert(_atomic_load(&d->closed));
  assert(!_atomic_load(&d->collectible));
  if (_atomic_add(&d->exit, 1) == d->entry - 1) {
    _atomic_store(&d->collectible, true);
    collect(dw_gc);
  }
}

void dw_lockfree_on_entry(dw_gc_t *dw_gc, item_list_t *item)
{
  aba_entry_t aba_entry, new_aba_entry;
  bool entered = false;
  duration_t *current_d = NULL;
  do {
    aba_entry.aba = _atomic_load(&dw_gc->aba_entry.aba);
    if (aba_entry.aba % 2 != 0) {
      relax();
      continue;
    }
    current_d = dw_gc->current_d;
    assert(current_d);
    new_aba_entry.aba = aba_entry.aba;
    aba_entry.entry = _atomic_load(&dw_gc->aba_entry.entry);
    do {
      new_aba_entry.entry = aba_entry.entry + 1;
      if (_atomic_cas(&dw_gc->aba_entry.dw, &aba_entry.dw, new_aba_entry.dw)) {
        entered = true;
        break;
      }
      if (aba_entry.aba != new_aba_entry.aba) {
        // duration changed
        break;
      }
    } while (true);
  } while (!entered);

  item->duration = current_d;
}

void dw_lockfree_on_exit(dw_gc_t *dw_gc, item_list_t *item)
{
  duration_t *current_d;
  duration_t *my_d = item->duration;
  assert(my_d);
  aba_entry_t aba_entry;
  do {
    aba_entry.aba = _atomic_load(&dw_gc->aba_entry.aba);
    if (aba_entry.aba % 2 != 0) {
      relax();
      continue;
    }
    uint32_t new_aba = aba_entry.aba + 1;
    assert(new_aba % 2 == 1);
    if (_atomic_cas(&dw_gc->aba_entry.aba, &aba_entry.aba, new_aba)) {
      current_d = dw_gc->current_d;
      assert(current_d);
      uint32_t entry = dw_gc->aba_entry.entry;
      dw_gc->aba_entry.entry = 0;
      if (current_d != my_d) {
        entry++;
      }
      current_d->closed = true;
      dw_gc->current_d = new_duration();
      duration_spscq_push(&dw_gc->duration_q, current_d);
      uint32_t old_aba = _atomic_add(&dw_gc->aba_entry.aba, 1);
      void_assert(old_aba % 2 == 1);
      current_d->head = item;
      break;
    }
  } while (true);

  if (current_d != my_d) {
    set_collectible(dw_gc, current_d);
  }
  set_collectible(dw_gc, my_d);
}

dw_gc_t *dw_init_gc()
{
  dw_gc_t *dw_gc = POOL_ALLOC(dw_gc_t);
  dw_gc->aba_entry.dw = 0;
  dw_gc->current_d = new_duration();
  dw_gc->pending_lock.dw = 0;
  duration_spscq_init(&dw_gc->duration_q);
  return dw_gc;
}

item_list_t *dw_item_list_new()
{
  return new_item_list();
}

void dw_item_list(item_list_t *item_list, node_t *p)
{
  p->mr_next = item_list->address;
  item_list->address = p;
}

void free_node_later(node_t *cur)
{
  dw_item_list(this_thread.item_list, cur);
}

void critical_enter()
{
  assert(this_thread.dw_gc);
  this_thread.item_list = dw_item_list_new();
  dw_lockfree_on_entry(this_thread.dw_gc, this_thread.item_list);
}

void critical_exit()
{
  assert(this_thread.dw_gc);
  assert(this_thread.item_list);
  dw_lockfree_on_exit(this_thread.dw_gc, this_thread.item_list);
}

void mr_init()
{
  dw_gc_t * dw_gc = dw_init_gc();
  for (int i = 0; i < MAX_THREADS; i++) {
    threads[i].dw_gc = dw_gc;
  }
}

void mr_thread_exit() {}
void mr_reinitialize() {}
