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
#include <string.h>

/* Memory */

void *xmalloc(unsigned long size);
void *memdup(const void *ptr, unsigned long length);
char *strndup(const char *src, unsigned long length);

unsigned long splice(
  void *dest, unsigned long dest_count,
  const void *src1, unsigned long src1_count,
  unsigned long size, unsigned long offset, unsigned long span,
  const void *src2, unsigned long src2_count);

void *splice_alloc(
  const void *src1, unsigned long src1_count,
  unsigned long size, unsigned long offset, unsigned long span,
  const void *src2, unsigned long src2_count,
  unsigned long *result_count);

unsigned long popcount(const void *data, unsigned long size);

/* Hash */

typedef unsigned long Hash;

#define HASH_FNV1A_INIT 0x811C9DC5ul

void hash_fnv1a(Hash *hash, const void *ptr, unsigned long len);

/* BitSet */

#define BITSET(bits)                                      \
  struct {                                                \
    unsigned char data[(bits + CHAR_BIT - 1) / CHAR_BIT]; \
  }

#define bitset_zero() \
  { 0 }

#define bitset_set(self, index) \
  ((self)->data[(index) / CHAR_BIT] |= 1ul << ((index) % CHAR_BIT))

#define bitset_reset(self, index) \
  ((self)->data[(index) / CHAR_BIT] &= ~(1ul << ((index) % CHAR_BIT)))

#define bitset_get(self, index) \
  ((self)->data[(index) / CHAR_BIT] & (1ul << ((index) % CHAR_BIT)))

#define bitset_clear(self) \
  memset((self)->data, 0, sizeof((self)->data))

#define bitset_count(self) \
  popcount((self)->data, sizeof((self)->data))

#define bitset_insert(self, other)                   \
  do {                                               \
    unsigned long        i;                          \
    unsigned char       *self_data  = (self)->data;  \
    const unsigned char *other_data = (other)->data; \
    for (i = 0; i < count_of((self)->data); ++i) {   \
      self_data[i] |= other_data[i];                 \
    }                                                \
  } while (0)
#define bitset_erase(self, other)                    \
  do {                                               \
    unsigned long        i;                          \
    unsigned char       *self_data  = (self)->data;  \
    const unsigned char *other_data = (other)->data; \
    for (i = 0; i < count_of((self)->data); ++i) {   \
      self_data[i] &= ~other_data[i];                \
    }                                                \
  } while (0)

/* Slice */

#define SLICE(type)      \
  struct {               \
    type         *ptr;   \
    unsigned long count; \
  }

#define slice_alloc(slice, new_count)                              \
  do {                                                             \
    (slice)->ptr   = xmalloc(sizeof(*(slice)->ptr) * (new_count)); \
    (slice)->count = new_count;                                    \
  } while (0)

#define slice_free(slice) \
  do {                    \
    free((slice)->ptr);   \
  } while (0)

#define slice_splice(slice, offset, span, other_ptr, other_count) \
  do {                                                            \
    void *old_ptr = (slice)->ptr;                                 \
    (slice)->ptr  = splice_alloc(                                 \
      old_ptr, (slice)->count,                                   \
      sizeof(*(slice)->ptr), offset, span,                       \
      other_ptr, other_count, &(slice)->count);                  \
    free(old_ptr);                                                \
  } while (0)

/* Buffer */

#define BUFFER(type)     \
  struct {               \
    type         *ptr;   \
    unsigned long count; \
    unsigned long used;  \
  }

#define buffer_alloc(buffer, capacity) \
  do {                                 \
    slice_alloc(buffer, capacity);     \
    (buffer)->used = 0;                \
  } while (0)

#define buffer_free(buffer)      \
  do {                           \
    slice_free(&(buffer)->data); \
  } while (0)

#define buffer_splice(buffer, offset, span, other_ptr, other_count)    \
  do {                                                                 \
    unsigned long new_count = (buffer)->used + (other_count) - (span); \
    if (new_count > (buffer)->count) {                                 \
      unsigned long i;                                                 \
      unsigned long new_capacity = new_count;                          \
      for (i = 1; i < sizeof(i) * CHAR_BIT; i <<= 1) {                 \
        new_capacity |= new_capacity >> i;                             \
      }                                                                \
      ++new_capacity;                                                  \
      slice_splice(buffer, (buffer)->count, 0, NULL, new_capacity);    \
    }                                                                  \
    splice(                                                            \
      (buffer)->ptr, (buffer)->count,                                  \
      NULL, (buffer)->used,                                            \
      sizeof(*(buffer)->ptr), offset, span,                            \
      other_ptr, other_count);                                         \
    (buffer)->used = new_count;                                        \
  } while (0)

#define buffer_push(buffer, other_ptr, other_count) \
  buffer_splice(buffer, (buffer)->used, 0, other_ptr, other_count)

#define buffer_pop(buffer, count) \
  do {                            \
    (buffer)->used -= (count);    \
  } while (0)

#define buffer_clear(buffer) \
  do {                       \
    (buffer)->used = 0;      \
  } while (0)

/* Character */

int is_alphabet(int c);
int is_number(int c);
int is_space(int c);
int is_graphic(int c);

long utf8_len(const char *str, long len);

/* Color */

#define MONOKAI_RED    0xFF6188
#define MONOKAI_GREEN  0xA9DC76
#define MONOKAI_YELLOW 0xFFD866
#define MONOKAI_BLUE   0x78DCE8
#define MONOKAI_PURPLE 0xAB9DF2

/* Meta programming */

#define META_EMPTY()
#define META_SWALLOW(x)
#define META_EXPAND(x)   x
#define META_DEFER(x)    x META_EMPTY()
#define META_OBSTRUCT(x) x META_DEFER(META_EMPTY)()

#define META_TRUE(p, q)  p
#define META_FALSE(p, q) q
#define META_AND(p, q)   p(q, META_FALSE)
#define META_OR(p, q)    p(META_TRUE, q)
#define META_NOT(p)      p(META_FALSE, META_TRUE)

#define META_DETECT(x) META_EXPAND(META_EXPAND(META_FALSE META_DEFER(META_SWALLOW)(x)))
#define META_DETECT_PROBE )(?, META_TRUE

/* Miscellaneous */

#define count_of(x) ((sizeof(x) / sizeof(0 [x])) / ((size_t) (!(sizeof(x) % sizeof(0 [x])))))

#if defined(__GNUC__) || defined(__clang__)

#define format(archetype, string_index, first_to_check) \
  __attribute__((format(archetype, string_index, first_to_check)))

#else

#define format(archetype, string_index, first_to_check)

#endif

#define unreachable()                                                                            \
  do {                                                                                           \
    fprintf(stderr, "Internal Error: Entered unreachable region [%s:%d]\n", __FILE__, __LINE__); \
    exit(EXIT_FAILURE);                                                                          \
  } while (0)

#endif
