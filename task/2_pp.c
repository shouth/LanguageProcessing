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

int main(int argc, char const *argv[])
{
  int status = EXIT_FAILURE;
  
  struct q_ctxt ctxt;
  struct q_load const *load;
  struct q_parse const *parse;

  q_init(&ctxt);

  if (argc != 2) {
    fprintf(stderr, "usage: %s <file>\n", argv[0]);
    goto exit;
  }

  load = q_load(&ctxt, argv[1]);
  if (load->status != Q_OK) {
    fprintf(stderr, "error: file not found");
    goto exit;
  }
  
  parse = q_parse(&ctxt, load->file);
  if (parse->status != Q_OK) {
    diag_print(parse->diag, stderr);
    goto exit;
  }

  pretty(parse->syn, stdout);

exit:
  q_deinit(&ctxt);

  return status;
}
