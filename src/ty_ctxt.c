/* SPDX-License-Identifier: Apache-2.0 */

#include <stddef.h>

#include "ty_ctxt.h"
#include "util.h"

struct TyCtxt {
  HashSet(Ty *) interner;
  HashMap(const RawSyntaxNode *, const Ty *) type;
};

static void ty_free(Ty *ty)
{
  if (ty) {
    switch (ty->kind) {
    case TY_ERROR:
    case TY_INTEGER:
    case TY_BOOLEAN:
    case TY_CHAR:
    case TY_STRING:
      /* do nothing. allocated statically */
      break;

    case TY_ARRAY:
      free(ty);
      break;

    case TY_PROC: {
      ProcTy *proc = (ProcTy *) ty;
      slice_free(&proc->params);
      free(ty);
      break;
    }
    }
  }
}

const Ty *ty_error(void)
{
  static const Ty ty = { TY_ERROR };
  return &ty;
}

const Ty *ty_integer(void)
{
  static const Ty ty = { TY_INTEGER };
  return &ty;
}

const Ty *ty_boolean(void)
{
  static const Ty ty = { TY_BOOLEAN };
  return &ty;
}

const Ty *ty_char(void)
{
  static const Ty ty = { TY_CHAR };
  return &ty;
}

const Ty *ty_string(void)
{
  static const Ty ty = { TY_STRING };
  return &ty;
}

static Ty *ty_intern(TyCtxt *ctxt, Ty *ty)
{
  HashMapEntry entry;
  if (!hashmap_entry(&ctxt->interner, &ty, &entry)) {
    hashmap_occupy(&ctxt->interner, &entry, &ty);
    return ty;
  } else {
    ty_free(ty);
    return *hashmap_key(&ctxt->interner, &entry);
  }
}

const Ty *ty_array(TyCtxt *ctxt, const Ty *base, unsigned long size)
{
  ArrayTy *array   = xmalloc(sizeof(*array));
  array->type.kind = TY_ARRAY;
  array->base      = base;
  array->size      = size;
  return ty_intern(ctxt, &array->type);
}

const Ty *ty_proc(TyCtxt *ctxt, const Ty **params, unsigned long param_count)
{
  ProcTy *proc    = xmalloc(sizeof(*proc));
  proc->type.kind = TY_PROC;
  slice_alloc(&proc->params, param_count);
  memcpy(proc->params.ptr, params, sizeof(*params) * param_count);
  return ty_intern(ctxt, &proc->type);
}

static void do_ty_hash(Hash *hash, const Ty *ty)
{
  hash_fnv1a(hash, &ty->kind, sizeof(ty->kind));
  switch (ty->kind) {
  case TY_ARRAY: {
    const ArrayTy *array = (const ArrayTy *) ty;
    do_ty_hash(hash, array->base);
    hash_fnv1a(hash, &array->size, sizeof(array->size));
    break;
  }

  case TY_PROC: {
    unsigned long i;
    const ProcTy *proc = (const ProcTy *) ty;
    for (i = 0; i < proc->params.count; i++) {
      do_ty_hash(hash, proc->params.ptr[i]);
    }
    break;
  }

  default:
    /* do nothing */
    break;
  }
}

static Hash ty_hash(const void *value)
{
  Hash hash = hash_fnv1a(NULL, NULL, 0);
  do_ty_hash(&hash, (const Ty *) value);
  return hash;
}

static int ty_eq(const void *lhs, const void *rhs)
{
  const Ty *l = (const Ty *) lhs;
  const Ty *r = (const Ty *) rhs;

  if (l->kind != r->kind) {
    return 0;
  }

  switch (l->kind) {
  case TY_ARRAY: {
    const ArrayTy *larray = (const ArrayTy *) l;
    const ArrayTy *rarray = (const ArrayTy *) r;
    return larray->size == rarray->size && ty_eq(larray->base, rarray->base);
  }

  case TY_PROC: {
    unsigned long i;
    const ProcTy *lproc = (const ProcTy *) l;
    const ProcTy *rproc = (const ProcTy *) r;
    if (lproc->params.count != rproc->params.count) {
      return 0;
    }
    for (i = 0; i < lproc->params.count; i++) {
      if (!ty_eq(lproc->params.ptr[i], rproc->params.ptr[i])) {
        return 0;
      }
    }
    return 1;
  }

  default:
    return 1;
  }
}

static Hash raw_syntax_hash(const void *ptr)
{
  const RawSyntaxNode **node = (const RawSyntaxNode **) ptr;
  return hash_fnv1a(NULL, node, sizeof(*node));
}

static int raw_syntax_eq(const void *lhs, const void *rhs)
{
  const RawSyntaxNode **lnode = (const RawSyntaxNode **) lhs;
  const RawSyntaxNode **rnode = (const RawSyntaxNode **) rhs;
  return *lnode == *rnode;
}

TyCtxt *ty_ctxt_alloc(void)
{
  TyCtxt *ctxt = xmalloc(sizeof(*ctxt));
  hashmap_alloc(&ctxt->interner, &ty_hash, &ty_eq);
  hashmap_alloc(&ctxt->type, &raw_syntax_hash, &raw_syntax_eq);
  return ctxt;
}

void ty_ctxt_free(TyCtxt *ctxt)
{
  HashMapEntry entry;
  hashmap_entry(&ctxt->interner, NULL, &entry);
  while (hashmap_next(&ctxt->interner, &entry)) {
    ty_free(*hashmap_key(&ctxt->interner, &entry));
  }

  hashmap_free(&ctxt->interner);
  hashmap_free(&ctxt->type);
  free(ctxt);
}

void ty_ctxt_set(TyCtxt *ctxt, const RawSyntaxNode *node, const Ty *ty)
{
  HashMapEntry entry;
  hashmap_entry(&ctxt->type, &node, &entry);
  hashmap_occupy(&ctxt->type, &entry, &node);
  *hashmap_value(&ctxt->type, &entry) = ty;
}

const Ty *ty_ctxt_get(TyCtxt *ctxt, const RawSyntaxNode *node)
{
  HashMapEntry entry;
  if (hashmap_entry(&ctxt->type, &node, &entry)) {
    return *hashmap_value(&ctxt->type, &entry);
  } else {
    return NULL;
  }
}
