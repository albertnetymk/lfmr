#define _XOPEN_SOURCE 800

#include "shared_object.h"
#include "mem/pool.h"
#include "sched/scheduler.h"
#include "actor/actor.h"
#include "ds/hash.h"
#include <assert.h>
#include <stdio.h>
#include <stdint.h>

typedef struct wrapper_t {
  void *p;
  struct wrapper_t *next;
} wrapper_t;

#define wrapper_pool_size 100
static __thread wrapper_t *wrapper_pool[wrapper_pool_size];
static __thread unsigned int wrapper_pool_counter;

typedef struct per_thread_t {
  wrapper_t *limbo_list[3];
  uint32_t entry;
  uint8_t in_critical;
  uint8_t local_epoch;
} per_thread_t;

#define void_assert(e) { (void)(e) ; assert(e); }

#define _atomic_sub(PTR, VAL) \
  (__atomic_fetch_sub(PTR, VAL, __ATOMIC_RELEASE))

#define _atomic_load_relaxed(PTR) \
  __atomic_load_n(PTR, __ATOMIC_RELAXED)

#define _atomic_store_relaxed(PTR, VAL) \
  __atomic_store_n(PTR, VAL, __ATOMIC_RELAXED)

static inline void gc_sendobject_shallow(pony_ctx_t *ctx, void *p)
{
  gc_sendobject(ctx, p, NULL);
}

static inline void gc_sendobject_shallow_done(pony_ctx_t *ctx)
{
  pony_send_done(ctx);
}

static inline void gc_recvobject_shallow(pony_ctx_t *ctx, void *p)
{
  gc_recvobject(ctx, p, NULL);
}

static inline void gc_recvobject_shallow_done(pony_ctx_t *ctx)
{
  pony_recv_done(ctx);
}

static inline void relax(void)
{
#if defined(PLATFORM_IS_X86) && !defined(PLATFORM_IS_VISUAL_STUDIO)
    asm volatile("pause" ::: "memory");
#endif
}

static size_t so_lockfree_inc_rc(void *p)
{
  if (!p) { return 0; }
  encore_passive_lf_so_t *f = (encore_passive_lf_so_t *)p;
  return _atomic_add(&f->rc, 1);
}

static size_t so_lockfree_dec_rc(void *p)
{
  if (!p) { return 0; }
  encore_passive_lf_so_t *f = (encore_passive_lf_so_t *)p;
  assert(_atomic_load(&f->rc) != 0);
  return _atomic_sub(&f->rc, 1);
}

extern __thread uint32_t so_thread_index;

#define barrier() asm volatile("": : :"memory")

static wrapper_t *new_wrapper()
{
  wrapper_t *new;
  if (wrapper_pool_counter > 0) {
    new = wrapper_pool[--wrapper_pool_counter];
  } else {
    new = POOL_ALLOC(wrapper_t);
  }
  return new;
}

static void free_wrapper(wrapper_t *d)
{
  if (wrapper_pool_counter == wrapper_pool_size) {
    wrapper_pool_counter = 0;
    for (unsigned int i = 0; i < wrapper_pool_size; ++i) {
      POOL_FREE(wrapper_t, wrapper_pool[i]);
    }
  }
  wrapper_pool[wrapper_pool_counter++] = d;
}

static void so_lockfree_delay_dec(so_gc_t *so_gc, void *p)
{
  if (!p) { return; }
  wrapper_t *w = new_wrapper();
  w->p = p;
  per_thread_t *thread = &so_gc->threads[so_thread_index];
  w->next = thread->limbo_list[thread->local_epoch];
  thread->limbo_list[thread->local_epoch] = w;
}

static void clean_list(pony_ctx_t *ctx, so_gc_t *so_gc, wrapper_t **list)
{
  wrapper_t *cur = *list;
  wrapper_t *pre;
  while (cur) {
    pre = cur;
    cur = cur->next;
    if (so_lockfree_dec_rc(pre->p) == 1) {
      void **f = (void**) pre->p;
      so_lockfree_delay_dec(so_gc, *(f+4));
      free(pre->p);

      // gc_recvobject_shallow(ctx, pre->p);
      // gc_recvobject_shallow_done(ctx);
    }
    free_wrapper(pre);
  }
  *list = NULL;
}
static void update_epoch(so_gc_t *so_gc)
{
  uint8_t old = 0;
  uint32_t n_threads = so_gc->n_threads;
  uint8_t global_epoch = so_gc->global_epoch;
  if (_atomic_cas(&so_gc->global_epoch_lock, &old, 1)) {
    for (uint32_t i = 0; i < n_threads; ++i) {
      per_thread_t *thread = &so_gc->threads[i];
      if (thread->in_critical && thread->local_epoch != global_epoch) {
        so_gc->global_epoch_lock = 0;
        return;
      }
    }
    so_gc->global_epoch = (so_gc->global_epoch + 1) % 3;
    _atomic_store(&so_gc->global_epoch_lock, 0);
  }
}

void so_lockfree_on_entry(pony_ctx_t *ctx, encore_so_t *this)
{
  so_gc_t *so_gc = &this->so_gc;
  per_thread_t *thread = &so_gc->threads[so_thread_index];
  uint8_t local_epoch = thread->local_epoch;
  while (true) {
    thread->in_critical = 1;
    barrier();
    uint8_t global_epoch = so_gc->global_epoch;
    if (local_epoch != global_epoch) {
      clean_list(ctx, so_gc, &thread->limbo_list[global_epoch]);
      thread->local_epoch = global_epoch;
      thread->entry = 0;
      return;
    }
    if (thread->entry++ < so_gc->entry_max) {
      return;
    }
    thread->in_critical = 0;
    thread->entry = 0;
    update_epoch(so_gc);
  }
}

void so_lockfree_on_exit(encore_so_t *this)
{
  barrier();
  this->so_gc.threads[so_thread_index].in_critical = 0;
}

extern uint32_t scheduler_cores();
encore_so_t *encore_create_so(pony_ctx_t *ctx, pony_type_t *type)
{
  encore_so_t *this = (encore_so_t*) encore_create(ctx, type);
  uint32_t n_threads = scheduler_cores();
  this->so_gc.threads = calloc(n_threads, sizeof(per_thread_t));
  this->so_gc.n_threads = n_threads;
  this->so_gc.entry_max = 100;
  this->so_gc.global_epoch = 1;
  return this;
}

void so_lockfree_register_final_cb(void *p, so_lockfree_final_cb_fn final_cb)
{
  encore_so_t *this = p;
  this->so_gc.final_cb = final_cb;
}

typedef pony_trace_fn non_subord_trace_fn;
typedef pony_trace_fn subord_trace_fn;

void so_lockfree_register_subord_trace_fn(void *p, subord_trace_fn trace)
{
  encore_so_t *this = p;
  this->subord_trace = trace;
}

void encore_so_finalizer(void *p)
{
  assert(p);
  encore_so_t *this = p;
  assert(this->so_gc.final_cb);
  pony_ctx_t *ctx = pony_ctx();
  this->so_gc.final_cb(ctx, this);
  free(this->so_gc.threads);
}

static void so_lockfree_pre_publish(void *p)
{
  if (!p) { return; }
  encore_passive_lf_so_t *f = (encore_passive_lf_so_t *)p;
  f->published = true;
  so_lockfree_inc_rc(f);
}

static void so_lockfree_unpre_publish(void *p)
{
  if (!p) { return; }
  encore_passive_lf_so_t *f = (encore_passive_lf_so_t *)p;
  assert(f->published);
  f->published = false;
  so_lockfree_dec_rc(f);
}

__attribute__((unused))
static void so_lockfree_publish(encore_so_t *this, void *p)
{
  if (!p) { return; }
  encore_passive_lf_so_t *f = (encore_passive_lf_so_t *)p;
  f->published = true;
#if 0
  if (!p) { return; }
  encore_passive_lf_so_t *f = (encore_passive_lf_so_t *)p;
  if (!so_lockfree_is_in_heap(f)) {
    so_lockfree_heap_push(this, f);
  }
#endif
}

static bool so_lockfree_is_published(void *p)
{
  assert(p);
  assert(p == UNFREEZE(p));
  encore_passive_lf_so_t *f = (encore_passive_lf_so_t *)p;
  return f->published;
}

void so_lockfree_spec_subord_field_apply(pony_ctx_t *ctx, encore_so_t *this,
    void *p)
{
  if (!p) { return; }
  so_lockfree_publish(this, p);
  gc_sendobject_shallow(ctx, p);
}

void so_lockfree_non_spec_subord_field_apply(pony_ctx_t *ctx, encore_so_t *this,
    void *p)
{
  if (!p) { return; }
  so_lockfree_publish(this, p);
  gc_sendobject_shallow(ctx, p);
}

void so_lockfree_subord_fields_apply_done(pony_ctx_t *ctx)
{
  gc_sendobject_shallow_done(ctx);
}

void so_lockfree_subord_field_final_apply(pony_ctx_t *ctx, void *p,
    non_subord_trace_fn fn)
{
  if (!p) { return; }
  assert(p == UNFREEZE(p));
  encore_passive_lf_so_t *f = (encore_passive_lf_so_t *)p;
  assert(f->published);
  size_t rc = _atomic_sub(&f->rc, 1);
  assert(rc > 0);
  if (rc == 1) {
#ifdef use_stw_mark_sweep
    // TODO the same as below
    encore_passive_lf_so_t *_f = f;
    _atomic_cas(&f->wrapper->p, &_f, NULL);
#else
#endif
    pony_gc_recv(ctx);
    pony_traceobject(ctx, f, fn);
    gc_handlestack(ctx);
  }
}

// TODO do we need non_subord_field_final_apply??

void so_lockfree_chain_final(pony_ctx_t *ctx, void *p, non_subord_trace_fn fn)
{
  if (!p) { return; }
  encore_passive_lf_so_t *f = (encore_passive_lf_so_t *)UNFREEZE(p);
  if (!f->published) { return; }
  size_t rc = _atomic_sub(&f->rc, 1);
  assert(rc > 0);
  if (rc == 1) {
#ifdef use_stw_mark_sweep
    // TODO currently, it's not necessary to nullify p, for mark-sweep could
    // collect wrapper anyway; however, it's put here so that mark-sweep is not
    // needed unless there's cycle in the container
    // TODO this cas could be dangerous if wrapper is already freed; might get
    // around using pool for single type of object
    encore_passive_lf_so_t *_f = f;
    _atomic_cas(&f->wrapper->p, &_f, NULL);
#else
#endif
    pony_gc_recv(ctx);
    pony_traceobject(ctx, f, fn);
    gc_handlestack(ctx);
  }
}

void so_lockfree_send(pony_ctx_t *ctx)
{
  void *p;
  while(ctx->lf_tmp_stack != NULL) {
    ctx->lf_tmp_stack = gcstack_pop(ctx->lf_tmp_stack, &p);
    gc_sendobject_shallow(ctx, p);
  }
  gc_sendobject_shallow_done(ctx);
}

void so_lockfree_unsend(pony_ctx_t *ctx)
{
  void *p;
  while(ctx->lf_tmp_stack != NULL) {
    ctx->lf_tmp_stack = gcstack_pop(ctx->lf_tmp_stack, &p);
  }
}

__attribute__((unused))
static void so_lockfree_recv(pony_ctx_t *ctx)
{
  void *p;
  while(ctx->lf_tmp_stack != NULL) {
    ctx->lf_tmp_stack = gcstack_pop(ctx->lf_tmp_stack, &p);
    gc_recvobject_shallow(ctx, p);
  }
  gc_recvobject_shallow_done(ctx);
}

void so_lockfree_set_trace_boundary(pony_ctx_t *ctx, void *p)
{
  ctx->boundary = p;
}

#define mannual_not_trace 1
bool _so_lockfree_cas_try_wrapper(pony_ctx_t *ctx, encore_so_t *this,
    void *X, void *Y, void *_Z, pony_trace_fn F)
{
  assert(X);
  bool ret;
  void *Z = UNFREEZE(_Z);

#if mannual_not_trace
#else
  pony_gc_collect_to_send(ctx);
  so_lockfree_set_trace_boundary(ctx, NULL);
  pony_traceobject(ctx, Z, F);
  pony_gc_collect_to_send_done(ctx);
#endif
  so_lockfree_pre_publish(Z);

  ret = _atomic_cas((void**)X, &Y, _Z);
  if (ret) {
    assert(Y == NULL);
    // so_lockfree_publish(this, Z);
#if mannual_not_trace
    gc_sendobject_shallow(ctx, Z);
    gc_sendobject_shallow_done(ctx);
#else
    so_lockfree_send(ctx);
#endif
  } else {
    so_lockfree_unpre_publish(Z);
    // so_lockfree_unsend(ctx);
  }

  return ret;
}

void* _so_lockfree_cas_extract_wrapper(void *_address, pony_trace_fn F)
{
  void **address = (void **)_address;
  void *tmp = *(void **)address;
  *address = NULL;
  // pony_ctx_t *ctx = pony_ctx();
  // pony_gc_recv(ctx);
  // pony_traceobject(ctx, tmp, F);
  // pony_recv_done(ctx);
  return tmp;
}

bool _so_lockfree_cas_link_wrapper(pony_ctx_t *ctx, encore_so_t *this,
    void *X, void *Y, void *Z, pony_trace_fn F)
{
  assert(X);
  bool ret;

  // ifdef mannual_not_trace for not tracing inside Z
#ifdef mannual_not_trace
#else
  pony_gc_collect_to_send(ctx);
  so_lockfree_set_trace_boundary(ctx, Y);
  pony_traceobject(ctx, Z, F);
  pony_gc_collect_to_send_done(ctx);
#endif

  so_lockfree_pre_publish(Z);

  ret = _atomic_cas((void**)X, &Y, Z);
  if (ret) {
    // so_lockfree_publish(this, Z);

#if mannual_not_trace
    gc_sendobject_shallow(ctx, Z);
    gc_sendobject_shallow_done(ctx);
#else
    so_lockfree_send(ctx);
#endif

    so_lockfree_delay_dec(&this->so_gc, Y);
  } else {
    so_lockfree_unpre_publish(Z);
    // so_lockfree_unsend(ctx);
  }

  return ret;
}

bool _so_lockfree_cas_unlink_wrapper(pony_ctx_t *ctx, encore_so_t *this,
    void *X, void *Y, void *Z, pony_trace_fn F)
{
  assert(X);
  so_lockfree_inc_rc(Z);
  bool ret = _atomic_cas((void**)X, &Y, Z);
  if (ret) {
#ifdef mannual_not_trace
#else
    pony_gc_collect_to_recv(ctx);
    so_lockfree_set_trace_boundary(ctx, Z);
    pony_traceobject(ctx, Y, F);
    pony_gc_collect_to_recv_done(ctx);

    so_lockfree_recv(ctx);

    gc_recvobject_shallow(ctx, Y);
    gc_recvobject_shallow_done(ctx);

    gc_sendobject_shallow(ctx, Y);
    gc_sendobject_shallow_done(ctx);
#endif

    so_lockfree_delay_dec(&this->so_gc, Y);
  } else {
    so_lockfree_delay_dec(&this->so_gc, Z);
  }
  return ret;
}

bool _so_lockfree_cas_swap_wrapper(pony_ctx_t *ctx, encore_so_t *this,
    void *X, void *Y, void *Z, pony_trace_fn F)
{
  assert(X);
  bool ret;

#ifdef mannual_not_trace
#else
  pony_gc_collect_to_send(ctx);
  so_lockfree_set_trace_boundary(ctx, NULL);
  pony_traceobject(ctx, Z, F);
  pony_gc_collect_to_send_done(ctx);
#endif

  so_lockfree_pre_publish(Z);

  ret = _atomic_cas((void**)X, &Y, Z);
  if (ret) {
    // so_lockfree_publish(this, Z);
#ifdef mannual_not_trace
    gc_sendobject_shallow(ctx, Z);
    gc_sendobject_shallow_done(ctx);
#else
    so_lockfree_send(ctx);

    pony_gc_collect_to_recv(ctx);
    so_lockfree_set_trace_boundary(ctx, NULL);
    pony_traceobject(ctx, Y, F);
    pony_gc_collect_to_recv_done(ctx);

    so_lockfree_recv(ctx);

    gc_sendobject_shallow(ctx, Y);
    gc_sendobject_shallow_done(ctx);
#endif

    so_lockfree_delay_dec(&this->so_gc, Y);
  } else {
    so_lockfree_unpre_publish(Z);
    // so_lockfree_unsend(ctx);
  }

  return ret;
}

// TODO I can probably unite the two
void so_lockfree_assign_spec_wrapper(pony_ctx_t *ctx, encore_so_t *this,
    void *lhs, void *rhs, pony_trace_fn F)
{
  assert(rhs == UNFREEZE(rhs));
  assert(lhs == UNFREEZE(lhs));
  so_lockfree_inc_rc(rhs);
  if (lhs && so_lockfree_is_published(lhs)) {
      so_lockfree_delay_dec(&this->so_gc, lhs);
  }
}

void _so_lockfree_assign_subord_wrapper(pony_ctx_t *ctx, encore_so_t *this,
    void *lhs, void *rhs)
{
  assert(rhs == UNFREEZE(rhs));
  assert(lhs == UNFREEZE(lhs));
  so_lockfree_inc_rc(rhs);
  if (lhs && so_lockfree_is_published(lhs)) {
      so_lockfree_delay_dec(&this->so_gc, lhs);
  }
}
