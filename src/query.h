/*
 * query.h -- queries for demand-driven compilation
 *
 * SPDX-FileCopyrightText: 2026 Shota Minami
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef QUERY_H
#define QUERY_H

#include "diag.h"
#include "ds.h"
#include "file.h"
#include "syn.h"

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

struct query_entry {
  char *path;
  struct query_load *load;
  struct query_parse *parse;
};

typedef size_t query_id_t;

struct query_ctxt {
  vec(struct query_entry) entries;
};

void query_init(struct query_ctxt *query);

void query_deinit(struct query_ctxt *query);

query_id_t query_add(struct query_ctxt *query, char const *path);

struct query_load const *query_load(struct query_ctxt *query, query_id_t id);

struct query_parse const *query_parse(struct query_ctxt *query, query_id_t id);

#endif /* QUERY_H */
