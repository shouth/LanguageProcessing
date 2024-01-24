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

void *dup(const void *ptr, unsigned long size, unsigned long count)
{
  if (count == 0) {
    return NULL;
  } else {
    void *result = xmalloc(size * count);
    memcpy(result, ptr, size * count);
    return result;
  }
}

unsigned long fnv1a(unsigned long hash, const void *ptr, unsigned long len)
{
  const unsigned char *data  = ptr;
  const unsigned char *end   = data + len;
  const unsigned long  prime = 0x01000193ul;

  for (; data < end; ++data) {
    hash = (hash ^ *data) * prime;
  }
  return 0xFFFFFFFFul & hash;
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
