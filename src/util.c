/* SPDX-License-Identifier: Apache-2.0 */

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

/* Hash */

Hash hash_fnv1a(unsigned long *hash, const void *ptr, unsigned long len)
{
  unsigned long i;

  Hash result = hash ? *hash : 0x811C9DC5ul;
  for (i = 0; i < len; ++i) {
    result = 0xFFFFFFFFul & ((result ^ *((unsigned char *) ptr + i)) * 0x01000193ul);
  }
  if (hash) {
    *hash = result;
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
