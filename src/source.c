/* SPDX-License-Identifier: Apache-2.0 */

#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "source.h"
#include "util.h"

static char *read_all(char const* filename, size_t *out_size)
{
  FILE *file = fopen(filename, "rb");
  char *buffer = NULL;
  unsigned long capacity = 4096;

  if (!file) {
    return NULL;
  }

  while (1) {
    buffer = xmalloc(capacity);
    *out_size = fread(buffer, 1, capacity, file);
    if (ferror(file)) {
      free(buffer);
      buffer = NULL;
      break;
    }
    if (*out_size + 1 < capacity) {
      buffer[*out_size] = '\0';
      break;
    }
    capacity *= 2;
    free(buffer);
    rewind(file);
  }

  fclose(file);
  return buffer;
}

static size_t count_lines(char const* text)
{
  size_t count = 1;
  assert(text != NULL);

  for (; *text; ++text, ++count) {
    text += strcspn(text, "\r\n");
    if (!strncmp(text, "\r\n", 2) || !strncmp(text, "\n\r", 2)) {
      ++text;
    }
  }
  return count;
}

static size_t *build_offset_tree(char const* text, size_t line_count)
{
  size_t i = 0, j = 0;
  size_t *offset_tree = xmalloc(sizeof(size_t) * (line_count + 1));
  memset(offset_tree, 0, sizeof(size_t) * (line_count + 1));

  for (; *text; ++i) {
    char const *start = text;
    text += strcspn(text, "\r\n");
    if (!strncmp(text, "\r\n", 2) || !strncmp(text, "\n\r", 2)) {
      ++text;
    }
    ++text;

    for (j = i + 1; j <= line_count; j += j & -j) {
      offset_tree[j] += text - start;
    }
  }
  return offset_tree;
}

Source *source_new(char const *filename, size_t filename_len)
{
  size_t i;
  char *ptr;
  Source *source = xmalloc(sizeof(Source));

  source->filename = xmalloc(filename_len + 1);
  strncpy(source->filename, filename, filename_len);
  source->filename[filename_len] = '\0';

  source->text = read_all(source->filename, &source->text_length);
  source->line_count = count_lines(source->text);
  source->offset_tree = build_offset_tree(source->text, source->line_count);

  return source;
}

void source_free(Source *source)
{
  if (source) {
    free(source->filename);
    free(source->text);
    free(source->offset_tree);
    free(source);
  }
}

int source_offset_location(const Source *source, size_t offset, SourceLocation *location)
{
  size_t i, j;
  size_t sum = 0;

  if (source->text_length < offset) {
    return 0;
  }

  i = source->line_count;
  for (j = 1; j < sizeof(source->line_count); j <<= 1) {
    i |= i >> j;
  }
  i += 1;
  i >>= 1;

  for (j = 0; i > 0; i >>= 1) {
    if (j + i <= source->line_count && sum + source->offset_tree[j + i] <= offset) {
      sum += source->offset_tree[j + i];
      j += i;
    }
  }

  location->line = j;
  location->column = offset - sum;
  return 1;
}

static size_t source_offset(Source const *source, size_t line)
{
  size_t offset = 0;
  size_t i;
  assert(source->line_count > line);

  for (i = line; line > 0; line -= line & -line) {
    offset += source->offset_tree[line];
  }
  return offset;
}

int source_line_range(Source const *source, size_t line, SourceRange *range)
{
  if (source->line_count < line) {
    return 0;
  }

  range->offset = source_offset(source, line);
  range->length = source_offset(source, line + 1) - range->offset;
  return 1;
}
