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

enum query_status {
  QUERY_OK,
  QUERY_ERR_NOT_FOUND,
  QUERY_ERR_BAD_SYNTAX
};

struct query_load {
  enum query_status status;
  struct file file;
};

struct query_parse {
  enum query_status status;
  struct syn_program *syn;
  struct diag diag;
};

struct query {
  char *path;
  struct sym_ctxt sym_ctxt;
  struct query_load *load;
  struct query_parse *parse;
};

typedef size_t query_id_t;

struct query_ctxt {
  vec(struct query) entries;
};

void query_init(struct query_ctxt *query);

void query_deinit(struct query_ctxt *query);

query_id_t query_add(struct query_ctxt *query, char const *path);

struct query_load const *query_load(struct query_ctxt *query, query_id_t id);

struct query_parse const *query_parse(struct query_ctxt *query, query_id_t id);

#endif /* DRIVER_H */
