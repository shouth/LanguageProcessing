/*
 * sym.c -- interned symbol
 *
 * SPDX-FileCopyrightText: 2026 Shota Minami
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <string.h>

#include "ds.h"
#include "sym.h"

unsigned long sym_hash(void const *x)
{
  struct sym const *s = *(struct sym const **) x;
  return s->hash;
}

int sym_eq(void const *lhs, void const *rhs)
{
  struct sym const *l = *(struct sym const **) lhs;
  struct sym const *r = *(struct sym const **) rhs;
  return l->hash == r->hash && l->len == r->len && !memcmp(l->str, r->str, l->len);
}

void sym_init(struct sym_ctxt *ctxt)
{
  ht_init(&ctxt->syms, sym_hash, sym_eq);
}

void sym_deinit(struct sym_ctxt *ctxt)
{
  struct ht_entry e;
  for (ht_entry(&ctxt->syms, NULL, &e); ht_next(&ctxt->syms, &e);) {
    struct sym *s = *ht_at(&ctxt->syms, &e);
    free((char *) s->str);
    free(s);
  }

  ht_deinit(&ctxt->syms);
}

struct sym const *sym_intern(struct sym_ctxt *ctxt, char const *str, size_t len)
{
  struct ht_entry e;

  struct sym key, *s = &key;
  key.str = str;
  key.len = len;
  hash_init(&key.hash);
  hash_add(&key.hash, key.str, key.len);

  if (!ht_entry(&ctxt->syms, &s, &e)) {
    s = malloc(sizeof(struct sym));
    s->str = malloc(key.len + 1);
    memcpy((char *) s->str, str, len);
    ((char *) s->str)[len] = '\0';
    s->len = key.len;
    s->hash = key.hash;
    ht_occupy(&ctxt->syms, &e, &s);
  }
  return *ht_at(&ctxt->syms, &e);
}
