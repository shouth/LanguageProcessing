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

#define slice_alloc(self, new_count)                             \
  do {                                                           \
    (self)->ptr   = xmalloc(sizeof(*(self)->ptr) * (new_count)); \
    (self)->count = new_count;                                   \
  } while (0)

#define slice_free(self) \
  do {                   \
    free((self)->ptr);   \
  } while (0)

/* Vec */

#define Vec(type)           \
  struct {                  \
    type         *ptr;      \
    unsigned long count;    \
    unsigned long capacity; \
  }

#define vec_alloc(self, new_count)    \
  do {                                \
    (self)->ptr      = NULL;          \
    (self)->capacity = 0;             \
    (self)->count    = new_count;     \
    vec_reserve(self, (self)->count); \
  } while (0)

#define vec_free(self) \
  do {                 \
    slice_free(self);  \
  } while (0)

#define vec_reserve(self, new_capacity)                                                                \
  do {                                                                                                 \
    extern void  *vec_reserve_impl(void *, unsigned long, unsigned long, unsigned long);               \
    unsigned long capacity = (new_capacity);                                                           \
    if (capacity > (self)->capacity) {                                                                 \
      (self)->ptr      = vec_reserve_impl((self)->ptr, sizeof(*(self)->ptr), (self)->count, capacity); \
      (self)->capacity = capacity;                                                                     \
    }                                                                                                  \
  } while (0)

#define vec_push(self, other_ptr, other_count)                                              \
  do {                                                                                      \
    vec_reserve(self, (self)->count + (other_count));                                       \
    memcpy((self)->ptr + (self)->count, (other_ptr), sizeof(*(self)->ptr) * (other_count)); \
    (self)->count += (other_count);                                                         \
  } while (0)

#define vec_pop(self, delete_count)  \
  do {                               \
    (self)->count -= (delete_count); \
  } while (0)

#define vec_clear(self) \
  do {                  \
    (self)->count = 0;  \
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

#define hashmap_alloc(self, hash, eq)                \
  do {                                               \
    hopscotch_alloc(&(self)->metadata, 0, hash, eq); \
    (self)->ptr   = NULL;                            \
    (self)->count = 0;                               \
  } while (0)

#define hashmap_free(self)             \
  do {                                 \
    hopscotch_free(&(self)->metadata); \
    free((self)->ptr);                 \
  } while (0)

#define hashmap_reserve(self, new_capacity)                                                               \
  do {                                                                                                    \
    extern void  *hashmap_reserve_impl(void *, unsigned long, Hopscotch *, unsigned long);                \
    unsigned long capacity = (new_capacity);                                                              \
    if (capacity > (self)->metadata.count) {                                                              \
      (self)->ptr = hashmap_reserve_impl((self)->ptr, sizeof(*(self)->ptr), &(self)->metadata, capacity); \
    }                                                                                                     \
  } while (0)

#define hashmap_entry(self, key, entry) \
  hopscotch_entry(&(self)->metadata, (self)->ptr, sizeof(*(self)->ptr), key, entry)

#define hashmap_key(self, entry) \
  (assert((entry)->bucket < (self)->metadata.count && (entry)->slot < HOPSCOTCH_BUCKET_SIZE), &(self)->ptr[(entry)->bucket + (entry)->slot].key)

#define hashmap_value(self, entry) \
  (assert((entry)->bucket < (self)->metadata.count && (entry)->slot < HOPSCOTCH_BUCKET_SIZE), &(self)->ptr[(entry)->bucket + (entry)->slot].value)

#define hashmap_next(self, entry) \
  hopscotch_next(&(self)->metadata, entry)

#define hashmap_occupy(self, entry, new_key)                                                              \
  do {                                                                                                    \
    int status;                                                                                           \
    while (!(status = hopscotch_occupy(&(self)->metadata, (self)->ptr, sizeof(*(self)->ptr), (entry)))) { \
      hashmap_reserve(self, (self)->metadata.count ? (self)->metadata.count * 2 : 1);                     \
      hopscotch_unchecked(&(self)->metadata, (new_key), (entry));                                         \
    }                                                                                                     \
    if (status == 1) {                                                                                    \
      ++(self)->count;                                                                                    \
    }                                                                                                     \
    (self)->ptr[(entry)->bucket + (entry)->slot].key = *(new_key);                                        \
  } while (0)

#define hashmap_release(self, entry)                   \
  do {                                                 \
    if (hopscotch_release(&(self)->metadata, entry)) { \
      --(self)->count;                                 \
    }                                                  \
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
