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

static void init_buckets(hash_map_t *table)
{
  long i;
  table->size       = 0;
  table->bucket_cnt = table->capacity + NBHD_RANGE;
  table->buckets    = xmalloc(sizeof(hash_map_entry_t) * table->bucket_cnt);
  for (i = 0; i < table->bucket_cnt; ++i) {
    table->buckets[i].hop   = 0;
    table->buckets[i].key   = NULL;
    table->buckets[i].value = NULL;
  }
}

static void grow_buckets(hash_map_t *table)
{
  hash_map_entry_t *old_buckets    = table->buckets;
  long              old_bucket_cnt = table->bucket_cnt;
  long              i;

  table->capacity <<= 1;
  init_buckets(table);
  for (i = 0; i < old_bucket_cnt; ++i) {
    if (old_buckets[i].key) {
      hash_map_update(table, old_buckets[i].key, old_buckets[i].value);
    }
  }
  free(old_buckets);
}

static long calc_index(hash_map_t *table, const void *key)
{
  assert(table && key);
  return table->hasher(key) & (table->capacity - 1);
}

const hash_map_entry_t *hash_map_find(hash_map_t *table, const void *key)
{
  long              index = calc_index(table, key);
  hash_map_entry_t *home  = table->buckets + index;
  unsigned long     hop   = home->hop;
  while (hop) {
    int t = bit_right_most(hop);
    if (table->comparator(key, home[t].key)) {
      return home + t;
    }
    hop &= ~(1ul << t);
  }
  return NULL;
}

void hash_map_update(hash_map_t *table, void *key, void *value)
{
  hash_map_entry_t *home  = table->buckets + calc_index(table, key);
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
    grow_buckets(table);
    hash_map_update(table, key, value);
  } else {
    empty->key   = key;
    empty->value = value;
    home->hop |= 1ul << (empty - home);
    ++table->size;
  }
}

int hash_map_remove(hash_map_t *table, const void *key)
{
  long              index = calc_index(table, key);
  hash_map_entry_t *home  = table->buckets + index;
  unsigned long     hop   = home->hop;
  while (hop) {
    int t = bit_right_most(hop);
    if (table->comparator(key, home[t].key)) {
      home[t].key = NULL;
      home->hop &= ~(1ul << t);
      --table->size;
      return 1;
    }
    hop &= ~(1ul << t);
  }
  return 0;
}

hash_map_t *hash_map_new(hash_map_comp_t *comparator, hash_map_hasher_t *hasher)
{
  hash_map_t *map  = xmalloc(sizeof(hash_map_t));
  map->capacity    = NBHD_RANGE;
  map->comparator  = comparator ? comparator : &default_comp;
  map->hasher      = hasher ? hasher : &default_hasher;
  init_buckets(map);
  return map;
}

void hash_map_delete(hash_map_t *table)
{
  if (table) {
    free(table->buckets);
  }
  free(table);
}
