#include <assert.h>

#include "utility.h"

#define NBHD_RANGE ((long) (sizeof(unsigned long) * 8))

static int default_comp(const void *lhs, const void *rhs)
{
  return lhs == rhs;
}

static unsigned long default_hasher(const void *ptr)
{
  return fnv1a(FNV1A_INIT, &ptr, sizeof(void *));
}

static void init_buckets(hash_map_t *map)
{
  long i;
  map->size       = 0;
  map->bucket_cnt = map->capacity + NBHD_RANGE;
  map->buckets    = xmalloc(sizeof(hash_map_entry_t) * map->bucket_cnt);
  for (i = 0; i < map->bucket_cnt; ++i) {
    map->buckets[i].hop   = 0;
    map->buckets[i].key   = NULL;
    map->buckets[i].value = NULL;
  }
}

static void grow_buckets(hash_map_t *map)
{
  hash_map_entry_t *old_buckets    = map->buckets;
  long              old_bucket_cnt = map->bucket_cnt;
  long              i;

  map->capacity <<= 1;
  init_buckets(map);
  for (i = 0; i < old_bucket_cnt; ++i) {
    if (old_buckets[i].key) {
      hash_map_update(map, old_buckets[i].key, old_buckets[i].value);
    }
  }
  free(old_buckets);
}

static long calc_index(hash_map_t *map, const void *key)
{
  assert(map && key);
  return map->hasher(key) & (map->capacity - 1);
}

hash_map_entry_t *hash_map_next(hash_map_t *map, hash_map_entry_t *entry)
{
  if (!entry) {
    entry = map->buckets;
  } else {
    ++entry;
  }

  for (; entry < map->buckets + map->bucket_cnt; ++entry) {
    if (entry->key) {
      return entry;
    }
  }
  return NULL;
}

hash_map_entry_t *hash_map_find(hash_map_t *map, const void *key)
{
  hash_map_entry_t *home = map->buckets + calc_index(map, key);
  unsigned long     hop  = home->hop;
  while (hop) {
    hash_map_entry_t *entry = home + bit_right_most(hop);
    if (map->comparator(key, entry->key)) {
      return entry;
    }
    hop &= ~(1ul << (entry - home));
  }
  return NULL;
}

void hash_map_update(hash_map_t *map, void *key, void *value)
{
  hash_map_entry_t *home  = map->buckets + calc_index(map, key);
  hash_map_entry_t *fence = home + NBHD_RANGE * 8;
  hash_map_entry_t *empty;
  for (empty = home; empty < fence; ++empty) {
    if (!empty->key) {
      break;
    }
  }
  if (empty == fence) {
    empty = NULL;
  }

  while (empty && empty - home >= NBHD_RANGE) {
    hash_map_entry_t *bucket = empty - NBHD_RANGE + 1;
    for (; bucket < empty; ++bucket) {
      unsigned long hop = bucket->hop;
      while (hop) {
        hash_map_entry_t *occupied = bucket + bit_right_most(hop);
        if (occupied < empty) {
          empty->key   = occupied->key;
          empty->value = occupied->value;
          bucket->hop &= ~(1ul << (occupied - bucket));
          bucket->hop |= 1ul << (empty - bucket);
          empty = occupied;
          break;
        }
        hop &= ~(1ul << (occupied - bucket));
      }
    }
    if (bucket == empty) {
      empty->key = NULL;
      empty      = NULL;
    }
  }

  if (!empty) {
    grow_buckets(map);
    hash_map_update(map, key, value);
  } else {
    empty->key   = key;
    empty->value = value;
    home->hop |= 1ul << (empty - home);
    ++map->size;
  }
}

int hash_map_remove(hash_map_t *map, const void *key)
{
  hash_map_entry_t *home = map->buckets + calc_index(map, key);
  unsigned long     hop  = home->hop;
  while (hop) {
    hash_map_entry_t *entry = home + bit_right_most(hop);
    if (map->comparator(key, entry->key)) {
      entry->key = NULL;
      home->hop &= ~(1ul << (entry - home));
      --map->size;
      return 1;
    }
    hop &= ~(1ul << (entry - home));
  }
  return 0;
}

hash_map_t *hash_map_new(hash_map_comp_t *comparator, hash_map_hasher_t *hasher)
{
  hash_map_t *map = xmalloc(sizeof(hash_map_t));
  map->capacity   = NBHD_RANGE;
  map->comparator = comparator ? comparator : &default_comp;
  map->hasher     = hasher ? hasher : &default_hasher;
  init_buckets(map);
  return map;
}

void hash_map_delete(hash_map_t *map)
{
  if (map) {
    free(map->buckets);
  }
  free(map);
}
