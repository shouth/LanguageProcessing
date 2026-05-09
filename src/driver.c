/*
 * driver.c -- drivers of the compiler
 *
 * SPDX-FileCopyrightText: 2026 Shota Minami
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "driver.h"
#include "diag.h"
#include "ds.h"
#include "file.h"
#include "sym.h"
#include "syn.h"
#include "ty.h"
#include "unit.h"

static hash_t str_hash(void const *item)
{
  char const *x = *(char **) item;
  hash_t h;
  hash_init(&h);
  hash_add(&h, x, strlen(x));
  return h;
}

static int str_eq(void const *lhs, void const *rhs)
{
  char const *l = *(char **) lhs;
  char const *r = *(char **) rhs;
  return !strcmp(l, r);
}

static hash_t file_hash(void const *item)
{
  struct file const *x = *(struct file **) item;
  hash_t h;
  hash_init(&h);
  hash_add(&h, &x, sizeof(struct file *));
  return h;
}

static int file_eq(void const *lhs, void const *rhs)
{
  struct file const *l = *(struct file **) lhs;
  struct file const *r = *(struct file **) rhs;
  return l == r;
}

void q_init(struct q_ctxt *q)
{
  sym_init(&q->sym);
  ty_init(&q->ty);
  ht_init(&q->load, str_hash, str_eq);
  ht_init(&q->parse, file_hash, file_eq);
  ht_init(&q->resolve, file_hash, file_eq);
}

void q_deinit(struct q_ctxt *q)
{
  struct ht_entry e;

  for (ht_entry(&q->resolve, NULL, &e); ht_next(&q->resolve, &e);) {
    struct q_resolve *x = ht_at(&q->resolve, &e);
    unit_deinit(x->unit);
    free(x->unit);
    diag_deinit(x->diag);
    free(x->diag);
  }
  ht_deinit(&q->resolve);
  
  for (ht_entry(&q->parse, NULL, &e); ht_next(&q->parse, &e);) {
    struct q_parse *x = ht_at(&q->parse, &e);
    syn_free(&x->syn->syn.node);
    diag_deinit(x->diag);
    free(x->diag);
  }
  ht_deinit(&q->parse);

  for (ht_entry(&q->load, NULL, &e); ht_next(&q->load, &e);) {
    struct q_load *x = ht_at(&q->load, &e);
    free(x->name);
    file_deinit(x->file);
    free(x->file);
  }
  ht_deinit(&q->load);

  ty_deinit(&q->ty);
  sym_deinit(&q->sym);
}

struct q_load const *q_load(struct q_ctxt *q, char const *path)
{
  struct ht_entry e;
  if (!ht_entry(&q->load, &path, &e)) {
    struct q_load ql;
    size_t len = strlen(path);

    ql.name = malloc(len + 1);
    memcpy(ql.name, path, len + 1);

    ql.file = malloc(sizeof(struct file));
    if (file_init(ql.file, ql.name)) {
      ql.status = Q_OK;
    } else {
      ql.status = Q_FILE_NOT_FOUND;
    }

    ht_occupy(&q->load, &e, &ql);
  }
  return ht_at(&q->load, &e);
}
