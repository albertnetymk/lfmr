#ifndef SHARED_OBJECT_H_L6JOK8YX
#define SHARED_OBJECT_H_L6JOK8YX

#include "encore.h"

typedef void (*so_lockfree_final_cb_fn) (pony_ctx_t *ctx, void *p);

typedef struct so_lockfree_subord_wrapper_t so_lockfree_subord_wrapper_t;

typedef struct encore_passive_lf_so_t {
  pony_type_t *t;
  struct encore_passive_lf_so_t *next;
  uint32_t rc;
  bool published;
} encore_passive_lf_so_t;

typedef struct so_lockfree_padding {
  char data[sizeof(encore_passive_lf_so_t) - sizeof(void*)];
} so_lockfree_padding;

typedef struct per_thread_t per_thread_t;

typedef struct so_gc_t {
  so_lockfree_final_cb_fn final_cb;
  per_thread_t *threads;
  uint32_t entry_max;
  uint32_t n_threads;
  uint8_t global_epoch;
  uint8_t global_epoch_lock;
} so_gc_t;

typedef struct encore_so_t
{
  pony_actor_pad_t pad;
  // Everything else that goes into an encore_actor that's not part of PonyRT
  bool resume;
  int await_counter;
  int suspend_counter;
  pthread_mutex_t *lock;
#ifndef LAZY_IMPL
  ucontext_t uctx;
  ucontext_t home_uctx;
  volatile bool run_to_completion;
  stack_page *page;
#else
  ucontext_t *saved;
#endif
  pony_type_t *_enc__self_type;
  so_gc_t so_gc;
  pony_trace_fn subord_trace;
  uint32_t gc_mark;
} encore_so_t;

#define FREEZE(field) ((void*)(((uintptr_t)field) | 1UL))
#define UNFREEZE(field) ((void*)(((uintptr_t)field) & ~1UL))

#define _SO_LOCKFREE_CAS_TRY_WRAPPER(X, Y, Z, F) \
  _so_lockfree_cas_try_wrapper(_ctx, (void*)_this, X, Y, Z, F)

#define _SO_LOCKFREE_CAS_EXTRACT_WRAPPER(X, F) \
  _so_lockfree_cas_extract_wrapper(&X, F)

#define _SO_LOCKFREE_CAS_LINK_WRAPPER(X, Y, Z, F) \
  _so_lockfree_cas_link_wrapper(_ctx, (void*)_this, X, Y, Z, F)

#define _SO_LOCKFREE_CAS_UNLINK_WRAPPER(X, Y, Z, F) \
  _so_lockfree_cas_unlink_wrapper(_ctx, (void*)_this, X, Y, Z, F)

#define _SO_LOCKFREE_CAS_SWAP_WRAPPER(X, Y, Z, F) \
  _so_lockfree_cas_swap_wrapper(_ctx, (void*)_this, X, Y, Z, F)

#define _SO_LOCKFREE_ASSIGN_SPEC_WRAPPER(LHS, RHS, F) \
  so_lockfree_assign_spec_wrapper(_ctx, (void*)_this, LHS, RHS, F)

#define _SO_LOCKFREE_ASSIGN_SUBORD_WRAPPER(LHS, RHS) \
  _so_lockfree_assign_subord_wrapper(_ctx, (void*)_this, LHS, RHS)

typedef struct to_trace_t to_trace_t;

encore_so_t *encore_create_so(pony_ctx_t *ctx, pony_type_t *type);
void so_lockfree_register_final_cb(void *p, so_lockfree_final_cb_fn final_cb);
void so_lockfree_register_subord_trace_fn(void *p, pony_trace_fn trace);
void so_lockfree_spec_subord_field_apply(pony_ctx_t *ctx, encore_so_t *this,
    void *p);
void so_lockfree_non_spec_subord_field_apply(pony_ctx_t *ctx, encore_so_t *this,
    void *p);
void so_lockfree_subord_fields_apply_done(pony_ctx_t *ctx);
void so_lockfree_subord_field_final_apply(pony_ctx_t *ctx, void *p,
    pony_trace_fn fn);
void so_lockfree_on_entry(pony_ctx_t *ctx, encore_so_t *this);
void so_lockfree_on_exit(encore_so_t *this);
void encore_so_finalizer(void *p);
void pony_gc_collect_to_send(pony_ctx_t* ctx);
void pony_gc_collect_to_send_done(pony_ctx_t *ctx);
void pony_gc_collect_to_recv(pony_ctx_t* ctx);
void pony_gc_collect_to_recv_done(pony_ctx_t *ctx);
void so_lockfree_send(pony_ctx_t *ctx);
void so_lockfree_unsend(pony_ctx_t *ctx);
void so_lockfree_set_trace_boundary(pony_ctx_t *ctx, void *p);
void so_lockfree_chain_final(pony_ctx_t *ctx, void *p, pony_trace_fn fn);
bool _so_lockfree_cas_try_wrapper(pony_ctx_t *ctx, encore_so_t *this,
    void *X, void *Y, void *_Z, pony_trace_fn F);
void* _so_lockfree_cas_extract_wrapper(void *_address, pony_trace_fn F);
bool _so_lockfree_cas_link_wrapper(pony_ctx_t *ctx, encore_so_t *this,
    void *X, void *Y, void *_Z, pony_trace_fn F);
bool _so_lockfree_cas_unlink_wrapper(pony_ctx_t *ctx, encore_so_t *this,
    void *X, void *Y, void *Z, pony_trace_fn F);
bool _so_lockfree_cas_swap_wrapper(pony_ctx_t *ctx, encore_so_t *this,
    void *X, void *Y, void *Z, pony_trace_fn F);
void so_lockfree_assign_spec_wrapper(pony_ctx_t *ctx, encore_so_t *this,
    void *lhs, void *rhs, pony_trace_fn F);
void _so_lockfree_assign_subord_wrapper(pony_ctx_t *ctx, encore_so_t *this,
    void *lhs, void *rhs);
void so_lockfree_non_spec_field_apply(void *p);
#endif /* end of include guard: SHARED_OBJECT_H_L6JOK8YX */
