#include "assert.h"

#include "utility.h"

#define NBHD_RANGE ((long) (sizeof(hash_hop_t) * 8))

int hash_default_comp(const void *lhs, const void *rhs)
{
  return lhs == rhs;
}

uint64_t hash_default_hasher(const void *ptr)
{
  return fnv1_ptr(ptr);
}

static void hash_init_buckets(hash_t *table)
{
  long i;
  table->size       = 0;
  table->bucket_cnt = table->capacity + NBHD_RANGE;
  table->buckets    = new_arr(hash_entry_t, table->bucket_cnt);
  for (i = 0; i < table->bucket_cnt; i++) {
    table->buckets[i].hop   = 0;
    table->buckets[i].key   = NULL;
    table->buckets[i].value = NULL;
  }
}

hash_t *hash_new(hash_comp_t *comparator, hash_hasher_t *hasher)
{
  hash_t *ret;
  assert(comparator && hasher);

  ret              = new (hash_t);
  ret->capacity    = 1 << 6;
  ret->load_factor = 60;
  ret->comparator  = comparator;
  ret->hasher      = hasher;
  hash_init_buckets(ret);
  return ret;
}

void hash_delete(hash_t *table, hash_deleter_t *key_deleter, hash_deleter_t *value_deleter)
{
  long i;
  if (!table) {
    return;
  }

  for (i = 0; i < table->bucket_cnt; i++) {
    if (table->buckets[i].key) {
      if (key_deleter) {
        key_deleter(table->buckets[i].key);
      }
      if (value_deleter) {
        value_deleter(table->buckets[i].value);
      }
    }
  }
  free(table->buckets);
  free(table);
}

static void hash_grow(hash_t *table)
{
  hash_entry_t *old_buckets;
  long          old_bucket_cnt;
  long          i;
  assert(table);

  old_buckets    = table->buckets;
  old_bucket_cnt = table->bucket_cnt;
  table->capacity <<= 1;
  hash_init_buckets(table);
  for (i = 0; i < old_bucket_cnt; i++) {
    if (old_buckets[i].key) {
      hash_insert_unsafe(table, old_buckets[i].key, old_buckets[i].value);
    }
  }
  free(old_buckets);
}

static long hash_index(hash_t *table, const void *key)
{
  assert(table && key);
  return table->hasher(key) & (table->capacity - 1);
}

const hash_entry_t *hash_find(hash_t *table, const void *key)
{
  hash_entry_t *home;
  hash_hop_t    hop;
  long          index;
  assert(table && key);

  index = hash_index(table, key);
  home  = table->buckets + index;
  hop   = home->hop;
  while (hop) {
    uint8_t t = trailing0(hop);
    if (table->comparator(key, home[t].key)) {
      return home + t;
    }
    hop ^= (hash_hop_t) 1 << t;
  }
  return NULL;
}

void hash_insert_unsafe(hash_t *table, void *key, void *value)
{
  hash_entry_t *home, *empty = NULL;
  long          dist, index;
  long          i;
  assert(table && key && value);

  index = hash_index(table, key);
  home  = table->buckets + index;
  for (dist = 0; dist < NBHD_RANGE * 8; dist++) {
    if (!home[dist].key) {
      empty = home + dist;
      break;
    }
  }

  while (empty && dist >= NBHD_RANGE) {
    hash_entry_t *entry = empty - NBHD_RANGE + 1;
    for (i = 0; i < NBHD_RANGE; i++) {
      hash_hop_t hop = entry[i].hop;
      if (hop) {
        uint8_t t = trailing0(hop);
        if (i + t < NBHD_RANGE) {
          hash_entry_t *next = entry + i + t;
          empty->key         = next->key;
          empty->value       = next->value;
          next->hop ^= (hash_hop_t) 1 << t;
          next->hop ^= (hash_hop_t) 1 << (NBHD_RANGE - i - 1);
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
    hash_grow(table);
    hash_insert_unsafe(table, key, value);
  } else {
    empty->key   = key;
    empty->value = value;
    home->hop ^= (hash_hop_t) 1 << dist;
    table->size++;
    if (100 * table->size / table->bucket_cnt >= table->load_factor) {
      hash_grow(table);
    }
  }
}

hash_entry_t *hash_insert(hash_t *table, void *key, void *value)
{
  hash_entry_t *ret;
  assert(table && key && value);

  ret = hash_remove(table, key);
  hash_insert_unsafe(table, key, value);
  return ret;
}

hash_entry_t *hash_remove(hash_t *table, const void *key)
{
  hash_entry_t *home;
  hash_hop_t    hop;
  long          index;
  assert(table && key);

  index = hash_index(table, key);
  home  = table->buckets + index;
  hop   = home->hop;
  while (hop) {
    uint8_t t = trailing0(hop);
    if (table->comparator(key, home[t].key)) {
      table->removed.key   = home[t].key;
      table->removed.value = home[t].value;
      home[t].key          = NULL;
      home->hop ^= (hash_hop_t) 1 << t;
      table->size--;
      return &table->removed;
    }
    hop ^= (hash_hop_t) 1 << t;
  }
  return NULL;
}
