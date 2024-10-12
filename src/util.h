/* SPDX-License-Identifier: Apache-2.0 */

#ifndef UTIL_H
#define UTIL_H

#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Memory */

void *xmalloc(unsigned long size);

unsigned long popcount(const void *data, unsigned long size);

/* Hash */

typedef unsigned long Hash;

Hash hash_fnv1a(Hash *hash, const void *ptr, unsigned long len);

/* BitSet */

#define BitSet(bits)                                      \
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

#define Slice(type)      \
  struct {               \
    type         *ptr;   \
    unsigned long count; \
  }

#define slice_alloc(seq, new_count)                            \
  do {                                                         \
    /* NOLINTBEGIN(bugprone-sizeof-expression) */              \
    (seq)->ptr   = xmalloc(sizeof(*(seq)->ptr) * (new_count)); \
    (seq)->count = new_count;                                  \
    /* NOLINTEND(bugprone-sizeof-expression) */                \
  } while (0)

#define slice_free(seq) \
  do {                  \
    free((seq)->ptr);   \
  } while (0)

/* Vec */

#define Vec(type)           \
  struct {                  \
    type         *ptr;      \
    unsigned long count;    \
    unsigned long capacity; \
  }

#define vec_alloc(vec, new_count)   \
  do {                              \
    (vec)->ptr      = NULL;         \
    (vec)->capacity = 0;            \
    (vec)->count    = new_count;    \
    vec_reserve(vec, (vec)->count); \
  } while (0)

#define vec_free(vec) \
  do {                \
    slice_free(vec);  \
  } while (0)

#define vec_reserve(vec, new_capacity)                                    \
  do {                                                                    \
    /* NOLINTBEGIN(bugprone-sizeof-expression) */                         \
    unsigned long capacity = new_capacity;                                \
    if (capacity > (vec)->capacity) {                                     \
      void         *old_ptr = (vec)->ptr;                                 \
      unsigned long i;                                                    \
                                                                          \
      --capacity;                                                         \
      for (i = 1; i < sizeof(i) * CHAR_BIT; i <<= 1) {                    \
        capacity |= capacity >> i;                                        \
      }                                                                   \
      ++capacity;                                                         \
                                                                          \
      (vec)->capacity = capacity;                                         \
      (vec)->ptr      = xmalloc(sizeof(*(vec)->ptr) * ((vec)->capacity)); \
      if ((vec)->count) {                                                 \
        memcpy((vec)->ptr, old_ptr, sizeof(*(vec)->ptr) * (vec)->count);  \
      }                                                                   \
      free(old_ptr);                                                      \
      /* NOLINTEND(bugprone-sizeof-expression) */                         \
    }                                                                     \
  } while (0)

#define vec_push(vec, other_ptr, other_count)                                            \
  do {                                                                                   \
    vec_reserve(vec, (vec)->count + (other_count));                                      \
    memcpy((vec)->ptr + (vec)->count, (other_ptr), sizeof(*(vec)->ptr) * (other_count)); \
    (vec)->count += (other_count);                                                       \
  } while (0)

#define vec_pop(vec, delete_count)  \
  do {                              \
    (vec)->count -= (delete_count); \
  } while (0)

#define vec_clear(vec) \
  do {                 \
    (vec)->count = 0;  \
  } while (0)

/* Hopscotch */

#define HOPSCOTCH_BUCKET_SIZE (sizeof(unsigned long) * CHAR_BIT)

typedef unsigned long         HopscotchHash(const void *ptr);
typedef int                   HopscotchEq(const void *lhs, const void *rhs);
typedef struct HopscotchEntry HopscotchEntry;
typedef struct Hopscotch      Hopscotch;

struct HopscotchEntry {
  unsigned long bucket;
  unsigned long slot;
};

struct Hopscotch {
  unsigned long *hops;
  unsigned long  count;
  HopscotchHash *hash;
  HopscotchEq   *eq;
};

void hopscotch_alloc(Hopscotch *hopscotch, unsigned long base_count, HopscotchHash *hash, HopscotchEq *eq);
void hopscotch_free(Hopscotch *hopscotch);
void hopscotch_unchecked(const Hopscotch *hopscotch, const void *key, HopscotchEntry *entry);
int  hopscotch_entry(const Hopscotch *hopscotch, void *data, unsigned long size, const void *key, HopscotchEntry *entry);
int  hopscotch_next(const Hopscotch *hopscotch, HopscotchEntry *entry);
int  hopscotch_occupy(Hopscotch *hopscotch, void *data, unsigned long size, HopscotchEntry *entry);
int  hopscotch_release(Hopscotch *hopscotch, HopscotchEntry *entry);

/* HashMap */

typedef HopscotchEntry HashMapEntry;

#define HashMap(key_type, value_type) \
  struct {                            \
    Hopscotch metadata;               \
    struct {                          \
      key_type   key;                 \
      value_type value;               \
    }            *ptr;                \
    unsigned long count;              \
  }

#define HashSet(key_type) \
  struct {                \
    Hopscotch metadata;   \
    struct {              \
      key_type key;       \
    }            *ptr;    \
    unsigned long count;  \
  }

#define hashmap_alloc(map, hash, eq)                \
  do {                                              \
    hopscotch_alloc(&(map)->metadata, 0, hash, eq); \
    (map)->ptr   = NULL;                            \
    (map)->count = 0;                               \
  } while (0)

#define hashmap_free(map)             \
  do {                                \
    hopscotch_free(&(map)->metadata); \
    free((map)->ptr);                 \
  } while (0)

#define hashmap_reserve(map, new_capacity)                                                               \
  do {                                                                                                   \
    unsigned long capacity = new_capacity;                                                               \
    if (capacity > (map)->metadata.count) {                                                              \
      void         *ptr      = (map)->ptr;                                                               \
      Hopscotch     metadata = (map)->metadata;                                                          \
      unsigned long i;                                                                                   \
                                                                                                         \
      while (1) {                                                                                        \
        unsigned long hop      = 0;                                                                      \
        unsigned long sentinel = metadata.count + HOPSCOTCH_BUCKET_SIZE - 1;                             \
                                                                                                         \
        hopscotch_alloc(&(map)->metadata, capacity, metadata.hash, metadata.eq);                         \
        (map)->ptr = xmalloc(sizeof(*(map)->ptr) * ((map)->metadata.count + HOPSCOTCH_BUCKET_SIZE - 1)); \
                                                                                                         \
        if (metadata.count == 0) {                                                                       \
          break;                                                                                         \
        }                                                                                                \
                                                                                                         \
        for (i = 0; i < sentinel; ++i) {                                                                 \
          hop = (hop >> 1) | metadata.hops[i];                                                           \
          if (hop & 1ul) {                                                                               \
            const void    *kv = (char *) ptr + i * sizeof(*(map)->ptr);                                  \
            HopscotchEntry entry;                                                                        \
            hopscotch_unchecked(&(map)->metadata, kv, &entry);                                           \
            if (!hopscotch_occupy(&(map)->metadata, (map)->ptr, sizeof(*(map)->ptr), &entry)) {          \
              break;                                                                                     \
            }                                                                                            \
            memcpy((map)->ptr + (entry.bucket + entry.slot), kv, sizeof(*(map)->ptr));                   \
          }                                                                                              \
        }                                                                                                \
                                                                                                         \
        if (i == sentinel) {                                                                             \
          break;                                                                                         \
        }                                                                                                \
                                                                                                         \
        capacity *= 2;                                                                                   \
        hopscotch_free(&(map)->metadata);                                                                \
        free((map)->ptr);                                                                                \
      }                                                                                                  \
                                                                                                         \
      free(ptr);                                                                                         \
      hopscotch_free(&metadata);                                                                         \
    }                                                                                                    \
  } while (0);

#define hashmap_entry(map, key, entry) \
  hopscotch_entry(&(map)->metadata, (map)->ptr, sizeof(*(map)->ptr), key, entry)

#define hashmap_key(map, entry) \
  (assert((entry)->bucket < (map)->metadata.count && (entry)->slot < HOPSCOTCH_BUCKET_SIZE), &(map)->ptr[(entry)->bucket + (entry)->slot].key)

#define hashmap_value(map, entry) \
  (assert((entry)->bucket < (map)->metadata.count && (entry)->slot < HOPSCOTCH_BUCKET_SIZE), &(map)->ptr[(entry)->bucket + (entry)->slot].value)

#define hashmap_next(map, entry) \
  hopscotch_next(&(map)->metadata, entry)

#define hashmap_occupy(map, entry, new_key)                                                            \
  do {                                                                                                 \
    int status;                                                                                        \
    while (!(status = hopscotch_occupy(&(map)->metadata, (map)->ptr, sizeof(*(map)->ptr), (entry)))) { \
      hashmap_reserve(map, (map)->metadata.count ? (map)->metadata.count * 2 : 1);                     \
      hopscotch_unchecked(&(map)->metadata, (new_key), (entry));                                       \
    }                                                                                                  \
    if (status == 1) {                                                                                 \
      ++(map)->count;                                                                                  \
    }                                                                                                  \
    (map)->ptr[(entry)->bucket + (entry)->slot].key = *(new_key);                                      \
  } while (0)

#define hashmap_release(map, entry)                   \
  do {                                                \
    if (hopscotch_release(&(map)->metadata, entry)) { \
      --(map)->count;                                 \
    }                                                 \
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

#endif /* UTIL_H */
