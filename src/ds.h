/*
 * ds.h -- data structures
 *
 * SPDX-FileCopyrightText: 2026 Shota Minami
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef DS_H
#define DS_H

#include <assert.h>
#include <limits.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

/* vector */

void *raw_vec_reserve(void *data, size_t size, size_t *cap, size_t ncap);

#define vec(T) \
  struct { \
    size_t count; \
    size_t cap; \
    T *data; \
  }

#define vec_init(v) \
  do { \
    (v)->count = 0; \
    (v)->cap = 0; \
    (v)->data = NULL; \
  } while (0)

#define vec_deinit(v) \
  do { \
    free((v)->data); \
  } while (0)

#define vec_at(v, i) \
  (assert((size_t) (i) < (v)->count), &(v)->data[(i)])

#define vec_front(v) \
  (assert((v)->count > 0), &(v)->data[0])

#define vec_back(v) \
  (assert((v)->count > 0), &(v)->data[(v)->count - 1])

#define vec_reserve(v, ncap) \
  do { \
    (v)->data = raw_vec_reserve((v)->data, sizeof(*(v)->data), &(v)->cap, (ncap)); \
  } while (0)

#define vec_push(v, elem) \
  do { \
    if ((v)->count == (v)->cap) { \
      vec_reserve((v), (v)->cap ? (v)->cap << 1 : 1); \
    } \
    memcpy(&(v)->data[(v)->count], (elem), sizeof(*elem)); \
    ++(v)->count; \
  } while (0)

#define vec_pop(v) \
  do { \
    assert((v)->count > 0); \
    --(v)->count; \
  } while (0)

#define vec_clear(v) \
  do { \
    (v)->count = 0; \
  } while (0)

/* hash table */

typedef unsigned long hash_t;

#if ULONG_MAX >= 0xffffffffffffffffUL

#define hash_init(h) \
  do { \
    *(h) = 0xcbf29ce484222325UL; \
  } while (0)

#define hash_add(h, data, size) \
  do { \
    size_t i; \
    for (i = 0; i < (size); ++i) { \
      *(h) ^= (unsigned char) ((char *) (data))[(i)]; \
      *(h) *= 0x100000001b3UL; \
    } \
  } while (0)

#else

#define hash_init(h) \
  do { \
    *(h) = 0x811c9dc5UL; \
  } while (0)

#define hash_add(h, data, size) \
  do { \
    size_t i; \
    for (i = 0; i < (size); ++i) { \
      *(h) ^= (unsigned char) ((char *) (data))[(i)]; \
      *(h) *= 0x1000193UL; \
    } \
  } while (0)

#endif

typedef hash_t (*ht_hash_fn_t)(void const *key);

typedef int (*ht_eq_fn_t)(void const *l, void const *r);

struct ht_entry {
  size_t bucket;
  size_t slot;
};

void *raw_ht_rehash(unsigned long **hop, unsigned long *mask, void *data, size_t size, ht_hash_fn_t hash);

int raw_ht_entry(struct ht_entry *entry, unsigned long *hop, unsigned long mask, void *data, size_t size, ht_hash_fn_t hash, ht_eq_fn_t eq, void const *key);

int raw_ht_next(struct ht_entry *entry, unsigned long *hop, unsigned long mask);

int raw_ht_occupy(struct ht_entry *entry, unsigned long *hop, unsigned long mask, void *data, size_t size);

int raw_ht_release(struct ht_entry *entry, unsigned long *hop);

#define hs(E) \
  struct { \
    unsigned long *hop; \
    unsigned long mask; \
    size_t count; \
    E *data; \
    ht_hash_fn_t hash; \
    ht_eq_fn_t eq; \
  }

#define hm(K, V) \
  hs(struct { K const key; V value; })

#define ht_init(m, hash_fn, eq_fn) \
  do { \
    (m)->hop = NULL; \
    (m)->mask = 0; \
    (m)->count = 0; \
    (m)->data = NULL; \
    (m)->hash = hash_fn; \
    (m)->eq = eq_fn; \
  } while (0)

#define ht_deinit(m) \
  do { \
    free((m)->hop); \
    free((m)->data); \
  } while (0)

#define ht_at(m, entry) \
  (assert((entry)->bucket + (entry)->slot <= (m)->mask), &(m)->data[((entry)->bucket + (entry)->slot) & (m)->mask])

#define ht_entry(m, elem, entry) \
  raw_ht_entry((entry), (m)->hop, (m)->mask, (m)->data, sizeof(*(m)->data), (m)->hash, (m)->eq, (elem))

#define ht_next(m, entry) \
  raw_ht_next((entry), (m)->hop, (m)->mask)

#define ht_occupy(m, entry, elem) \
  do { \
    while (!raw_ht_occupy((entry), (m)->hop, (m)->mask, (m)->data, sizeof(*(m)->data))) { \
      (m)->data = raw_ht_rehash(&(m)->hop, &(m)->mask, (m)->data, sizeof(*(m)->data), (m)->hash); \
      ht_entry((m), (elem), (entry)); \
    } \
    ++(m)->count; \
    memcpy(ht_at((m), (entry)), (elem), sizeof(*elem)); \
  } while (0)

#define ht_release(m, entry) \
  do { \
    if (raw_ht_release((entry), (m)->hop)) { \
      --(m)->count; \
    } \
  } while (0)

#define ht_clear(m) \
  do { \
    memset((m)->hop, 0, sizeof(unsigned long) * ((m)->mask + 1)); \
    (m)->count = 0; \
  } while (0)

/* bits */

typedef unsigned long bits_t;

#define bits(N) \
  struct { \
    bits_t data[((N) + sizeof(bits_t) * CHAR_BIT - 1) / (sizeof(bits_t) * CHAR_BIT)]; \
  }

#define bits_bucket(i) ((i) / (sizeof(bits_t) * CHAR_BIT))

#define bits_slot(i) ((i) % (sizeof(bits_t) * CHAR_BIT))

#define bits_set(bits, i, value) \
  do { \
    ((bits_t *) (bits))[bits_bucket(i)] = (((bits_t *) (bits))[bits_bucket(i)] & ~((bits_t) 1 << bits_slot(i))) | ((bits_t) !!(value) << bits_slot(i)); \
  } while (0)

#define bits_test(bits, i) \
  ((((bits_t *) (bits))[bits_bucket(i)] >> bits_slot(i)) & 1)

/* fenwick tree */

void fw_build(size_t *tree, size_t n);

void fw_update(size_t *tree, size_t n, size_t i, size_t delta);

size_t fw_query(size_t *tree, size_t i);

#endif /* DS_H */
