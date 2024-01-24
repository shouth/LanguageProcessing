#include <stddef.h>
#include <stdlib.h>

#include "map.h"
#include "utility.h"

#define NEIGHBORHOOD ((long) sizeof(unsigned long) * 8)

struct MapBucket {
  unsigned long hop;
  void         *key;
  void         *value;
};

struct Map {
  unsigned long  count;
  unsigned long  mask;
  MapBucket     *buckets;
  MapHasher     *hasher;
  MapComparator *comparator;
};

static unsigned long map_default_hasher(const void *value)
{
  return fnv1a(FNV1A_INIT, &value, sizeof(value));
}

static int map_default_comparator(const void *left, const void *right)
{
  return left == right;
}

Map *map_new(MapHasher *hasher, MapComparator *comparator)
{
  return map_new_with_capacity(1l << 4, hasher, comparator);
}

Map *map_new_with_capacity(unsigned long capacity, MapHasher *hasher, MapComparator *comparator)
{
  Map *map        = xmalloc(sizeof(Map));
  map->count      = 0;
  map->mask       = 0;
  map->buckets    = NULL;
  map->hasher     = hasher ? hasher : &map_default_hasher;
  map->comparator = comparator ? comparator : &map_default_comparator;
  map_reserve(map, capacity);
  return map;
}

void map_free(Map *map)
{
  if (map) {
    free(map->buckets);
    free(map);
  }
}

unsigned long map_count(Map *map)
{
  return map->count;
}

static void map_index_init(MapIndex *index, Map *map, void *key)
{
  index->_bucket = map->buckets + (map->hasher(key) & map->mask);
  index->_slot   = NULL;
}

void map_reserve(Map *map, unsigned long capacity)
{
  unsigned long i;
  unsigned long new_mask = capacity - 1;

  for (i = 1; i < sizeof(new_mask); i <<= 1) {
    new_mask |= new_mask >> i;
  }

  if (new_mask > map->mask) {
    MapBucket    *old_buckets = map->buckets;
    unsigned long old_mask    = map->mask;
    unsigned long hop         = 0;

    map->count   = 0;
    map->mask    = new_mask;
    map->buckets = xmalloc(sizeof(MapBucket) * (new_mask + NEIGHBORHOOD));
    for (i = 0; i < new_mask + NEIGHBORHOOD; ++i) {
      map->buckets[i].hop   = 0;
      map->buckets[i].key   = NULL;
      map->buckets[i].value = NULL;
    }

    if (old_buckets) {
      for (i = 0; i < old_mask + NEIGHBORHOOD; ++i) {
        hop = (hop >> 1) | old_buckets[i].hop;
        if (hop & 1) {
          MapIndex index;
          map_index_init(&index, map, old_buckets[i].key);
          map_update(map, &index, old_buckets[i].key, old_buckets[i].value);
        }
      }
      free(old_buckets);
    }
  }
}

int map_entry(Map *map, void *key, MapIndex *index)
{
  unsigned long i;
  map_index_init(index, map, key);
  for (i = 0; i < NEIGHBORHOOD; ++i) {
    if (index->_bucket->hop & (1ul << i) && map->comparator(key, index->_bucket[i].key)) {
      index->_slot = index->_bucket + i;
      break;
    }
  }
  return !!index->_slot;
}

void map_iterator(Map *map, MapIndex *index)
{
  index->_bucket = map->buckets;
  index->_slot   = NULL;
}

int map_next(Map *map, MapIndex *index)
{
  unsigned long i;
  if (index->_slot) {
    ++index->_slot;
  } else {
    index->_slot = index->_bucket;
  }

  for (i = index->_slot - index->_bucket; i < NEIGHBORHOOD; ++i) {
    if (index->_bucket->hop & (1ul << i)) {
      index->_slot = index->_bucket + i;
      return 1;
    }
  }

  ++index->_bucket;
  for (; index->_bucket < map->buckets + map->mask + NEIGHBORHOOD; ++index->_bucket) {
    if (index->_bucket->hop) {
      for (i = 0; i < NEIGHBORHOOD; ++i) {
        if (index->_bucket->hop & (1ul << i)) {
          index->_slot = index->_bucket + i;
          return 1;
        }
      }
    }
  }

  index->_bucket = NULL;
  index->_slot   = NULL;
  return 0;
}

void *map_key(Map *map, MapIndex *index)
{
  return index->_slot ? index->_slot->key : NULL;
}

void *map_value(Map *map, MapIndex *index)
{
  return index->_slot ? index->_slot->value : NULL;
}

void map_update(Map *map, MapIndex *index, void *key, void *value)
{
  MapBucket *empty = index->_slot;

  if (!empty) {
    long i = index->_bucket - map->buckets < NEIGHBORHOOD
      ? -(index->_bucket - map->buckets)
      : -NEIGHBORHOOD + 1;

    unsigned long hop = 0;
    for (; i < NEIGHBORHOOD * 8; ++i) {
      hop = (hop >> 1) | index->_bucket[i].hop;
      if (i >= 0 && !(hop & 1)) {
        empty = index->_bucket + i;
        break;
      }
    }

    if (empty) {
      while (empty - index->_bucket >= NEIGHBORHOOD) {
        MapBucket *bucket = empty - NEIGHBORHOOD + 2;
        for (; bucket < empty; ++bucket) {
          if (bucket->hop & ((1ul << (empty - bucket + 1)) - 1)) {
            MapBucket *next = bucket;
            for (; next < empty; ++next) {
              if (bucket->hop & (1ul << (next - bucket))) {
                break;
              }
            }

            bucket->hop &= ~(1ul << (next - bucket));
            bucket->hop |= 1ul << (empty - bucket);
            empty->key   = next->key;
            empty->value = next->value;
            empty        = next;
            break;
          }
        }
        if (bucket == empty) {
          empty = NULL;
          break;
        }
      }
    }
  }

  if (empty) {
    empty->key   = key;
    empty->value = value;
    index->_bucket->hop |= 1ul << (empty - index->_bucket);
    index->_slot = empty;
    ++map->count;
  } else {
    map_reserve(map, (map->mask + 1) << 1);
    map_index_init(index, map, key);
    map_update(map, index, key, value);
  }
}

void map_erase(Map *map, MapIndex *index)
{
  if (index->_slot) {
    index->_bucket->hop &= ~(1ul << (index->_slot - index->_bucket));
    index->_slot = NULL;
    --map->count;
  }
}
