/*
 * syn.c -- syntax tree test
 *
 * SPDX-FileCopyrightText: 2026 Shota Minami
 * SPDX-License-Identifier: Apache-2.0
 */

#define _POSIX_C_SOURCE 200809L

#include <stdlib.h>
#include <stdio.h>

#include "syn.h"

int main(void)
{
  syn_ckpt_t binary, entire;
  struct syn_binary_expr *expr;
  struct syn_bldr bldr;

  char buffer[256];
  FILE *out = fmemopen(buffer, sizeof(buffer), "w");

  char const *expected =
    "SYN_BINARY_EXPR @ 0..5\n"
    "  SYN_ENTIRE_VAR_EXPR @ 0..1\n"
    "    SYN_IDENT @ 0..1 \"x\"\n"
    "  SYN_PLUS @ 2..3 \"+\"\n"
    "    SYN_WHITESPACE @ 1..2\n"
    "  SYN_ENTIRE_VAR_EXPR @ 3..5\n"
    "    SYN_IDENT @ 4..5 \"y\"\n"
    "      SYN_WHITESPACE @ 3..4\n";

  syn_bldr_init(&bldr);

  binary = syn_bldr_open(&bldr);
  entire = syn_bldr_open(&bldr);
  syn_bldr_tok(&bldr, SYN_IDENT, "x", 1);
  syn_bldr_close(&bldr, SYN_ENTIRE_VAR_EXPR, entire);
  syn_bldr_triv(&bldr, SYN_WHITESPACE, " ", 1);
  syn_bldr_tok(&bldr, SYN_PLUS, "+", 1);
  syn_bldr_triv(&bldr, SYN_WHITESPACE, " ", 1);
  entire = syn_bldr_open(&bldr);
  syn_bldr_tok(&bldr, SYN_IDENT, "y", 1);
  syn_bldr_close(&bldr, SYN_ENTIRE_VAR_EXPR, entire);
  syn_bldr_close(&bldr, SYN_BINARY_EXPR, binary);

  expr = (struct syn_binary_expr *) syn_bldr_finish(&bldr);
  syn_bldr_deinit(&bldr);

  syn_print((struct syn_node *) expr, out);
  fflush(out);
  assert(strcmp(buffer, expected) == 0);

  fclose(out);
  syn_free((struct syn_node *) expr);
  return EXIT_SUCCESS;
}
