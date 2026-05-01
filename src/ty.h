/*
 * ty.h -- type system
 *
 * SPDX-FileCopyrightText: 2026 Shota Minami
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TY_H
#define TY_H

#include "ds.h"

enum ty_kind {
  TY_INT,
  TY_BOOL,
  TY_CHAR,
  TY_ARRAY,
  TY_PROC
};

struct ty {
  enum ty_kind kind;
  hash_t hash;
};

struct ty_array {
  struct ty ty;
  struct ty const *elem;
  size_t size;
};

struct ty_proc {
  struct ty ty;
  vec(struct ty const *) params;
};

struct ty_ctxt {
  hs(struct ty *) registry;
};

void ty_init(struct ty_ctxt *ctxt);

void ty_deinit(struct ty_ctxt *ctxt);

struct ty const *ty_int(void);

struct ty const *ty_bool(void);

struct ty const *ty_char(void);

struct ty const *ty_array(struct ty_ctxt *ctxt, struct ty const *elem, size_t size);

struct ty const *ty_proc(struct ty_ctxt *ctxt, struct ty const **params, size_t count);

#endif /* TY_H */
