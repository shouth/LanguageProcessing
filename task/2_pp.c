/*
 * 2_pp.c -- task2: parser
 *
 * SPDX-FileCopyrightText: 2026 Shota Minami
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "driver.h"
#include "diag.h"
#include "query.h"

int main(int argc, char const *argv[])
{
  struct query_ctxt ctxt;
  query_id_t id;
  struct query_parse *parse;

  query_init(&ctxt);

  if (argc != 2) {
    fprintf(stderr, "usage: %s <file>\n", argv[0]);
    goto cleanup;
  }

  id = query_add(&ctxt, argv[1]);
  parse = query_parse(&ctxt, id);
  if (parse->status != QUERY_OK) {
    if (parse->status == QUERY_ERR_NOT_FOUND) {
      fprintf(stderr, "error: file not found: %s\n", argv[1]);
    } else if (parse->status == QUERY_ERR_BAD_SYNTAX) {
      diag_print(&parse->diag, stderr);
    }
    goto cleanup;
  }
  pretty(parse->syn, stdout);

cleanup:
  query_deinit(&ctxt);
  return EXIT_SUCCESS;
}
