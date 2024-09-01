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

/* Seq */

#define Seq(type)        \
  struct {               \
    type         *ptr;   \
    unsigned long count; \
  }

typedef Seq(char) CharSeq;
typedef Seq(unsigned char) UCharSeq;
typedef Seq(int) IntSeq;
typedef Seq(unsigned int) UIntSeq;
typedef Seq(long) LongSeq;
typedef Seq(unsigned long) ULongSeq;

#define seq_alloc(seq, new_count)                                      \
  do {                                                                 \
    (seq)->ptr   = xmalloc(nolint(sizeof(*(seq)->ptr)) * (new_count)); \
    (seq)->count = new_count;                                          \
  } while (0)

#define seq_free(seq) \
  do {                \
    free((seq)->ptr); \
  } while (0)

#define seq_splice(seq, offset, span, other_ptr, other_count) \
  do {                                                        \
    void *old_ptr = (seq)->ptr;                               \
    (seq)->ptr    = splice_alloc(                             \
      old_ptr, (seq)->count,                               \
      nolint(sizeof(*(seq)->ptr)), offset, span,           \
      other_ptr, other_count, &(seq)->count);              \
    free(old_ptr);                                            \
  } while (0)

/* Vec */

#define Vec(type)        \
  struct {               \
    type         *ptr;   \
    unsigned long count; \
    unsigned long used;  \
  }

typedef Vec(char) CharVec;
typedef Vec(unsigned char) UCharVec;
typedef Vec(int) IntVec;
typedef Vec(unsigned int) UIntVec;
typedef Vec(long) LongVec;
typedef Vec(unsigned long) ULongVec;

#define vec_alloc(vec, new_count)    \
  do {                               \
    unsigned long count = new_count; \
    (vec)->ptr          = NULL;      \
    (vec)->count        = 0;         \
    (vec)->used         = count;     \
    vec_reserve(vec, count);         \
  } while (0)

#define vec_free(vec) \
  do {                \
    seq_free(vec);    \
  } while (0)

#define vec_reserve(vec, new_capacity)                  \
  do {                                                  \
    unsigned long capacity = new_capacity;              \
    if (capacity > (vec)->count) {                      \
      unsigned long i;                                  \
      for (i = 1; i < sizeof(i) * CHAR_BIT; i <<= 1) {  \
        capacity |= capacity >> i;                      \
      }                                                 \
      ++capacity;                                       \
      seq_splice(vec, (vec)->count, 0, NULL, capacity); \
    }                                                   \
  } while (0)

#define vec_splice(vec, offset, span, other_ptr, other_count) \
  do {                                                        \
    vec_reserve(vec, (vec)->used + (other_count) - (span));   \
    (vec)->used = splice(                                     \
      (vec)->ptr, (vec)->count,                               \
      NULL, (vec)->used,                                      \
      nolint(sizeof(*(vec)->ptr)), offset, span,              \
      other_ptr, other_count);                                \
  } while (0)

#define vec_push(vec, other_ptr, other_count) \
  vec_splice(vec, (vec)->used, 0, other_ptr, other_count)

#define vec_pop(vec, count) \
  do {                      \
    (vec)->used -= (count); \
  } while (0)

#define vec_clear(vec) \
  do {                 \
    (vec)->used = 0;   \
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

#define nolint(x) /* NOLINTBEGIN rules */ x /* NOLINTEND */

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
