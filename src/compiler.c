/*
 * compiler.c -- compiler
 *
 * SPDX-FileCopyrightText: 2026 Shota Minami
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "compiler.h"

char const *load(char const *path, size_t *len)
{
  FILE *file = NULL;
  size_t read = 0;
  char *buffer = NULL;
  char *result = NULL;

  if (!(file = fopen(path, "rb"))) {
    goto cleanup;
  }

  while (!feof(file)) {
    char buf[4096];
    read += fread(buf, 1, sizeof(buf), file);
    if (ferror(file)) {
      goto cleanup;
    }
  }

  if (!(buffer = malloc(read + 1))) {
    goto cleanup;
  }

  rewind(file);
  if (fread(buffer, 1, read, file) != read) {
    goto cleanup;
  }

  buffer[read] = '\0';
  result = buffer;
  buffer = NULL;
  if (len) {
    *len = read;
  }

cleanup:
  if (file) {
    fclose(file);
  }
  free(buffer);

  return result;
}
