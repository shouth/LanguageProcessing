/* SPDX-License-Identifier: Apache-2.0 */

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utility.h"

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

void *memdup(const void *ptr, unsigned long length)
{
  if (length == 0) {
    return NULL;
  } else {
    void *result = xmalloc(length);
    memcpy(result, ptr, length);
    return result;
  }
}

char *strndup(const char *src, unsigned long length)
{
  if (length == 0) {
    return NULL;
  } else {
    char *dest = xmalloc(length + 1);
    strncpy(dest, src, length);
    dest[length] = '\0';
    return dest;
  }
}

unsigned long splice(
  void *dest, unsigned long dest_count,
  const void *src1, unsigned long src1_count,
  unsigned long size, unsigned long offset, unsigned long span,
  const void *src2, unsigned long src2_count)
{
  unsigned long result_count = src1_count + src2_count - span;
  if (result_count > dest_count) {
    result_count = dest_count;
  }

  if (src1) {
    memcpy(dest, src1, offset * size);
  }

  if (offset + src2_count < dest_count) {
    unsigned long remain = dest_count - (offset + src2_count);
    unsigned long count  = src1_count - (offset + span);
    if (count > remain) {
      count = remain;
    }

    if (count > 0) {
      if (src1) {
        memcpy((char *) dest + (offset + src2_count) * size, (char *) src1 + (offset + span) * size, count * size);
      } else {
        memmove((char *) dest + (offset + src2_count) * size, (char *) dest + (offset + span) * size, count * size);
      }
    }
  }

  {
    unsigned long remain = dest_count - offset;
    unsigned long count  = src2_count;
    if (count > remain) {
      count = remain;
    }

    if (count > 0) {
      if (src2) {
        memcpy((char *) dest + offset * size, (char *) src2, count * size);
      }
    }
  }

  return result_count;
}

void *splice_alloc(
  const void *src1, unsigned long src1_count,
  unsigned long size, unsigned long offset, unsigned long span,
  const void *src2, unsigned long src2_count,
  unsigned long *result_count)
{
  unsigned long count  = src1_count + src2_count - span;
  void         *result = xmalloc(count * size);
  *result_count        = splice(result, count, src1, src1_count, size, offset, span, src2, src2_count);
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

unsigned long hash_fnv1a(unsigned long *hash, const void *ptr, unsigned long len)
{
  unsigned long i;

  unsigned long result = hash ? *hash : 0x811C9DC5ul;
  for (i = 0; i < len; ++i) {
    result = 0xFFFFFFFFul & ((result ^ *((unsigned char *) ptr + i)) * 0x01000193ul);
  }
  if (hash) {
    *hash = result;
  }
  return result;
}

/* Hopscotch */

#define BUCKET_SIZE (sizeof(unsigned long) * CHAR_BIT)

void hopscotch_alloc(Hopscotch *hopscotch, unsigned long span, HopscotchHash *hash, HopscotchEq *eq)
{
  seq_alloc(&hopscotch->hops, span);
  memset(hopscotch->hops.ptr, 0, span * sizeof(unsigned long));
  hopscotch->hash = hash;
  hopscotch->eq   = eq;
}

void hopscotch_free(Hopscotch *hopscotch)
{
  seq_free(&hopscotch->hops);
}

void hopscotch_unchecked(Hopscotch *hopscotch, const void *key, HopscotchEntry *entry)
{
  if (key && hopscotch->hops.span > 0) {
    entry->bucket = hopscotch->hash(key) % hopscotch->hops.span;
  } else {
    entry->bucket = hopscotch->hops.span;
  }
  entry->slot = BUCKET_SIZE;
}

int hopscotch_entry(Hopscotch *hopscotch, void *data, unsigned long size, const void *key, HopscotchEntry *entry)
{
  hopscotch_unchecked(hopscotch, key, entry);
  if (entry->bucket < hopscotch->hops.span) {
    for (entry->slot = 0; entry->slot < BUCKET_SIZE; ++entry->slot) {
      if (hopscotch->hops.ptr[entry->bucket] & (1ul << entry->slot)) {
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
  if (entry->bucket < hopscotch->hops.span) {
    ++entry->slot;
  } else {
    entry->bucket = 0;
    entry->slot   = 0;
  }

  for (; entry->bucket < hopscotch->hops.span; ++entry->bucket) {
    for (; entry->slot < BUCKET_SIZE; ++entry->slot) {
      if (hopscotch->hops.ptr[entry->bucket] & (1ul << entry->slot)) {
        return 1;
      }
    }
    entry->slot = 0;
  }
  return 0;
}

int hopscotch_occupy(Hopscotch *hopscotch, void *data, unsigned long size, HopscotchEntry *entry)
{
  if (entry->bucket >= hopscotch->hops.span) {
    return 0;
  } else if (entry->slot < BUCKET_SIZE) {
    return -1;
  } else if (hopscotch->hops.ptr[entry->bucket] + 1 == 0) {
    return 0;
  } else {
    unsigned long bucket, slot;
    unsigned long occupied = 0;
    unsigned long empty    = -1ul;

    bucket = entry->bucket > BUCKET_SIZE - 1ul ? entry->bucket - (BUCKET_SIZE - 1ul) : 0ul;
    for (; bucket < entry->bucket + BUCKET_SIZE * 8ul; ++bucket) {
      occupied >>= 1ul;
      if (bucket < hopscotch->hops.span) {
        occupied |= hopscotch->hops.ptr[bucket];
      }
      if (bucket >= entry->bucket && ~occupied & 1ul) {
        empty = bucket;
        break;
      }
    }

    if (empty == -1ul) {
      return 0;
    }

    while (empty - entry->bucket >= BUCKET_SIZE) {
      for (bucket = empty - (BUCKET_SIZE - 1ul); bucket < empty; ++bucket) {
        if (bucket < hopscotch->hops.span && hopscotch->hops.ptr[bucket] & ((1ul << (empty - bucket)) - 1ul)) {
          break;
        }
      }

      if (bucket == empty) {
        return 0;
      }

      for (slot = 0; bucket + slot < empty; ++slot) {
        if (hopscotch->hops.ptr[bucket] & (1ul << slot)) {
          break;
        }
      }

      hopscotch->hops.ptr[bucket] &= ~(1ul << slot);
      hopscotch->hops.ptr[bucket] |= 1ul << (empty - bucket);
      memcpy((char *) data + empty * size, (char *) data + (bucket + slot) * size, size);
      empty = bucket + slot;
    }

    entry->slot = empty - entry->bucket;
    hopscotch->hops.ptr[entry->bucket] |= 1ul << entry->slot;
    return 1;
  }
}

void hopscotch_release(Hopscotch *hopscotch, HopscotchEntry *entry)
{
  if (entry->bucket < hopscotch->hops.span && entry->slot < BUCKET_SIZE) {
    hopscotch->hops.ptr[entry->bucket] &= ~(1ul << entry->slot);
  }
}

#undef BUCKET_SIZE

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
