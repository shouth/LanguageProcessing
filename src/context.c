#include <stdlib.h>
#include <string.h>

#include "context.h"
#include "source.h"
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
  return fnv1a(FNV1A_SEED, x->ptr, x->len);
}

static void symbol_deleter(void *value)
{
  symbol_t *x = value;
  free((void *) x->ptr);
  free(x);
}

const symbol_t *symbol(context_t *ctx, const char *ptr, long len)
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

static int substs_comp(const void *lhs, const void *rhs)
{
  const substs_t *l = lhs;
  const substs_t *r = rhs;
  return l->count == r->count && !memcmp(l->types, r->types, l->count);
}

static unsigned long substs_hash(const void *value)
{
  const substs_t *x = value;
  return fnv1a(FNV1A_SEED, x->types, sizeof(type_t *) * x->count);
}

const substs_t *substs(context_t *ctx, const type_t **types, long count)
{
  substs_t key;
  key.types = types;
  key.count = count;

  {
    const hash_entry_t *entry = hash_find(ctx->substs_interner, &key);
    if (!entry) {
      substs_t *substs = xmalloc(sizeof(substs_t));
      memcpy(substs, &key, sizeof(substs_t));
      entry = hash_insert_unchecked(ctx->substs_interner, substs, NULL);
    }
    return entry->key;
  }
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
    const type_procedure_t *lproc = lhs;
    const type_procedure_t *rproc = rhs;
    return lproc->params == rproc->params;
  }
  }
  unreachable();
}

static unsigned long type_hash(const void *value)
{
  const type_t *x    = value;
  unsigned long hash = fnv1a(FNV1A_SEED, &x->kind, sizeof(type_kind_t));
  switch (x->kind) {
  case TYPE_ARRAY: {
    const type_array_t *arr = value;
    hash                    = fnv1a(hash, &arr->base, sizeof(substs_t *));
    hash                    = fnv1a(hash, &arr->size, sizeof(long));
    break;
  }
  case TYPE_PROCEDURE: {
    const type_procedure_t *proc = value;
    hash                         = fnv1a(hash, &proc->params, sizeof(substs_t *));
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

static const type_t *type_make(context_t *ctx, type_t *type, type_kind_t kind)
{
  type->kind = kind;
  {
    const hash_entry_t *entry = hash_find(ctx->type_interner, type);
    if (!entry) {
      type_t *ntype = xmalloc(sizeof(type_t));
      memcpy(ntype, type, sizeof(type_t));
      entry = hash_insert_unchecked(ctx->type_interner, ntype, NULL);
    }
    return entry->key;
  }
}

const type_t *type_integer(context_t *ctx) { return ctx->type_integer; }
const type_t *type_boolean(context_t *ctx) { return ctx->type_boolean; }
const type_t *type_char(context_t *ctx) { return ctx->type_char; }
const type_t *type_string(context_t *ctx) { return ctx->type_string; }
const type_t *type_program(context_t *ctx) { return ctx->type_program; }

const type_t *type_array(context_t *ctx, const substs_t *base, long size)
{
  type_array_t type;
  type.base = base;
  type.size = size;
  return type_make(ctx, (type_t *) &type, TYPE_ARRAY);
}

const type_t *type_procedure(context_t *ctx, const substs_t *params)
{
  type_procedure_t type;
  type.params = params;
  return type_make(ctx, (type_t *) &type, TYPE_PROCEDURE);
}

context_t *ctx_init(context_t *ctx)
{
  ctx->symbol_interner = hash_new(&symbol_comp, &symbol_hash);
  ctx->substs_interner = hash_new(&substs_comp, &substs_hash);
  ctx->type_interner   = hash_new(&type_comp, &type_hash);

  {
    type_t type;
    ctx->type_boolean = type_make(ctx, &type, TYPE_BOOLEAN);
    ctx->type_char    = type_make(ctx, &type, TYPE_CHAR);
    ctx->type_integer = type_make(ctx, &type, TYPE_INTEGER);
    ctx->type_string  = type_make(ctx, &type, TYPE_STRING);
    ctx->type_program = type_make(ctx, &type, TYPE_PROGRAM);
  }
  return ctx;
}

void ctx_deinit(context_t *ctx)
{
  hash_delete(ctx->symbol_interner, symbol_deleter, NULL);
  hash_delete(ctx->substs_interner, free, NULL);
  hash_delete(ctx->type_interner, free, NULL);
}
