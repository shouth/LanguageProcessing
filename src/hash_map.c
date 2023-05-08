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
    hop ^= 1ul << t;
  }
  return NULL;
}

void hash_map_update(hash_map_t *table, void *key, void *value)
{
  long              index = calc_index(table, key);
  long              dist;
  hash_map_entry_t *home  = table->buckets + index;
  hash_map_entry_t *empty = NULL;
  for (dist = 0; dist < NBHD_RANGE * 8; ++dist) {
    if (!home[dist].key) {
      empty = home + dist;
      break;
    }
  }

  while (empty && dist >= NBHD_RANGE) {
    hash_map_entry_t *entry = empty - NBHD_RANGE + 1;
    long              i;
    for (i = 0; i < NBHD_RANGE; ++i) {
      if (entry[i].hop) {
        int t = bit_right_most(entry[i].hop);
        if (i + t < NBHD_RANGE) {
          hash_map_entry_t *next = entry + i + t;
          empty->key             = next->key;
          empty->value           = next->value;
          next->hop ^= 1ul << t;
          next->hop ^= 1ul << (NBHD_RANGE - i - 1);
          empty = next;
          dist -= NBHD_RANGE - 1 - i - t;
          break;
        }
      }
    }
    if (i == NBHD_RANGE) {
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
    home->hop ^= 1ul << dist;
    ++table->size;
    if (100 * table->size / table->bucket_cnt >= table->load_factor) {
      grow_buckets(table);
    }
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
      home->hop ^= 1ul << t;
      --table->size;
      return 1;
    }
    hop ^= 1ul << t;
  }
  return 0;
}

hash_map_t *hash_map_new(hash_map_comp_t *comparator, hash_map_hasher_t *hasher)
{
  hash_map_t *map  = xmalloc(sizeof(hash_map_t));
  map->capacity    = 1 << 4;
  map->load_factor = 60;
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
