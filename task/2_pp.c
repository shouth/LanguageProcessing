/*
 * 2_pp.c -- task2: parser
 *
 * SPDX-FileCopyrightText: 2026 Shota Minami
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "compiler.h"
#include "diag.h"
#include "syn.h"

int main(int argc, char const *argv[])
{
  char const *text = NULL;
  size_t len;
  struct syn_program *program = NULL;
  struct diag diag;

  diag_init(&diag);

  if (argc != 2) {
    fprintf(stderr, "usage: %s <file>\n", argv[0]);
    goto cleanup;
  }

  if (!(text = load(argv[1], &len))) {
    fprintf(stderr, "error: failed to load file\n");
    goto cleanup;
  }

  if (parse(text, len, argv[1], &diag, &program)) {
    pretty(program, stdout);
  }
  diag_print(&diag, stdout);

cleanup:
  free((void *)text);
  syn_free((struct syn_node *) program);
  diag_deinit(&diag);
  return EXIT_SUCCESS;
}
