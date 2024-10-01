/* SPDX-License-Identifier: Apache-2.0 */

#include <stddef.h>

#include "mppl_ty_ctxt.h"
#include "util.h"

struct MpplTyCtxt {
  HashSet(MpplTy *) interner;
  HashMap(const RawSyntaxNode *, const MpplTy *) type;
};

static void ty_free(MpplTy *ty)
{
  if (ty) {
    if (ty->kind == MPPL_TY_PROC) {
      MpplProcTy *proc = (MpplProcTy *) ty;
      slice_free(&proc->params);
    }
    free(ty);
  }
}

const MpplTy *mppl_ty_integer(void)
{
  static const MpplTy ty = { MPPL_TY_INTEGER };
  return &ty;
}

const MpplTy *mppl_ty_boolean(void)
{
  static const MpplTy ty = { MPPL_TY_BOOLEAN };
  return &ty;
}

const MpplTy *mppl_ty_char(void)
{
  static const MpplTy ty = { MPPL_TY_CHAR };
  return &ty;
}

const MpplTy *mppl_ty_string(void)
{
  static const MpplTy ty = { MPPL_TY_STRING };
  return &ty;
}

static MpplTy *ty_intern(MpplTyCtxt *ctxt, MpplTy *ty)
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

const MpplTy *mppl_ty_array(MpplTyCtxt *ctxt, const MpplTy *base)
{
  MpplArrayTy *array = xmalloc(sizeof(*array));
  array->type.kind   = MPPL_TY_ARRAY;
  array->base        = base;
  array->size        = 0;
  return ty_intern(ctxt, &array->type);
}

const MpplTy *mppl_ty_proc(MpplTyCtxt *ctxt, const MpplTy *params, unsigned long param_count)
{
  MpplProcTy *proc = xmalloc(sizeof(*proc));
  proc->type.kind  = MPPL_TY_PROC;
  slice_alloc(&proc->params, param_count);
  memcpy(proc->params.ptr, params, sizeof(*params) * param_count);
  return ty_intern(ctxt, &proc->type);
}

static void do_ty_hash(Hash *hash, const MpplTy *ty)
{
  hash_fnv1a(hash, &ty->kind, sizeof(ty->kind));
  switch (ty->kind) {
  case MPPL_TY_ARRAY: {
    const MpplArrayTy *array = (const MpplArrayTy *) ty;
    do_ty_hash(hash, array->base);
    hash_fnv1a(hash, &array->size, sizeof(array->size));
    break;
  }

  case MPPL_TY_PROC: {
    unsigned long     i;
    const MpplProcTy *proc = (const MpplProcTy *) ty;
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
  do_ty_hash(&hash, (const MpplTy *) value);
  return hash;
}

static int ty_eq(const void *lhs, const void *rhs)
{
  const MpplTy *l = (const MpplTy *) lhs;
  const MpplTy *r = (const MpplTy *) rhs;

  if (l->kind != r->kind) {
    return 0;
  }

  switch (l->kind) {
  case MPPL_TY_ARRAY: {
    const MpplArrayTy *larray = (const MpplArrayTy *) l;
    const MpplArrayTy *rarray = (const MpplArrayTy *) r;
    return larray->size == rarray->size && ty_eq(larray->base, rarray->base);
  }

  case MPPL_TY_PROC: {
    unsigned long     i;
    const MpplProcTy *lproc = (const MpplProcTy *) l;
    const MpplProcTy *rproc = (const MpplProcTy *) r;
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

static Hash ptr_hash(const void *ptr)
{
  return hash_fnv1a(NULL, ptr, sizeof(ptr));
}

static int ptr_eq(const void *lhs, const void *rhs)
{
  return lhs == rhs;
}

MpplTyCtxt *mppl_ty_ctxt_alloc(void)
{
  MpplTyCtxt *ctxt = xmalloc(sizeof(*ctxt));
  hashmap_alloc(&ctxt->interner, &ty_hash, &ty_eq);
  hashmap_alloc(&ctxt->type, &ptr_hash, &ptr_eq);
  return ctxt;
}

void mppl_ty_ctxt_free(MpplTyCtxt *ctxt)
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

const MpplTy **mppl_ty_ctxt_type_of(MpplTyCtxt *ctxt, const RawSyntaxNode *node)
{
  HashMapEntry entry;
  if (!hashmap_entry(&ctxt->type, node, &entry)) {
    hashmap_occupy(&ctxt->type, &entry, &node);
    *hashmap_value(&ctxt->type, &entry) = NULL;
  }
  return hashmap_value(&ctxt->type, &entry);
}
