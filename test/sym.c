/*
 * sym.c -- interned symbol test
 *
 * SPDX-FileCopyrightText: 2026 Shota Minami
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sym.h"

int main(void)
{
  struct sym_ctxt ctxt;
  sym_init(&ctxt);

  {
    char const *s1 = "hello";
    char const *s2 = "world";
    char const *s3 = "hello";
    char const *s4 = "hello, world";

    struct sym const *sym1 = sym_intern(&ctxt, s1, 5);
    struct sym const *sym2 = sym_intern(&ctxt, s2, 5);
    struct sym const *sym3 = sym_intern(&ctxt, s3, 5);
    struct sym const *sym4 = sym_intern(&ctxt, s4, 12);

    assert(sym1 != sym2);
    assert(sym1 == sym3);
    assert(sym1 != sym4);
  }

  sym_deinit(&ctxt);

  return EXIT_SUCCESS;
}
