#ifndef UTILITY_H
#define UTILITY_H

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

void *xmalloc(unsigned long size);
void *dup(const void *ptr, unsigned long size, unsigned long count);

#define FNV1A_INIT 0x811C9DC5ul

unsigned long fnv1a(unsigned long hash, const void *ptr, unsigned long len);

unsigned long popcount(void *data, unsigned long size);

#define ULONG_BIT (sizeof(unsigned long) * CHAR_BIT)

#define BITSET(name, bits) unsigned long name[(bits + ULONG_BIT - 1) / ULONG_BIT]

#define bitset_set(self, index)   (self[(index) / ULONG_BIT] |= 1ul << ((index) % ULONG_BIT))
#define bitset_reset(self, index) (self[(index) / ULONG_BIT] &= ~(1ul << ((index) % ULONG_BIT)))
#define bitset_get(self, index)   ((self[(index) / ULONG_BIT] >> ((index) % ULONG_BIT)) & 1ul)
#define bitset_clear(self)        memset(self, 0, sizeof(self))
#define bitset_count(self)        popcount(self, sizeof(self))

int is_alphabet(int c);
int is_number(int c);
int is_space(int c);
int is_graphic(int c);

long utf8_len(const char *str, long len);

#define unreachable()                                                                            \
  do {                                                                                           \
    fprintf(stderr, "Internal Error: Entered unreachable region [%s:%d]\n", __FILE__, __LINE__); \
    exit(EXIT_FAILURE);                                                                          \
  } while (0)

#endif
