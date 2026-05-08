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
#include "ds.h"
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

void pretty(struct syn_program const *program, FILE *out);

#define Q_OK              0
#define Q_FILE_NOT_FOUND  1
#define Q_BAD_SYNTAX      2

struct q_load {
  char *name;
  int status;
  struct file *file;
};

struct q_parse {
  struct file const *file;
  int status;
  struct syn_program *syn;
  struct diag *diag;
};

struct q_ctxt {
  struct sym_ctxt sym;
  hs(struct q_load) load;
  hs(struct q_parse) parse;
};

void q_init(struct q_ctxt *q);

void q_deinit(struct q_ctxt *q);

struct q_load const *q_load(struct q_ctxt *q, char const *path);

struct q_parse const *q_parse(struct q_ctxt *q, struct file const *file);

#endif /* DRIVER_H */
