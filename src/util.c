/* SPDX-License-Identifier: Apache-2.0 */

#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"

/* Memory */

void *xmalloc(unsigned long size)
{
  void *result = malloc(size);
  if (!result) {
    fprintf(stderr, "Internal Error: Failed to allocate memory. Aborted.");
    exit(EXIT_FAILURE);
  }
  return result;
}

unsigned long popcount(const void *data, unsigned long count)
{
#define B2(n) n, n + 1, n + 1, n + 2
#define B4(n) B2(n), B2(n + 1), B2(n + 1), B2(n + 2)
#define B6(n) B4(n), B4(n + 1), B4(n + 1), B4(n + 2)

  static const unsigned char table[] = {
    B6(0), B6(1), B6(1), B6(2)
  };

#undef B2
#undef B4
#undef B6

  unsigned long result = 0;
  unsigned long i;
  for (i = 0; i < count; ++i) {
    result += table[((unsigned char *) data)[i]];
  }
  return result;
}

/* Vec */

void *vec_reserve_impl(void *ptr, unsigned long size, unsigned long used, unsigned long capacity)
{
  unsigned long i;
  void         *result;

  --capacity;
  for (i = 1; i < sizeof(i) * CHAR_BIT; i <<= 1) {
    capacity |= capacity >> i;
  }
  ++capacity;

  result = xmalloc(size * capacity);

  if (ptr) {
    memcpy(result, ptr, size * used);
    free(ptr);
  }

  return result;
}

/* Charactor */

int is_alphabet(int c)
{
  return !!strchr("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz", c);
}

int is_number(int c)
{
  return c >= '0' && c <= '9';
}

int is_space(int c)
{
  return !!strchr(" \t\r\n", c);
}

int is_graphic(int c)
{
  return is_alphabet(c) || is_number(c) || is_space(c) || !!strchr("!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~", c);
}

long utf8_len(const char *str, unsigned long len)
{
  if (len == 0) {
    return 0;
  } else if ((str[0] & 0x80) == 0x00) {
    return 1;
  } else if ((str[0] & 0xE0) == 0xC0) {
    return len >= 2 && (str[1] & 0xC0) == 0x80 ? 2 : -1;
  } else if ((str[0] & 0xF0) == 0xE0) {
    return len >= 3 && (str[1] & 0xC0) == 0x80 && (str[2] & 0xC0) == 0x80 ? 3 : -1;
  } else if ((str[0] & 0xF8) == 0xF0) {
    return len >= 4 && (str[1] & 0xC0) == 0x80 && (str[2] & 0xC0) == 0x80 && (str[3] & 0xC0) == 0x80 ? 4 : -1;
  } else {
    return -1;
  }
}

/* Text */

void text_offsets_update(size_t *offsets, size_t count, size_t index, size_t length)
{
  size_t i;
  assert(count > index);

  for (i = index + 1; i < count; i += i & -i) {
    offsets[i] += length;
  }
}

size_t text_offsets_at(size_t const *offsets, size_t count, size_t index)
{
  size_t i;
  size_t offset = 0;
  assert(count > index);

  for (i = index; i > 0; i -= i & -i) {
    offset += offsets[i];
  }
  return offset;
}

size_t text_offsets_locate(size_t const *offsets, size_t count, size_t offset, size_t *out_column)
{
  size_t i, j;
  size_t line;
  size_t start = 0;

  i = count;
  for (j = 1; j < sizeof(size_t) * CHAR_BIT; j <<= 1) {
    i |= i >> j;
  }
  i = (i + 1) >> 1;

  for (line = 0; i > 0; i >>= 1) {
    if (line + i <= count && start + offsets[line + i] <= offset) {
      start += offsets[line + i];
      line += i;
    }
  }

  if (out_column) {
    *out_column = offset - start;
  }
  return line;
}

/* Misc */

char *load_file(char const *filename, size_t *out_length)
{
  FILE *file = fopen(filename, "rb");
  char *content = NULL;
  size_t capacity = 4096;
  size_t read_size;

  if (!file) {
    return NULL;
  }

  while (1) {
    content = xmalloc(capacity);
    read_size = fread(content, 1, capacity, file);
    if (ferror(file)) {
      free(content);
      content = NULL;
      break;
    }
    if (read_size + 1 < capacity) {
      content[read_size] = '\0';
      break;
    }
    capacity *= 2;
    free(content);
    rewind(file);
  }
  fclose(file);

  if (content && out_length) {
    *out_length = read_size;
  }
  return content;
}
