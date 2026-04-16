/*
 * ds.h -- data structures
 *
 * SPDX-FileCopyrightText: 2026 Shota Minami
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef DS_H
#define DS_H

#include <assert.h>
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
  (assert((i) >= 0 && (i) < (v)->count), &(v)->data[(i)])

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
    memcpy(vec_at((v), (v)->count), (elem), sizeof(*elem)); \
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

typedef unsigned long (*ht_hash)(void *key);

typedef int (*ht_eq)(void *l, void *r);

typedef unsigned long ht_hop;

struct ht_entry {
  size_t bucket;
  size_t slot;
};

void *raw_ht_rehash(ht_hop **hop, unsigned long *mask, void *data, size_t size, ht_hash hash, ht_eq eq);

int raw_ht_entry(struct ht_entry *entry, ht_hop *hop, unsigned long mask, void *data, size_t size, ht_hash hash, ht_eq eq, void *key);

int raw_ht_next(struct ht_entry *entry, ht_hop *hop, unsigned long mask);

int raw_ht_occupy(struct ht_entry *entry, ht_hop *hop, unsigned long mask, void *data, size_t size);

int raw_ht_release(struct ht_entry *entry, ht_hop *hop);

#define hs(E) \
  struct { \
    ht_hop *hop; \
    unsigned long mask; \
    size_t count; \
    E *data; \
    ht_hash hash; \
    ht_eq eq; \
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
      (m)->data = raw_ht_rehash(&(m)->hop, &(m)->mask, (m)->data, sizeof(*(m)->data), (m)->hash, (m)->eq); \
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
    memset((m)->hop, 0, sizeof(ht_hop) * ((m)->mask + 1)); \
    (m)->count = 0; \
  } while (0)

#endif /* DS_H */
