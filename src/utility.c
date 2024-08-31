/*
   Copyright 2022 Shota Minami

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utility.h"

void *xmalloc(unsigned long size)
{
  void *result = malloc(size);
  if (!result) {
    fprintf(stderr, "Internal Error: Failed to allocate memory. Aborted.");
    exit(EXIT_FAILURE);
  }
  return result;
}

void *memdup(const void *ptr, unsigned long length)
{
  if (length == 0) {
    return NULL;
  } else {
    void *result = xmalloc(length);
    memcpy(result, ptr, length);
    return result;
  }
}

char *strndup(const char *src, unsigned long length)
{
  if (length == 0) {
    return NULL;
  } else {
    char *dest = xmalloc(length + 1);
    strncpy(dest, src, length);
    dest[length] = '\0';
    return dest;
  }
}

unsigned long splice(
  void *dest, unsigned long dest_count,
  const void *src1, unsigned long src1_count,
  unsigned long size, unsigned long offset, unsigned long span,
  const void *src2, unsigned long src2_count)
{
  unsigned long result_count = src1_count + src2_count - span;
  if (result_count > dest_count) {
    result_count = dest_count;
  }

  if (src1) {
    memcpy(dest, src1, offset * size);
  }

  if (offset + src2_count < dest_count) {
    unsigned long remain = dest_count - (offset + src2_count);
    unsigned long count  = src1_count - (offset + span);
    if (count > remain) {
      count = remain;
    }

    if (count > 0) {
      if (src1) {
        memcpy((char *) dest + (offset + src2_count) * size, (char *) src1 + (offset + span) * size, count * size);
      } else {
        memmove((char *) dest + (offset + src2_count) * size, (char *) dest + (offset + span) * size, count * size);
      }
    }
  }

  {
    unsigned long remain = dest_count - offset;
    unsigned long count  = src2_count;
    if (count > remain) {
      count = remain;
    }

    if (count > 0) {
      if (src2) {
        memcpy((char *) dest + offset * size, (char *) src2, count * size);
      }
    }
  }

  return result_count;
}

void *splice_alloc(
  const void *src1, unsigned long src1_count,
  unsigned long size, unsigned long offset, unsigned long span,
  const void *src2, unsigned long src2_count,
  unsigned long *result_count)
{
  unsigned long count  = src1_count + src2_count - span;
  void         *result = xmalloc(count * size);
  *result_count        = splice(result, count, src1, src1_count, size, offset, span, src2, src2_count);
  return result;
}

void hash_fnv1a(Hash *hash, const void *ptr, unsigned long len)
{
  const unsigned char *data  = ptr;
  const unsigned char *end   = data + len;
  const unsigned long  prime = 0x01000193ul;

  for (; data < end; ++data) {
    *hash = 0xFFFFFFFFul & ((*hash ^ *data) * prime);
  }
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

long utf8_len(const char *str, long len)
{
  if ((str[0] & 0x80) == 0x00) {
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
