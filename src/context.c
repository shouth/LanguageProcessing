#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ast.h"
#include "context.h"
#include "mpplc.h"
#include "source.h"
#include "types.h"
#include "utility.h"

static int symbol_comp(const void *lhs, const void *rhs)
{
  const symbol_t *l = lhs;
  const symbol_t *r = rhs;
  return l->len == r->len && !strncmp(l->ptr, r->ptr, l->len);
}

static unsigned long symbol_hash(const void *value)
{
  const symbol_t *x = value;
  return fnv1a(FNV1A_INIT, x->ptr, x->len);
}

static void symbol_deleter(void *value)
{
  if (value) {
    symbol_t *x = value;
    free((void *) x->ptr);
  }
  free(value);
}

const symbol_t *ctx_mk_symbol(context_t *ctx, const char *ptr, long len)
{
  symbol_t key;
  key.ptr = ptr;
  key.len = len;

  {
    const hash_entry_t *entry = hash_find(ctx->symbol_interner, &key);
    if (!entry) {
      symbol_t *symbol = xmalloc(sizeof(symbol_t));
      char     *str    = xmalloc(len + 1);
      strncpy(str, ptr, len);
      str[len]    = '\0';
      symbol->ptr = str;
      symbol->len = len;
      entry       = hash_insert_unchecked(ctx->symbol_interner, symbol, NULL);
    }
    return entry->key;
  }
}

subst_t *ctx_loan_subst(context_t *ctx, const type_t *type)
{
  subst_t *subst = ctx->subst_loan;
  if (ctx->subst_loan) {
    ctx->subst_loan = subst->next;
  } else {
    subst = xmalloc(sizeof(subst_t));
  }
  subst->type = type;
  subst->next = NULL;
  return subst;
}

static int type_comp(const void *lhs, const void *rhs)
{
  const type_t *l = lhs;
  const type_t *r = rhs;

  if (l->kind != r->kind) {
    return 0;
  }

  switch (l->kind) {
  case TYPE_INTEGER:
  case TYPE_BOOLEAN:
  case TYPE_CHAR:
  case TYPE_STRING:
  case TYPE_PROGRAM: {
    return 1;
  }
  case TYPE_ARRAY: {
    const type_array_t *larr = lhs;
    const type_array_t *rarr = rhs;
    return larr->base == rarr->base && larr->size == rarr->size;
  }
  case TYPE_PROCEDURE: {
    const type_procedure_t *lproc   = lhs;
    const type_procedure_t *rproc   = rhs;
    const subst_t          *lparams = lproc->params;
    const subst_t          *rparams = rproc->params;
    for (; lparams && rparams; lparams = lparams->next, rparams = rparams->next) {
      if (lparams->type != rparams->type) {
        return 0;
      }
    }
    return !lparams && !rparams;
  }
  }
  unreachable();
}

static unsigned long type_hash(const void *value)
{
  const type_t *x    = value;
  unsigned long hash = fnv1a(FNV1A_INIT, &x->kind, sizeof(type_kind_t));
  switch (x->kind) {
  case TYPE_ARRAY: {
    const type_array_t *arr = value;

    hash = fnv1a(hash, &arr->base->type, sizeof(type_t *));
    hash = fnv1a(hash, &arr->size, sizeof(long));
    break;
  }
  case TYPE_PROCEDURE: {
    const type_procedure_t *proc  = value;
    const subst_t          *subst = proc->params;

    for (; subst; subst = subst->next) {
      hash = fnv1a(hash, &subst->type, sizeof(type_t *));
    }
    break;
  }
  case TYPE_BOOLEAN:
  case TYPE_CHAR:
  case TYPE_INTEGER:
  case TYPE_STRING:
  case TYPE_PROGRAM:
    /* do nothing */
    break;
  }
  return hash;
}

static void delete_subst(subst_t *subst)
{
  while (subst) {
    subst_t *next = subst->next;
    free(subst);
    subst = next;
  }
}

static void type_deleter(void *value)
{
  if (value) {
    type_t *type = value;
    switch (type->kind) {
    case TYPE_ARRAY: {
      type_array_t *array = value;
      delete_subst(array->base);
      break;
    }
    case TYPE_PROCEDURE: {
      type_procedure_t *proc = value;
      delete_subst(proc->params);
      break;
    }
    default:
      /* do nothing */
      break;
    }
  }
  free(value);
}

static void cache_subst(context_t *ctx, subst_t *subst)
{
  while (subst) {
    subst_t *next   = subst->next;
    subst->next     = ctx->subst_loan;
    ctx->subst_loan = subst;
    subst           = next;
  }
}

static const type_t *mk_type(context_t *ctx, type_t *type, type_kind_t kind)
{
  type->kind = kind;
  {
    const hash_entry_t *entry = hash_find(ctx->type_interner, type);
    if (!entry) {
      type_t *ntype = xmalloc(sizeof(type_t));
      memcpy(ntype, type, sizeof(type_t));
      entry = hash_insert_unchecked(ctx->type_interner, ntype, NULL);
    } else {
      switch (kind) {
      case TYPE_ARRAY: {
        type_array_t *array = (type_array_t *) type;
        cache_subst(ctx, array->base);
        break;
      }
      case TYPE_PROCEDURE: {
        type_procedure_t *proc = (type_procedure_t *) type;
        cache_subst(ctx, proc->params);
        break;
      }
      default:
        /* do nothing */
        break;
      }
    }
    return entry->key;
  }
}

const type_t *ctx_mk_type_integer(context_t *ctx) { return ctx->type_integer; }
const type_t *ctx_mk_type_boolean(context_t *ctx) { return ctx->type_boolean; }
const type_t *ctx_mk_type_char(context_t *ctx) { return ctx->type_char; }
const type_t *ctx_mk_type_string(context_t *ctx) { return ctx->type_string; }
const type_t *ctx_mk_type_program(context_t *ctx) { return ctx->type_program; }

const type_t *ctx_mk_type_array(context_t *ctx, subst_t *base, long size)
{
  type_t        type;
  type_array_t *array = (type_array_t *) &type;
  array->base         = base;
  array->size         = size;
  return mk_type(ctx, &type, TYPE_ARRAY);
}

const type_t *ctx_mk_type_procedure(context_t *ctx, subst_t *params)
{
  type_t            type;
  type_procedure_t *proc = (type_procedure_t *) &type;
  proc->params           = params;
  return mk_type(ctx, &type, TYPE_PROCEDURE);
}

void mpplc_init(context_t *ctx, const char *in_name, const char *out_name)
{
  ctx->in_name = xmalloc(strlen(in_name) + 1);
  strcpy(ctx->in_name, in_name);

  if (out_name) {
    ctx->out_name = xmalloc(strlen(out_name) + 1);
    strcpy(ctx->out_name, out_name);
  } else {
    long in_name_len = strlen(in_name);
    ctx->out_name    = xmalloc(in_name_len + 1);
    sscanf(ctx->in_name, "%s.mpl", ctx->out_name);
    sprintf(ctx->out_name + in_name_len - 4, ".csl");
  }

  ctx->src        = src_new(in_name);
  ctx->ast        = NULL;
  ctx->defs       = NULL;
  ctx->resolution = NULL;

  ctx->symbol_interner = hash_new(&symbol_comp, &symbol_hash);
  ctx->type_interner   = hash_new(&type_comp, &type_hash);
  ctx->subst_loan      = NULL;

  {
    type_t type;
    ctx->type_boolean = mk_type(ctx, &type, TYPE_BOOLEAN);
    ctx->type_char    = mk_type(ctx, &type, TYPE_CHAR);
    ctx->type_integer = mk_type(ctx, &type, TYPE_INTEGER);
    ctx->type_string  = mk_type(ctx, &type, TYPE_STRING);
    ctx->type_program = mk_type(ctx, &type, TYPE_PROGRAM);
  }
}

static void delete_defs(def_t *def)
{
  while (def) {
    def_t *next = def->next;
    delete_defs(def->inner);
    free(def);
    def = next;
  }
}

void mpplc_deinit(context_t *ctx)
{
  if (ctx) {
    free(ctx->in_name);
    free(ctx->out_name);

    src_delete(ctx->src);
    ast_delete(ctx->ast);
    delete_defs(ctx->defs);
    hash_delete(ctx->resolution, NULL, NULL);

    hash_delete(ctx->symbol_interner, &symbol_deleter, NULL);
    hash_delete(ctx->type_interner, &type_deleter, NULL);
    delete_subst(ctx->subst_loan);
  }
}
