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
  struct src src;
  struct syn_program *program = NULL;
  struct diag diag;

  diag_init(&diag);

  if (argc != 2) {
    fprintf(stderr, "usage: %s <file>\n", argv[0]);
    goto cleanup;
  }

  if (!src_init(&src, argv[1])) {
    fprintf(stderr, "error: failed to load file\n");
    goto cleanup;
  }

  if (parse(src.text, src.text_len, &src, &diag, &program)) {
    pretty(program, stdout);
  }
  diag_print(&diag, stdout);

cleanup:
  src_deinit(&src);
  syn_free((struct syn_node *) program);
  diag_deinit(&diag);
  return EXIT_SUCCESS;
}
