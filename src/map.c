#include <limits.h>
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

static void map_index_init(MapIterator *iterator, unsigned long hash, void *key)
{
  iterator->key    = key;
  iterator->hash   = hash;
  iterator->offset = ULONG_MAX;
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

static MapBucket *map_bucket_at(const Map *map, const MapIterator *iterator)
{
  return map->buckets + (iterator->hash & (map->capacity - 1));
}

int map_find(const Map *map, void *key, MapIterator *iterator)
{
  map_index_init(iterator, map->hasher(key), key);
  {
    MapBucket *bucket   = map_bucket_at(map, iterator);
    MapBucket *slot     = bucket;
    MapBucket *sentinel = slot + NEIGHBORHOOD - 1;
    for (; slot < sentinel; ++slot) {
      if (bucket->hop & (1ul << (slot - bucket)) && map->comparator(iterator->key, slot->key)) {
        iterator->offset = slot - bucket;
        break;
      }
    }
  }
  return iterator->offset != ULONG_MAX;
}

void *map_value(Map *map, void *key)
{
  MapIterator index;
  map_find(map, key, &index);
  return map_value_at(map, &index);
}

void *map_value_at(Map *map, MapIterator *iterator)
{
  if (iterator->offset == ULONG_MAX) {
    map_insert_at(map, iterator, NULL);
  }

  {
    MapBucket *slot = map_bucket_at(map, iterator) + iterator->offset;
    return slot->value;
  }
}

void map_insert(Map *map, void *key, void *value)
{
  MapIterator index;
  map_find(map, key, &index);
  map_insert_at(map, &index, value);
}

void map_insert_at(Map *map, MapIterator *iterator, void *value)
{
  MapBucket *bucket = map_bucket_at(map, iterator);
  MapBucket *empty  = NULL;

  if (iterator->offset != ULONG_MAX) {
    empty = bucket + iterator->offset;
  } else {
    MapBucket *candidate = bucket;
    MapBucket *sentinel  = bucket + NEIGHBORHOOD * 8;
    for (; candidate < sentinel; ++candidate) {
      if (!(candidate->hop & (1ul << (NEIGHBORHOOD - 1)))) {
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
    empty->key   = iterator->key;
    empty->value = value;
    bucket->hop |= 1ul << (empty - bucket);
    iterator->offset = empty - bucket;
    ++map->size;
  } else {
    Map        old      = *map;
    MapBucket *bucket   = old.buckets;
    MapBucket *sentinel = old.buckets + old.capacity + NEIGHBORHOOD - 1;

    map_init_with_capacity(map, old.capacity << 1, old.hasher, old.comparator);
    for (; bucket < sentinel; ++bucket) {
      if (bucket->hop & (1ul << (NEIGHBORHOOD - 1))) {
        MapIterator iterator;
        map_index_init(&iterator, map->hasher(bucket->key), bucket->key);
        map_insert_at(map, &iterator, bucket->value);
      }
    }
    map_insert_at(map, iterator, value);
    map_deinit(&old);
  }
}

void map_erase(Map *map, void *key)
{
  MapIterator index;
  map_find(map, key, &index);
  map_erase_at(map, &index);
}

void map_erase_at(Map *map, MapIterator *iterator)
{
  if (iterator->offset != ULONG_MAX) {
    MapBucket *bucket = map_bucket_at(map, iterator);
    MapBucket *slot   = bucket + iterator->offset;

    bucket->hop &= ~(1ul << (slot - bucket));
    slot->hop &= ~(1ul << (NEIGHBORHOOD - 1));
    iterator->offset = ULONG_MAX;
  }
}
