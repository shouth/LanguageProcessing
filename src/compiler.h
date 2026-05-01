/*
 * compiler.h -- compiler
 *
 * SPDX-FileCopyrightText: 2026 Shota Minami
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef COMPILER_H
#define COMPILER_H

#include <stddef.h>
#include <stdio.h>

#include "diag.h"
#include "syn.h"

char const *load(char const *path, size_t *len);

struct token {
  enum syn_kind kind;
  size_t len;
  int nonclosed;
  int nongraphic;
};

int lex(char const *text, size_t len, struct token *token);

int parse(char const *text, size_t len, char const *filename, struct diag *diag, struct syn_program **program);

void pretty(struct syn_program const *program, FILE *out);

#endif /* COMPILER_H */
