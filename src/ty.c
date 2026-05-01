/*
 * ty.c -- type system
 *
 * SPDX-FileCopyrightText: 2026 Shota Minami
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>

#include "ds.h"
#include "ty.h"

static hash_t ty_hash(void const *key)
{
  struct ty const *ty = *(struct ty const **) key;
  return ty->hash;
}

static int ty_eq(void const *left, void const *right)
{
  struct ty const *l = *(struct ty const **) left;
  struct ty const *r = *(struct ty const **) right;

  if (l->kind != r->kind) {
    return 0;
  } else if (l->kind == TY_ARRAY) {
    struct ty_array const *l = *(struct ty_array const **) left;
    struct ty_array const *r = *(struct ty_array const **) right;

    return l->size == r->size && l->elem == r->elem;
  } else if (l->kind == TY_PROC) {
    size_t i;
    struct ty_proc const *l = *(struct ty_proc const **) left;
    struct ty_proc const *r = *(struct ty_proc const **) right;

    if (l->params.count != r->params.count) {
      return 0;
    }
    for (i = 0; i < l->params.count; ++i) {
      if (l->params.data[i] != r->params.data[i]) {
        return 0;
      }
    }
    return 1;
  } else {
    return 1;
  }
}

void ty_init(struct ty_ctxt *ctxt)
{
  ht_init(&ctxt->registry, ty_hash, ty_eq);
}

void ty_deinit(struct ty_ctxt *ctxt)
{
  struct ht_entry e;
  for (ht_entry(&ctxt->registry, NULL, &e); ht_next(&ctxt->registry, &e);) {
    struct ty *ty = *ht_at(&ctxt->registry, &e);
    if (ty->kind == TY_ARRAY) {
      struct ty_array *array = (struct ty_array *) ty;
      free(array);
    } else if (ty->kind == TY_PROC) {
      struct ty_proc *proc = (struct ty_proc *) ty;
      vec_deinit(&proc->params);
      free(proc);
    }
  }
  ht_deinit(&ctxt->registry);
}

struct ty const *ty_int(void)
{
  static const struct ty ty = { TY_INT, 0 };
  return &ty;
}

struct ty const *ty_bool(void)
{
  static const struct ty ty = { TY_BOOL, 0 };
  return &ty;
}

struct ty const *ty_char(void)
{
  static const struct ty ty = { TY_CHAR, 0 };
  return &ty;
}

struct ty const *ty_array(struct ty_ctxt *ctxt, struct ty const *elem, size_t size)
{
  struct ht_entry e;

  struct ty_array array;
  struct ty *ty = (struct ty *) &array;
  array.ty.kind = TY_ARRAY;
  array.elem = elem;
  array.size = size;

  hash_init(&array.ty.hash);
  hash_add(&array.ty.hash, &array.ty.kind, sizeof(array.ty.kind));
  hash_add(&array.ty.hash, &array.elem, sizeof(array.elem));
  hash_add(&array.ty.hash, &array.size, sizeof(array.size));

  if (!ht_entry(&ctxt->registry, &ty, &e)) {
    struct ty_array *new_ty = malloc(sizeof(struct ty_array));
    *new_ty = array;
    ty = (struct ty *) new_ty;
    ht_occupy(&ctxt->registry, &e, &ty);
  }

  return *ht_at(&ctxt->registry, &e);
}

struct ty const *ty_proc(struct ty_ctxt *ctxt, struct ty const **params, size_t count)
{
  struct ht_entry e;
  size_t i;

  struct ty_proc proc;
  struct ty *ty = (struct ty *) &proc;
  proc.ty.kind = TY_PROC;
  vec_init(&proc.params);
  for (i = 0; i < count; ++i) {
    vec_push(&proc.params, &params[i]);
  }

  hash_init(&proc.ty.hash);
  hash_add(&proc.ty.hash, &proc.ty.kind, sizeof(proc.ty.kind));
  for (i = 0; i < proc.params.count; ++i) {
    struct ty const *param = proc.params.data[i];
    hash_add(&proc.ty.hash, &param, sizeof(param));
  }

  if (ht_entry(&ctxt->registry, &ty, &e)) {
    vec_deinit(&proc.params);
  } else {
    struct ty_proc *new_ty = malloc(sizeof(struct ty_proc));
    *new_ty = proc;
    ty = (struct ty *) new_ty;
    ht_occupy(&ctxt->registry, &e, &ty);
  }

  return *ht_at(&ctxt->registry, &e);
}
