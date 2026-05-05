/*
 * query.c -- queries for demand-driven compilation
 *
 * SPDX-FileCopyrightText: 2026 Shota Minami
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <stdlib.h>

#include "diag.h"
#include "driver.h"
#include "ds.h"
#include "query.h"
#include "src.h"
#include "syn.h"

static struct query_unit *get(struct query_ctxt *query, query_id_t id)
{
  if (id < query->units.count) {
    return &query->units.data[id];
  } else {
    return NULL;
  }
}

void query_init(struct query_ctxt *query)
{
  vec_init(&query->units);
}

void query_deinit(struct query_ctxt *query)
{
  size_t i;
  for (i = 0; i < query->units.count; ++i) {
    struct query_unit *u = &query->units.data[i];

    free(u->path);

    if (u->load) {
      src_deinit(&u->load->src);
      free(u->load);
    }

    if (u->parse) {
      syn_free((struct syn_node *) u->parse->syn);
      diag_deinit(&u->parse->diag);
      free(u->parse);
    }
  }
  vec_deinit(&query->units);
}

query_id_t query_add(struct query_ctxt *query, char const *path)
{
  query_id_t id = query->units.count;
  struct query_unit u;
  size_t len = strlen(path);
  u.path = malloc(len + 1);
  memcpy(u.path, path, len + 1);
  u.load = NULL;
  u.parse = NULL;
  vec_push(&query->units, &u);
  return id;
}

struct query_load *query_load(struct query_ctxt *query, query_id_t id)
{
  struct query_unit *u = get(query, id);
  if (!u) {
    return NULL;
  } else if (u->load) {
    return u->load;
  } else {
    u->load = malloc(sizeof(struct query_load));
    if (src_init(&u->load->src, u->path)) {
      u->load->status = QUERY_OK;
    } else {
      u->load->status = QUERY_ERR_NOT_FOUND;
    }
    return u->load;
  }
}

struct query_parse *query_parse(struct query_ctxt *query, query_id_t id)
{
  struct query_unit *u = get(query, id);
  if (!u) {
    return NULL;
  } else if (u->parse) {
    return u->parse;
  } else {
    struct query_load *load = query_load(query, id);
    u->parse = malloc(sizeof(struct query_parse));
    u->parse->syn = NULL;
    diag_init(&u->parse->diag, NULL);
    if (!load) {
      u->parse->status = QUERY_ERR_NOT_FOUND;
    } else if (load->status != QUERY_OK) {
      u->parse->status = load->status;
    } else if (parse(load->src.text, load->src.text_len, &load->src, &u->parse->diag, &u->parse->syn)) {
      u->parse->status = QUERY_OK;
    } else {
      u->parse->status = QUERY_ERR_BAD_SYNTAX;
    }
    return u->parse;
  }
}
