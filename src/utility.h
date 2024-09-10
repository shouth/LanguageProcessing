/* SPDX-License-Identifier: Apache-2.0 */

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

Hash hash_fnv1a(Hash *hash, const void *ptr, unsigned long len);

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

#define Seq(type)       \
  struct {              \
    type         *ptr;  \
    unsigned long span; \
  }

#define seq_alloc(seq, new_count)                             \
  do {                                                        \
    /* NOLINTBEGIN(bugprone-sizeof-expression) */             \
    (seq)->ptr  = xmalloc(sizeof(*(seq)->ptr) * (new_count)); \
    (seq)->span = new_count;                                  \
    /* NOLINTEND(bugprone-sizeof-expression) */               \
  } while (0)

#define seq_free(seq) \
  do {                \
    free((seq)->ptr); \
  } while (0)

#define seq_splice(seq, offset, delete_span, other_ptr, other_count) \
  do {                                                               \
    /* NOLINTBEGIN(bugprone-sizeof-expression) */                    \
    void *old_ptr = (seq)->ptr;                                      \
    (seq)->ptr    = splice_alloc(                                    \
      old_ptr, (seq)->span,                                       \
      sizeof(*(seq)->ptr), offset, delete_span,                   \
      other_ptr, other_count, &(seq)->span);                      \
    free(old_ptr);                                                   \
    /* NOLINTEND(bugprone-sizeof-expression) */                      \
  } while (0)

/* Vec */

#define Vec(type)        \
  struct {               \
    type         *ptr;   \
    unsigned long span;  \
    unsigned long count; \
  }

#define vec_alloc(vec, new_count)   \
  do {                              \
    (vec)->ptr   = NULL;            \
    (vec)->span  = 0;               \
    (vec)->count = new_count;       \
    vec_reserve(vec, (vec)->count); \
  } while (0)

#define vec_free(vec) \
  do {                \
    seq_free(vec);    \
  } while (0)

#define vec_reserve(vec, new_capacity)                 \
  do {                                                 \
    unsigned long capacity = new_capacity;             \
    if (capacity > (vec)->span) {                      \
      unsigned long i;                                 \
      --capacity;                                      \
      for (i = 1; i < sizeof(i) * CHAR_BIT; i <<= 1) { \
        capacity |= capacity >> i;                     \
      }                                                \
      ++capacity;                                      \
      seq_splice(vec, (vec)->span, 0, NULL, capacity); \
    }                                                  \
  } while (0)

#define vec_splice(vec, offset, delete_span, other_ptr, other_count) \
  do {                                                               \
    /* NOLINTBEGIN(bugprone-sizeof-expression) */                    \
    vec_reserve(vec, (vec)->count + (other_count) - (delete_span));  \
    (vec)->count = splice(                                           \
      (vec)->ptr, (vec)->span,                                       \
      NULL, (vec)->count,                                            \
      sizeof(*(vec)->ptr), offset, delete_span,                      \
      other_ptr, other_count);                                       \
    /* NOLINTEND(bugprone-sizeof-expression) */                      \
  } while (0)

#define vec_push(vec, other_ptr, other_count) \
  vec_splice(vec, (vec)->count, 0, other_ptr, other_count)

#define vec_pop(vec, delete_count)  \
  do {                              \
    (vec)->count -= (delete_count); \
  } while (0)

#define vec_clear(vec) \
  do {                 \
    (vec)->count = 0;  \
  } while (0)

/* Hopscotch */

typedef unsigned long         HopscotchHash(const void *ptr);
typedef int                   HopscotchEq(const void *lhs, const void *rhs);
typedef struct HopscotchEntry HopscotchEntry;
typedef struct Hopscotch      Hopscotch;

struct HopscotchEntry {
  unsigned long bucket;
  unsigned long slot;
};

struct Hopscotch {
  Seq(unsigned long) hops;
  HopscotchHash *hash;
  HopscotchEq   *eq;
};

void hopscotch_alloc(Hopscotch *hopscotch, unsigned long span, HopscotchHash *hash, HopscotchEq *eq);
void hopscotch_free(Hopscotch *hopscotch);
void hopscotch_unchecked(Hopscotch *hopscotch, const void *key, HopscotchEntry *entry);
int  hopscotch_entry(Hopscotch *hopscotch, void *data, unsigned long size, const void *key, HopscotchEntry *entry);
int  hopscotch_next(Hopscotch *hopscotch, HopscotchEntry *entry);
int  hopscotch_occupy(Hopscotch *hopscotch, void *data, unsigned long size, HopscotchEntry *entry);
void hopscotch_release(Hopscotch *hopscotch, HopscotchEntry *entry);

/* HashMap */

typedef HopscotchEntry HashMapEntry;

#define HashMap(key_type, value_type) \
  struct {                            \
    Hopscotch metadata;               \
    struct {                          \
      union {                         \
        key_type mutable;             \
        key_type const readonly;      \
      } key;                          \
      value_type value;               \
    }            *ptr;                \
    unsigned long span;               \
    unsigned long count;              \
  }

#define hashmap_alloc(map, hash, eq)                \
  do {                                              \
    hopscotch_alloc(&(map)->metadata, 0, hash, eq); \
    seq_alloc(map, 0);                              \
    (map)->count = 0;                               \
  } while (0)

#define hashmap_free(map)             \
  do {                                \
    hopscotch_free(&(map)->metadata); \
    seq_free(map);                    \
  } while (0)

#define hashmap_reserve(map, new_capacity)                                                      \
  do {                                                                                          \
    unsigned long capacity = new_capacity;                                                      \
    if (capacity + sizeof(unsigned long) * CHAR_BIT - 1 > (map)->span) {                        \
      void         *old_ptr  = (map)->ptr;                                                      \
      unsigned long old_span = (map)->span;                                                     \
      Hopscotch     metadata = (map)->metadata;                                                 \
      int           finished = 0;                                                               \
      unsigned long i, hop;                                                                     \
                                                                                                \
      --capacity;                                                                               \
      for (i = 1; i < sizeof(i) * CHAR_BIT; i <<= 1) {                                          \
        capacity |= capacity >> i;                                                              \
      }                                                                                         \
      ++capacity;                                                                               \
                                                                                                \
      while (!finished) {                                                                       \
        hopscotch_alloc(&(map)->metadata, capacity, metadata.hash, metadata.eq);                \
        seq_alloc(map, capacity + sizeof(unsigned long) * CHAR_BIT - 1);                        \
                                                                                                \
        finished = 1;                                                                           \
        hop      = 0;                                                                           \
        for (i = 0; i < old_span; ++i) {                                                        \
          hop >>= 1;                                                                            \
          if (i < metadata.hops.span) {                                                         \
            hop |= metadata.hops.ptr[i];                                                        \
          }                                                                                     \
          if (hop & 1ul) {                                                                      \
            const void    *kv = (char *) old_ptr + i * sizeof(*(map)->ptr);                     \
            HopscotchEntry entry;                                                               \
            hopscotch_unchecked(&(map)->metadata, kv, &entry);                                  \
            if (!hopscotch_occupy(&(map)->metadata, (map)->ptr, sizeof(*(map)->ptr), &entry)) { \
              finished = 0;                                                                     \
              capacity *= 2;                                                                    \
              hopscotch_free(&(map)->metadata);                                                 \
              seq_free(map);                                                                    \
              break;                                                                            \
            }                                                                                   \
            memcpy((map)->ptr + (entry.bucket + entry.slot), kv, sizeof(*(map)->ptr));          \
          }                                                                                     \
        }                                                                                       \
      }                                                                                         \
                                                                                                \
      free(old_ptr);                                                                            \
      hopscotch_free(&metadata);                                                                \
    }                                                                                           \
  } while (0);

#define hashmap_entry(map, key, entry)                                                 \
  (key                                                                                 \
      ? hopscotch_entry(&(map)->metadata, (map)->ptr, sizeof(*(map)->ptr), key, entry) \
      : (hopscotch_unchecked(&(map)->metadata, NULL, entry), 0))

#define hashmap_key(map, entry)                                                      \
  ((entry)->bucket < (map)->span && (entry)->slot < sizeof(unsigned long) * CHAR_BIT \
      ? &(map)->ptr[(entry)->bucket + (entry)->slot].key.readonly                    \
      : NULL)

#define hashmap_value(map, entry)                                                    \
  ((entry)->bucket < (map)->span && (entry)->slot < sizeof(unsigned long) * CHAR_BIT \
      ? &(map)->ptr[(entry)->bucket + (entry)->slot].value                           \
      : NULL)

#define hashmap_next(map, entry) \
  hopscotch_next(&(map)->metadata, entry)

#define hashmap_update(map, entry, new_key, new_value)                                                 \
  do {                                                                                                 \
    int status;                                                                                        \
    while (!(status = hopscotch_occupy(&(map)->metadata, (map)->ptr, sizeof(*(map)->ptr), (entry)))) { \
      hashmap_reserve(map, (map)->metadata.hops.span ? (map)->metadata.hops.span * 2 : 1);             \
      hopscotch_unchecked(&(map)->metadata, (new_key), (entry));                                       \
    }                                                                                                  \
    if (status == 1) {                                                                                 \
      ++(map)->count;                                                                                  \
    }                                                                                                  \
    (map)->ptr[(entry)->bucket + (entry)->slot].key.mutable = *(new_key);                              \
    (map)->ptr[(entry)->bucket + (entry)->slot].value       = *(new_value);                            \
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
