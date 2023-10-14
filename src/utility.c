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
