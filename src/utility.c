#include <stdio.h>
#include <stdlib.h>

#include "utility.h"

void *xmalloc(size_t size)
{
  void *result = malloc(size);
  if (!result) {
    fprintf(stderr, "Internal Error: Failed to allocate memory. Aborted.");
    exit(EXIT_FAILURE);
  }
  return result;
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

#if defined(__GNUC__) || defined(__clang__)

int bit_popcount(unsigned long n)
{
  return __builtin_popcountl(n);
}

int bit_right_most(unsigned long n)
{
  return __builtin_ctzl(n);
}

int bit_left_most(unsigned long n)
{
  return 63 - __builtin_clzl(n);
}

#else

int bit_popcount(unsigned long n)
{
  int count = 0;
  for (; n; n &= n - 1) {
    ++count;
  }
  return count;
}

int bit_right_most(unsigned long n)
{
  return bit_popcount((n & (~n + 1)) - 1);
}

int bit_left_most(unsigned long n)
{
  int count = 0;
  for (; n; n >>= 1) {
    ++count;
  }
  return count;
}

#endif
