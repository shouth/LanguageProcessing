/*
 * sym.h -- interned symbol
 *
 * SPDX-FileCopyrightText: 2026 Shota Minami
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SYM_H
#define SYM_H

#include <stddef.h>

#include "ds.h"

struct sym {
  char const *str;
  size_t len;
  hash_t hash;
};

struct sym_ctxt {
  hs(struct sym *) registry;
};

hash_t sym_hash(void const *item);

int sym_eq(void const *lhs, void const *rhs);

void sym_init(struct sym_ctxt *ctxt);

void sym_deinit(struct sym_ctxt *ctxt);

struct sym const *sym_intern(struct sym_ctxt *ctxt, char const *str, size_t len);

#endif /* SYM_H */
