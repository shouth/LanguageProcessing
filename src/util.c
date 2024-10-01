/* SPDX-License-Identifier: Apache-2.0 */

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"

/* Memory */

void *xmalloc(unsigned long size)
{
  void *result = malloc(size);
  if (!result) {
    fprintf(stderr, "Internal Error: Failed to allocate memory. Aborted.");
    exit(EXIT_FAILURE);
  }
  return result;
}

unsigned long popcount(const void *data, unsigned long count)
{
#define B2(n) n, n + 1, n + 1, n + 2
#define B4(n) B2(n), B2(n + 1), B2(n + 1), B2(n + 2)
#define B6(n) B4(n), B4(n + 1), B4(n + 1), B4(n + 2)

  static const unsigned char table[] = {
    B6(0), B6(1), B6(1), B6(2)
  };

#undef B2
#undef B4
#undef B6

  unsigned long result = 0;
  unsigned long i;
  for (i = 0; i < count; ++i) {
    result += table[((unsigned char *) data)[i]];
  }
  return result;
}

/* Hash */

Hash hash_fnv1a(unsigned long *hash, const void *ptr, unsigned long len)
{
  unsigned long i;

  Hash result = hash ? *hash : 0x811C9DC5ul;
  for (i = 0; i < len; ++i) {
    result = 0xFFFFFFFFul & ((result ^ *((unsigned char *) ptr + i)) * 0x01000193ul);
  }
  if (hash) {
    *hash = result;
  }
  return result;
}

/* Hopscotch */

void hopscotch_alloc(Hopscotch *hopscotch, unsigned long base_count, HopscotchHash *hash, HopscotchEq *eq)
{
  hopscotch->count = base_count;
  if (hopscotch->count > 0) {
    unsigned long i;

    --hopscotch->count;
    for (i = 1; i < sizeof(i) * CHAR_BIT; i <<= 1) {
      hopscotch->count |= hopscotch->count >> i;
    }
    ++hopscotch->count;

    hopscotch->hops = xmalloc((hopscotch->count + HOPSCOTCH_BUCKET_SIZE) * sizeof(unsigned long));
    memset(hopscotch->hops, 0, (hopscotch->count + HOPSCOTCH_BUCKET_SIZE) * sizeof(unsigned long));
  } else {
    hopscotch->hops = NULL;
  }
  hopscotch->hash = hash;
  hopscotch->eq   = eq;
}

void hopscotch_free(Hopscotch *hopscotch)
{
  free(hopscotch->hops);
}

void hopscotch_unchecked(Hopscotch *hopscotch, const void *key, HopscotchEntry *entry)
{
  if (key && hopscotch->count > 0) {
    entry->bucket = hopscotch->hash(key) & (hopscotch->count - 1);
  } else {
    entry->bucket = hopscotch->count;
  }
  entry->slot = HOPSCOTCH_BUCKET_SIZE;
}

int hopscotch_entry(Hopscotch *hopscotch, void *data, unsigned long size, const void *key, HopscotchEntry *entry)
{
  hopscotch_unchecked(hopscotch, key, entry);
  if (entry->bucket < hopscotch->count) {
    for (entry->slot = 0; entry->slot < HOPSCOTCH_BUCKET_SIZE; ++entry->slot) {
      if (hopscotch->hops[entry->bucket] & (1ul << entry->slot)) {
        if (hopscotch->eq((char *) data + (entry->bucket + entry->slot) * size, key)) {
          return 1;
        }
      }
    }
  }
  return 0;
}

int hopscotch_next(Hopscotch *hopscotch, HopscotchEntry *entry)
{
  if (entry->bucket < hopscotch->count) {
    ++entry->slot;
  } else {
    entry->bucket = 0;
    entry->slot   = 0;
  }

  for (; entry->bucket < hopscotch->count; ++entry->bucket) {
    for (; entry->slot < HOPSCOTCH_BUCKET_SIZE; ++entry->slot) {
      if (hopscotch->hops[entry->bucket] & (1ul << entry->slot)) {
        return 1;
      }
    }
    entry->slot = 0;
  }
  return 0;
}

int hopscotch_occupy(Hopscotch *hopscotch, void *data, unsigned long size, HopscotchEntry *entry)
{
  if (entry->bucket >= hopscotch->count) {
    return 0;
  } else if (entry->slot < HOPSCOTCH_BUCKET_SIZE) {
    return -1;
  } else if (hopscotch->hops[entry->bucket] + 1 == 0) {
    return 0;
  } else {
    unsigned long empty = -1ul;

    {
      unsigned long bucket   = entry->bucket > HOPSCOTCH_BUCKET_SIZE - 1ul ? entry->bucket - (HOPSCOTCH_BUCKET_SIZE - 1ul) : 0ul;
      unsigned long sentinel = entry->bucket + HOPSCOTCH_BUCKET_SIZE * 8ul;
      unsigned long occupied = 0;

      for (; bucket < sentinel; ++bucket) {
        occupied = (occupied >> 1) | hopscotch->hops[bucket];
        if (bucket >= entry->bucket && ~occupied & 1ul) {
          empty = bucket;
          break;
        }
      }

      if (bucket == sentinel) {
        return 0;
      }
    }

    while (empty - entry->bucket >= HOPSCOTCH_BUCKET_SIZE) {
      unsigned long bucket, next;

      for (bucket = empty - (HOPSCOTCH_BUCKET_SIZE - 1ul); bucket < empty; ++bucket) {
        if (bucket < hopscotch->count && hopscotch->hops[bucket] & ((1ul << (empty - bucket)) - 1ul)) {
          break;
        }
      }

      if (bucket == empty) {
        return 0;
      }

      for (next = bucket; next < empty; ++next) {
        if (hopscotch->hops[bucket] & (1ul << (next - bucket))) {
          break;
        }
      }

      hopscotch->hops[bucket] &= ~(1ul << (next - bucket));
      hopscotch->hops[bucket] |= 1ul << (empty - bucket);
      memcpy((char *) data + empty * size, (char *) data + next * size, size);
      empty = next;
    }

    entry->slot = empty - entry->bucket;
    hopscotch->hops[entry->bucket] |= 1ul << entry->slot;
    return 1;
  }
}

int hopscotch_release(Hopscotch *hopscotch, HopscotchEntry *entry)
{
  if (entry->bucket < hopscotch->count && entry->slot < HOPSCOTCH_BUCKET_SIZE) {
    hopscotch->hops[entry->bucket] &= ~(1ul << entry->slot);
    return 1;
  } else {
    return 0;
  }
}

/* Charactor */

int is_alphabet(int c)
{
  return !!strchr("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz", c);
}

int is_number(int c)
{
  return c >= '0' && c <= '9';
}

int is_space(int c)
{
  return !!strchr(" \t\r\n", c);
}

int is_graphic(int c)
{
  return is_alphabet(c) || is_number(c) || is_space(c) || !!strchr("!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~", c);
}

long utf8_len(const char *str, long len)
{
  if ((str[0] & 0x80) == 0x00) {
    return 1;
  } else if ((str[0] & 0xE0) == 0xC0) {
    return len >= 2 && (str[1] & 0xC0) == 0x80 ? 2 : -1;
  } else if ((str[0] & 0xF0) == 0xE0) {
    return len >= 3 && (str[1] & 0xC0) == 0x80 && (str[2] & 0xC0) == 0x80 ? 3 : -1;
  } else if ((str[0] & 0xF8) == 0xF0) {
    return len >= 4 && (str[1] & 0xC0) == 0x80 && (str[2] & 0xC0) == 0x80 && (str[3] & 0xC0) == 0x80 ? 4 : -1;
  } else {
    return -1;
  }
}
