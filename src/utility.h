#ifndef UTILITY_H
#define UTILITY_H

#include <stdlib.h>

void *xmalloc(unsigned long size);

#define FNV1A_INIT 0x811C9DC5ul

unsigned long fnv1a(unsigned long hash, const void *ptr, unsigned long len);

#define unreachable()                                                                            \
  do {                                                                                           \
    fprintf(stderr, "Internal Error: Entered unreachable region [%s:%d]\n", __FILE__, __LINE__); \
    exit(EXIT_FAILURE);                                                                          \
  } while (0)

#endif
