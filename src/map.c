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
  index->bucket = map->buckets + (map->hasher(key) & map->mask);
  index->slot   = NULL;
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
          map_index_init(&index, map, map->buckets[i].key);
          map_update(map, &index, map->buckets[i].key, map->buckets[i].value);
        }
      }
      free(old_buckets);
    }
  }
}

int map_find(Map *map, void *key, MapIndex *index)
{
  unsigned long i;
  map_index_init(index, map, key);
  for (i = 0; i < NEIGHBORHOOD - 1; ++i) {
    if (index->bucket->hop & (1ul << i) && map->comparator(key, index->bucket[i].key)) {
      index->slot = index->bucket + i;
      break;
    }
  }
  return !!index->slot;
}

void map_index(Map *map, MapIndex *index)
{
  index->bucket = NULL;
  index->slot   = NULL;
}

int map_next(Map *map, MapIndex *index)
{
  unsigned long i;
  if (index->bucket) {
    ++index->slot;
  } else {
    index->bucket = map->buckets;
    index->slot   = map->buckets;
  }

  for (i = index->slot - index->bucket; i < NEIGHBORHOOD - 1; ++i) {
    if (index->bucket->hop & (1ul << i)) {
      index->slot = index->bucket + i;
      return 1;
    }
  }

  ++index->bucket;
  for (; index->bucket < map->buckets + map->mask + NEIGHBORHOOD; ++index->bucket) {
    if (index->bucket->hop) {
      for (i = 0; i < NEIGHBORHOOD - 1; ++i) {
        if (index->bucket->hop & (1ul << i)) {
          index->slot = index->bucket + i;
          return 1;
        }
      }
    }
  }

  index->bucket = NULL;
  index->slot   = NULL;
  return 0;
}

void *map_key(Map *map, MapIndex *index)
{
  return index->slot ? index->slot->key : NULL;
}

void *map_value(Map *map, MapIndex *index)
{
  return index->slot ? index->slot->value : NULL;
}

void map_update(Map *map, MapIndex *index, void *key, void *value)
{
  MapBucket *empty = index->slot;

  if (!empty) {
    MapBucket *candidate = index->bucket;
    MapBucket *sentinel  = index->bucket + NEIGHBORHOOD * 8;
    for (; candidate < sentinel; ++candidate) {
      if (!(candidate->hop & (1ul << (NEIGHBORHOOD - 1)))) {
        empty = candidate;
        break;
      }
    }

    if (empty) {
      while (empty - index->bucket >= NEIGHBORHOOD - 1) {
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
            next->hop &= ~(1ul << (NEIGHBORHOOD - 1));

            bucket->hop |= 1ul << (empty - bucket);
            empty->hop |= 1ul << (NEIGHBORHOOD - 1);

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
    empty->hop |= 1ul << (NEIGHBORHOOD - 1);
    empty->key   = key;
    empty->value = value;
    index->bucket->hop |= 1ul << (empty - index->bucket);
    index->slot = empty;
    ++map->count;
  } else {
    map_reserve(map, (map->mask + 1) << 1);
    map_index_init(index, map, key);
    map_update(map, index, key, value);
  }
}

void map_erase(Map *map, MapIndex *index)
{
  if (index->slot) {
    index->bucket->hop &= ~(1ul << (index->slot - index->bucket));
    index->slot->hop &= ~(1ul << (NEIGHBORHOOD - 1));
    index->slot = NULL;
    --map->count;
  }
}
