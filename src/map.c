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

static void map_index_init(const Map *map, MapIterator *iterator, void *key)
{
  iterator->key    = key;
  iterator->bucket = map->buckets + (map->hasher(key) & (map->capacity - 1));
  iterator->slot   = NULL;
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
    MapBucket *sentinel = map->buckets + map->capacity + NEIGHBORHOOD - 1;
    for (; bucket < sentinel; ++bucket) {
      bucket->hop = 0;
    }
  }
}

void map_deinit(Map *map)
{
  free(map->buckets);
}

int map_find(const Map *map, void *key, MapIterator *iterator)
{
  map_index_init(map, iterator, key);
  {
    MapBucket *candidate = iterator->bucket;
    MapBucket *sentinel  = iterator->bucket + NEIGHBORHOOD - 1;
    for (; candidate < sentinel; ++candidate) {
      if (iterator->bucket->hop & (1ul << (candidate - iterator->bucket)) && map->comparator(iterator->key, candidate->key)) {
        iterator->slot = candidate;
        break;
      }
    }
  }
  return !!iterator->slot;
}

void *map_value(Map *map, void *key)
{
  MapIterator iterator;
  map_find(map, key, &iterator);
  return map_value_at(map, &iterator);
}

void *map_value_at(Map *map, MapIterator *iterator)
{
  if (!iterator->slot) {
    map_update_at(map, iterator, NULL);
  }
  return iterator->slot->value;
}

void map_update(Map *map, void *key, void *value)
{
  MapIterator iterator;
  map_find(map, key, &iterator);
  map_update_at(map, &iterator, value);
}

void map_update_at(Map *map, MapIterator *iterator, void *value)
{
  MapBucket *empty = iterator->slot;

  if (!empty) {
    MapBucket *candidate = iterator->bucket;
    MapBucket *sentinel  = iterator->bucket + NEIGHBORHOOD * 8;
    for (; candidate < sentinel; ++candidate) {
      if (!(candidate->hop & (1ul << (NEIGHBORHOOD - 1)))) {
        empty = candidate;
        break;
      }
    }

    if (empty) {
      while (empty - iterator->bucket >= NEIGHBORHOOD - 1) {
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
    empty->key   = iterator->key;
    empty->value = value;
    iterator->bucket->hop |= 1ul << (empty - iterator->bucket);
    iterator->slot = empty;
    ++map->size;
  } else {
    Map        old      = *map;
    MapBucket *bucket   = old.buckets;
    MapBucket *sentinel = old.buckets + old.capacity + NEIGHBORHOOD - 1;

    map_init_with_capacity(map, old.capacity << 1, old.hasher, old.comparator);
    for (; bucket < sentinel; ++bucket) {
      if (bucket->hop & (1ul << (NEIGHBORHOOD - 1))) {
        MapIterator iterator;
        map_index_init(map, &iterator, bucket->key);
        map_update_at(map, &iterator, bucket->value);
      }
    }
    map_index_init(map, iterator, iterator->key);
    map_update_at(map, iterator, value);
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
  if (iterator->slot) {
    iterator->bucket->hop &= ~(1ul << (iterator->slot - iterator->bucket));
    iterator->slot->hop &= ~(1ul << (NEIGHBORHOOD - 1));
    iterator->slot = NULL;
    --map->size;
  }
}
