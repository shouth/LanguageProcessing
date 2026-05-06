/*
 * driver.h -- drivers of the compiler
 *
 * SPDX-FileCopyrightText: 2026 Shota Minami
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef DRIVER_H
#define DRIVER_H

#include <stddef.h>
#include <stdio.h>

#include "diag.h"
#include "file.h"
#include "sym.h"
#include "syn.h"

struct token {
  enum syn_kind kind;
  size_t len;
  int nonclosed;
  int nongraphic;
};

int lex(char const *text, size_t len, struct token *token);

int parse(char const *text, size_t len, struct sym_ctxt *ctxt, struct file const *file, struct diag *diag, struct syn_program **program);

void pretty(struct syn_program const *program, FILE *out);

#endif /* DRIVER_H */
