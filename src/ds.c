/*
 * ds.c -- data structures
 *
 * SPDX-FileCopyrightText: 2026 Shota Minami
 * SPDX-License-Identifier: Apache-2.0
 */

#include <limits.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "ds.h"

/* vector */

void *raw_vec_reserve(void *data, size_t size, size_t *cap, size_t ncap)
{
  if (ncap > *cap) {
    data = realloc(data, size * ncap);
    *cap = ncap;
  }
  return data;
}

/* hash map */

void *raw_ht_rehash(unsigned long **hop, unsigned long *mask, void *data, size_t size, ht_hash_fn_t hash)
{
  void *ndata;
  unsigned long *nhop;
  unsigned long nmask = *mask;
  size_t i;

  while (1) {
    nmask = nmask << 1 | 1;
    ndata = calloc(nmask + 1, size);
    nhop = calloc(nmask + 1, sizeof(unsigned long));

    if (*mask) {
      unsigned long window = *mask < sizeof(unsigned long) * CHAR_BIT ? *mask : sizeof(unsigned long) * CHAR_BIT;
      unsigned long occupied = 0;

      for (i = window - 1; i > 0; --i) {
        occupied = occupied >> 1 | (*hop)[-i & *mask];
      }

      for (i = 0; i <= *mask; ++i) {
        occupied = occupied >> 1 | (*hop)[i];
        if (occupied & 1) {
          void *item = (char *) data + size * i;
          struct ht_entry e;
          e.bucket = hash(item) & nmask;
          e.slot = -1UL;
          if (!raw_ht_occupy(&e, nhop, nmask, ndata, size)) {
            goto fail;
          }
          memcpy((char *) ndata + size * ((e.bucket + e.slot) & nmask), item, size);
        }
      }
    }

    free(data);
    free(*hop);
    *mask = nmask;
    *hop = nhop;
    return ndata;

  fail:
    free(ndata);
    free(nhop);
  }
}

int raw_ht_entry(struct ht_entry *entry, unsigned long *hop, unsigned long mask, void *data, size_t size, ht_hash_fn_t hash, ht_eq_fn_t eq, void const *elem)
{
  if (elem && mask) {
    entry->bucket = hash(elem) & mask;
    for (entry->slot = 0; entry->slot < sizeof(unsigned long) * CHAR_BIT; ++entry->slot) {
      if (hop[entry->bucket] & (1UL << entry->slot)) {
        if (eq(elem, (char *) data + size * ((entry->bucket + entry->slot) & mask))) {
          return 1;
        }
      }
    }
  } else {
    entry->bucket = 0;
  }

  entry->slot = -1UL;
  return 0;
}

int raw_ht_next(struct ht_entry *entry, unsigned long *hop, unsigned long mask)
{
  if (mask) {
    ++entry->slot;
    for (; entry->bucket <= mask; ++entry->bucket) {
      for (; entry->slot < sizeof(unsigned long) * CHAR_BIT; ++entry->slot) {
        if (hop[entry->bucket] & (1UL << entry->slot)) {
          return 1;
        }
      }
      entry->slot = 0;
    }
    entry->slot = -1UL;
  }
  return 0;
}

int raw_ht_occupy(struct ht_entry *entry, unsigned long *hop, unsigned long mask, void *data, size_t size)
{
  unsigned long window = mask < sizeof(unsigned long) * CHAR_BIT ? mask : sizeof(unsigned long) * CHAR_BIT;
  unsigned long limit = mask < sizeof(unsigned long) * CHAR_BIT * 8 ? mask : sizeof(unsigned long) * CHAR_BIT * 8;
  unsigned long occupied = 0;
  unsigned long empty = -1UL;
  unsigned long i;

  if (!mask) {
    return 0;
  }

  if (entry->slot != -1UL) {
    return 1;
  }

  for (i = window - 1; i > 0; --i) {
    occupied = occupied >> 1 | hop[(entry->bucket - i) & mask];
  }

  for (i = 0; i < limit; ++i) {
    occupied = occupied >> 1 | hop[(entry->bucket + i) & mask];
    if (!(occupied & 1)) {
      empty = i;
      break;
    }
  }

  if (empty == -1UL) {
    return 0;
  }

  while (empty >= sizeof(unsigned long) * CHAR_BIT) {
    unsigned long next = -1UL;
    for (i = empty - window + 1; i < empty; ++i) {
      unsigned long b = hop[(entry->bucket + i) & mask] & -hop[(entry->bucket + i) & mask];
      if (b && b < (1UL << (empty - i))) {
        next = i;
        while (b >>= 1) {
          ++next;
        }
        break;
      }
    }

    if (next == -1UL) {
      return 0;
    }

    memcpy((char *) data + size * ((entry->bucket + empty) & mask), (char *) data + size * ((entry->bucket + next) & mask), size);
    hop[(entry->bucket + i) & mask] &= ~(1UL << (next - i));
    hop[(entry->bucket + i) & mask] |= 1UL << (empty - i);
    empty = next;
  }

  entry->slot = empty;
  hop[entry->bucket] |= 1UL << empty;
  return 1;
}

int raw_ht_release(struct ht_entry *entry, unsigned long *hop)
{
  if (entry->slot != -1UL) {
    hop[entry->bucket] &= ~(1UL << entry->slot);
    return 1;
  } else {
    return 0;
  }
}

/* fenwick tree */

void fw_build(size_t *tree, size_t n)
{
  size_t i;
  for (i = 1; i <= n; ++i) {
    size_t parent = i + (i & -i);
    if (parent <= n) {
      tree[parent - 1] += tree[i - 1];
    }
  }
}

void fw_update(size_t *tree, size_t n, size_t i, size_t delta)
{
  for (; i <= n; i += i & -i) {
    tree[i - 1] += delta;
  }
}

size_t fw_query(size_t *tree, size_t i)
{
  size_t sum = 0;
  for (; i > 0; i -= i & -i) {
    sum += tree[i - 1];
  }
  return sum;
}
