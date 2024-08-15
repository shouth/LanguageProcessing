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

#ifndef UTILITY_H
#define UTILITY_H

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

void *xmalloc(unsigned long size);
void *memdup(const void *ptr, unsigned long length);
char *strndup(const char *src, unsigned long length);

#define FNV1A_INIT 0x811C9DC5ul

unsigned long fnv1a(unsigned long hash, const void *ptr, unsigned long len);

unsigned long popcount(void *data, unsigned long size);

#define ULONG_BIT (sizeof(unsigned long) * CHAR_BIT)

#define BITSET(name, bits)                                  \
  struct {                                                  \
    unsigned long data[(bits + ULONG_BIT - 1) / ULONG_BIT]; \
  } name

#define bitset_data(self)         ((unsigned long *) (self))
#define bitset_set(self, index)   (bitset_data(self)[(index) / ULONG_BIT] |= 1ul << ((index) % ULONG_BIT))
#define bitset_reset(self, index) (bitset_data(self)[(index) / ULONG_BIT] &= ~(1ul << ((index) % ULONG_BIT)))
#define bitset_get(self, index)   (bitset_data(self)[(index) / ULONG_BIT] >> ((index) % ULONG_BIT)) & 1ul)
#define bitset_clear(self) memset(bitset_data(self), 0, sizeof(*(self)))
#define bitset_count(self) popcount(bitset_data(self), sizeof(*(self)))

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

#define META_SWALLOW(x)
#define META_EXPAND(x)   x
#define META_DEFER(x)    x META_SWALLOW()
#define META_OBSTRUCT(x) x META_DEFER(META_SWALLOW)()

#define META_TRUE(p, q)  p
#define META_FALSE(p, q) q
#define META_AND(p, q)   p(q, META_FALSE)
#define META_OR(p, q)    p(META_TRUE, q)
#define META_NOT(p)      p(META_FALSE, META_TRUE)

#define META_IF(p, x, y) p(x, y)

#define META_DETECT(x) META_EXPAND(META_EXPAND(META_FALSE META_DEFER(META_SWALLOW)(x)))
#define META_DETECT_PROBE )(?, META_TRUE

#endif
