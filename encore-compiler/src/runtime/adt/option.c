#include "option.h"

pony_type_t option_type =
  {
    .id = ID_OPTION,
    .size = sizeof(struct option_t),
    .trace = option_trace,
  };

option_t DEFAULT_NOTHING = {.tag = NOTHING};

void option_trace(pony_ctx_t* ctx, void *p){
  struct option_t *option = p;
  if(option->tag == DEFAULT_NOTHING.tag){
    pony_traceobject(ctx, option, NULL);
  } else {
    encore_arg_t val = option->val;
    if(option->type == ENCORE_ACTIVE){
      pony_traceactor(ctx, val.p);
    }else if (option->type != ENCORE_PRIMITIVE){
      pony_traceobject(ctx, val.p, option->type->trace);
    }
  }
}

option_t *option_mk(pony_ctx_t* ctx, option_tag tag, encore_arg_t arg, pony_type_t* type){
  ctx = pony_ctx();
  option_t *o = encore_alloc(ctx, sizeof(option_t));
  *o = (option_t) {.tag = tag, .val = arg, .type = type};
  return o;
}
