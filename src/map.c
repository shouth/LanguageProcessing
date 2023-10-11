#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#include "map.h"
#include "utility.h"

#define NEIGHBORHOOD ((long) sizeof(unsigned long) * 8)

static unsigned long map_default_hasher(const void *value)
{
  return fnv1a(FNV1A_INIT, &value, sizeof(value));
}

static int map_default_comparator(const void *left, const void *right)
{
  return left == right;
}

static void map_index_init(MapIndex *index, unsigned long hash, void *key)
{
  index->key    = key;
  index->hash   = hash;
  index->offset = ULONG_MAX;
}

void map_init(Map *map, MapHasher *hasher, MapComparator *comparator)
{
  map_init_with_capacity(map, 1l << 4, hasher, comparator);
}

void map_init_with_capacity(Map *map, unsigned long capacity, MapHasher *hasher, MapComparator *comparator)
{
  {
    unsigned int shift = 1;
    --capacity;
    for (; shift < sizeof(capacity); shift <<= 1) {
      capacity |= capacity >> shift;
    }
    ++capacity;
  }

  map->size       = 0;
  map->capacity   = capacity;
  map->buckets    = xmalloc(sizeof(MapBucket) * (map->capacity + NEIGHBORHOOD - 1));
  map->hasher     = hasher ? hasher : &map_default_hasher;
  map->comparator = comparator ? comparator : &map_default_comparator;

  {
    MapBucket *bucket   = map->buckets;
    MapBucket *sentinel = bucket + map->capacity + NEIGHBORHOOD - 1;
    for (; bucket < sentinel; ++bucket) {
      bucket->hop = 0;
    }
  }
}

void map_deinit(Map *map)
{
  free(map->buckets);
}

static MapBucket *map_bucket_at(const Map *map, const MapIndex *index)
{
  return map->buckets + (index->hash & (map->capacity - 1));
}

int map_find(const Map *map, void *key, MapIndex *index)
{
  map_index_init(index, map->hasher(key), key);
  {
    MapBucket *bucket   = map_bucket_at(map, index);
    MapBucket *slot     = bucket;
    MapBucket *sentinel = slot + NEIGHBORHOOD - 1;
    for (; slot < sentinel; ++slot) {
      if (bucket->hop & (1ul << (slot - bucket)) && map->comparator(index->key, slot->key)) {
        index->offset = slot - bucket;
        break;
      }
    }
  }
  return index->offset != ULONG_MAX;
}

void *map_value(Map *map, void *key)
{
  MapIndex index;
  map_find(map, key, &index);
  return map_value_at(map, &index);
}

void *map_value_at(Map *map, MapIndex *index)
{
  if (index->offset == ULONG_MAX) {
    map_insert_at(map, index, NULL);
  }

  {
    MapBucket *slot = map_bucket_at(map, index) + index->offset;
    return slot->value;
  }
}

void map_insert(Map *map, void *key, void *value)
{
  MapIndex index;
  map_find(map, key, &index);
  map_insert_at(map, &index, value);
}

void map_insert_at(Map *map, MapIndex *index, void *value)
{
  MapBucket *bucket = map_bucket_at(map, index);
  MapBucket *empty  = NULL;

  if (index->offset != ULONG_MAX) {
    empty = bucket + index->offset;
  } else {
    MapBucket *candidate = bucket;
    MapBucket *sentinel  = bucket + NEIGHBORHOOD * 12;
    for (; candidate < sentinel; ++candidate) {
      if (!(bucket->hop & (1ul << (NEIGHBORHOOD - 1)))) {
        empty = candidate;
        break;
      }
    }

    if (empty) {
      while (empty - bucket >= NEIGHBORHOOD - 1) {
        MapBucket *anchor = empty - NEIGHBORHOOD + 2;
        for (; anchor < empty; ++anchor) {
          MapBucket *next = anchor;
          for (; next < empty; ++next) {
            if (anchor->hop & (1ul << (next - anchor))) {
              break;
            }
          }
          if (next < empty) {
            anchor->hop |= 1ul << (empty - anchor);
            anchor->hop &= ~(1ul << (next - anchor));
            empty->hop |= 1ul << (NEIGHBORHOOD - 1);
            next->hop &= ~(1ul << (NEIGHBORHOOD - 1));

            empty->key   = next->key;
            empty->value = next->value;
            empty        = next;
            break;
          }
        }
        if (anchor == empty) {
          empty = NULL;
          break;
        }
      }
    }
  }

  if (empty) {
    empty->hop |= 1ul << (NEIGHBORHOOD - 1);
    empty->key   = index->key;
    empty->value = value;
    bucket->hop |= 1ul << (empty - bucket);
    index->offset = empty - bucket;
    ++map->size;
  } else {
    MapBucket *buckets  = map->buckets;
    MapBucket *bucket   = buckets;
    MapBucket *sentinel = buckets + map->capacity + NEIGHBORHOOD - 1;

    printf("resize to %lu\n", map->capacity << 1);
    map_init_with_capacity(map, map->capacity << 1, map->hasher, map->comparator);
    for (; bucket < sentinel; ++bucket) {
      if (bucket->hop & (1ul << (NEIGHBORHOOD - 1))) {
        MapIndex index;
        map_index_init(&index, map->hasher(bucket->key), bucket->key);
        map_insert_at(map, &index, bucket->value);
      }
    }
    free(buckets);
    map_insert_at(map, index, value);
  }
}

void map_erase(Map *map, void *key)
{
  MapIndex index;
  map_find(map, key, &index);
  map_erase_at(map, &index);
}

void map_erase_at(Map *map, MapIndex *index)
{
  if (index->offset != ULONG_MAX) {
    MapBucket *bucket = map_bucket_at(map, index);
    MapBucket *slot   = bucket + index->offset;

    bucket->hop &= ~(1ul << (slot - bucket));
    slot->hop &= ~(1ul << (NEIGHBORHOOD - 1));
    index->offset = ULONG_MAX;
  }
}
