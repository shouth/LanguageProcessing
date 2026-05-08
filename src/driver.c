/*
 * driver.c -- drivers of the compiler
 *
 * SPDX-FileCopyrightText: 2026 Shota Minami
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <stdlib.h>

#include "diag.h"
#include "driver.h"
#include "ds.h"
#include "file.h"
#include "sym.h"
#include "syn.h"

static struct query *get(struct query_ctxt *query, query_id_t id)
{
  if (id < query->entries.count) {
    return &query->entries.data[id];
  } else {
    return NULL;
  }
}

void query_init(struct query_ctxt *query)
{
  vec_init(&query->entries);
}

void query_deinit(struct query_ctxt *query)
{
  size_t i;
  for (i = 0; i < query->entries.count; ++i) {
    struct query *u = &query->entries.data[i];

    if (u->parse) {
      syn_free((struct syn_node *) u->parse->syn);
      diag_deinit(&u->parse->diag);
      free(u->parse);
    }

    if (u->load) {
      file_deinit(&u->load->file);
      free(u->load);
    }

    sym_deinit(&u->sym_ctxt);
    free(u->path);
  }
  vec_deinit(&query->entries);
}

query_id_t query_add(struct query_ctxt *query, char const *path)
{
  query_id_t id = query->entries.count;
  struct query u;
  size_t len = strlen(path);
  u.path = malloc(len + 1);
  memcpy(u.path, path, len + 1);
  sym_init(&u.sym_ctxt);
  u.load = NULL;
  u.parse = NULL;
  vec_push(&query->entries, &u);
  return id;
}

struct query_load const *query_load(struct query_ctxt *query, query_id_t id)
{
  struct query *u = get(query, id);
  if (!u) {
    return NULL;
  } else if (u->load) {
    return u->load;
  } else {
    u->load = malloc(sizeof(struct query_load));
    if (file_init(&u->load->file, u->path)) {
      u->load->status = QUERY_OK;
    } else {
      u->load->status = QUERY_ERR_NOT_FOUND;
    }
    return u->load;
  }
}

struct query_parse const *query_parse(struct query_ctxt *query, query_id_t id)
{
  struct query *u = get(query, id);
  if (!u) {
    return NULL;
  } else if (u->parse) {
    return u->parse;
  } else {
    struct query_load const *load = query_load(query, id);
    u->parse = malloc(sizeof(struct query_parse));
    u->parse->syn = NULL;
    diag_init(&u->parse->diag, NULL);
    if (!load) {
      u->parse->status = QUERY_ERR_NOT_FOUND;
    } else if (load->status != QUERY_OK) {
      u->parse->status = load->status;
    } else if (parse(load->file.text, load->file.text_len, &u->sym_ctxt, &load->file, &u->parse->diag, &u->parse->syn)) {
      u->parse->status = QUERY_OK;
    } else {
      u->parse->status = QUERY_ERR_BAD_SYNTAX;
    }
    return u->parse;
  }
}
