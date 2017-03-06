#include "test.h"
#include <stdbool.h>

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

#define ACCESS_ONCE(x) (*(volatile typeof(x) *)&(x))
// #define barrier() __asm__ __volatile__("": : :"memory")
#define mb() asm volatile("mfence":::"memory")

#define POOL_ALLOC(t) malloc(sizeof(t))
#define POOL_FREE(t, p) free(p)

typedef struct per_thread_meta_t {
  node_t *auto_ebr_limbo_list[N_EPOCHS];
  uint32_t auto_ebr_entries;
  uint8_t auto_erb_in_critical;
  uint8_t local_epoch;
} per_thread_meta_t;

typedef struct auto_ebr_t {
  per_thread_meta_t *threads;
  uint32_t update_threadhold;
  uint8_t global_epoch;
  uint8_t global_epoch_lock;
} auto_ebr_t;

void free_node_later(node_t *p)
{
  per_thread_meta_t *meta =
    &this_thread.auto_ebr->threads[this_thread.thread_id];
  uint8_t local_epoch = meta->local_epoch;
  p->mr_next = meta->auto_ebr_limbo_list[local_epoch];
  meta->auto_ebr_limbo_list[local_epoch] = p;
}

static void clean_list(node_t **list)
{
  node_t *cur = *list;
  node_t *pre;
  while (cur) {
    pre = cur;
    cur = cur->mr_next;
    POOL_FREE(node_t, pre);
  }
  *list = NULL;
}

static void update_epoch(auto_ebr_t *auto_ebr)
{
  uint8_t old = 0;
  if (_atomic_cas(&auto_ebr->global_epoch_lock, &old, 1)) {
    for (int i = 0; i < MAX_THREADS; ++i) {
      if (auto_ebr->threads[i].auto_erb_in_critical
          && auto_ebr->threads[i].local_epoch != auto_ebr->global_epoch) {
        auto_ebr->global_epoch_lock = 0;
        return;
      }
    }
    auto_ebr->global_epoch = (auto_ebr->global_epoch + 1) % N_EPOCHS;
    _atomic_store(&auto_ebr->global_epoch_lock, 0);
  }
}

void critical_enter()
{
  per_thread_meta_t *meta =
    &this_thread.auto_ebr->threads[this_thread.thread_id];
  uint8_t local_epoch = meta->local_epoch;
  while (true) {
    meta->auto_erb_in_critical = 1;
    mb();
    uint8_t global_epoch = this_thread.auto_ebr->global_epoch;
    if (local_epoch != global_epoch) {
      clean_list(&meta->auto_ebr_limbo_list[global_epoch]);
      meta->local_epoch = global_epoch;
      meta->auto_ebr_entries = 0;
      return;
    }
    if (meta->auto_ebr_entries++ < this_thread.auto_ebr->update_threadhold) {
      return;
    }
    meta->auto_erb_in_critical = 0;
    meta->auto_ebr_entries = 0;
    update_epoch(this_thread.auto_ebr);
  }
}

void critical_exit()
{
  mb();
  this_thread.auto_ebr->threads[this_thread.thread_id].auto_erb_in_critical = 0;
}

auto_ebr_t *auto_ebr_init()
{
  auto_ebr_t *new = POOL_ALLOC(sizeof(auto_ebr_t));

  new->update_threadhold = 100;
  new->global_epoch = 1;
  new->global_epoch_lock = 0;
  new->threads = calloc(MAX_THREADS, sizeof(per_thread_meta_t));
  return new;
}

void mr_init()
{
  auto_ebr_t *auto_ebr = auto_ebr_init();
  for (int i = 0; i < MAX_THREADS; i++) {
    threads[i].auto_ebr = auto_ebr;
    threads[i].thread_id = (uint32_t) i;
  }
}

void mr_thread_exit() {}
void mr_reinitialize() {}
