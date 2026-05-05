/*
 * src.c -- source code
 *
 * SPDX-FileCopyrightText: 2026 Shota Minami
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "src.h"
#include "ds.h"

static char *load_text(char const *path, size_t *len)
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

static size_t *make_offsets(char const *text, size_t text_len, size_t *line_count)
{
  size_t i, j;
  vec(size_t) offsets;
  size_t offset;

  vec_init(&offsets);
  for (i = 0; i < text_len; i = j) {
    for (j = i; j < text_len; ++j) {
      if (text[j] == '\n') {
        ++j;
        if (text[j] == '\r') {
          ++j;
        }
        offset = j - i;
        vec_push(&offsets, &offset);
        break;
      }
      if (text[j] == '\r') {
        ++j;
        if (text[j] == '\n') {
          ++j;
        }
        offset = j - i;
        vec_push(&offsets, &offset);
        break;
      }
    }
  }

  fw_build(offsets.data, offsets.count);
  *line_count = offsets.count;
  return offsets.data;
}

int src_init(struct src *src, const char *path)
{
  size_t len = strlen(path);
  src->name = malloc(len + 1);
  memcpy(src->name, path, len + 1);
  src->text = load_text(path, &src->text_len);

  if (src->text) {
    src->line_offsets = make_offsets(src->text, src->text_len, &src->line_count);
    return 1;
  } else {
    src->line_offsets = NULL;
    src->line_count = 0;
    return 0;
  }
}

void src_deinit(struct src *src)
{
  free(src->name);
  free(src->text);
  free(src->line_offsets);
}

int src_locate(struct src *src, size_t offset, size_t *line, size_t *column)
{
  if (offset >= src->text_len) {
    return 0;
  } else {
    *line = fw_upper_bound(src->line_offsets, src->line_count, offset, column);
    return 1;
  }
}
