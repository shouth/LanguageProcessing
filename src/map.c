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
  }

  map->size       = 0;
  map->mask       = capacity;
  map->buckets    = xmalloc(sizeof(MapBucket) * (map->mask + NEIGHBORHOOD));
  map->hasher     = hasher ? hasher : &map_default_hasher;
  map->comparator = comparator ? comparator : &map_default_comparator;

  {
    MapBucket *bucket   = map->buckets;
    MapBucket *sentinel = map->buckets + map->mask + NEIGHBORHOOD;
    for (; bucket < sentinel; ++bucket) {
      bucket->hop = 0;
    }
  }
}

void map_deinit(Map *map)
{
  free(map->buckets);
}

void map_iterator(MapIterator *iterator, Map *map)
{
  iterator->parent = map;
  iterator->bucket = map->buckets + map->mask + NEIGHBORHOOD;
}

int map_iterator_next(MapIterator *iterator)
{
  MapBucket *sentinel = iterator->parent->buckets + iterator->parent->mask + NEIGHBORHOOD;
  if (iterator->bucket == sentinel) {
    iterator->bucket = iterator->parent->buckets;
  } else {
    ++iterator->bucket;
  }

  while (iterator->bucket < sentinel && !(iterator->bucket->hop & (1ul << (NEIGHBORHOOD - 1)))) {
    ++iterator->bucket;
  }
  return iterator->bucket != sentinel;
}

void *map_iterator_key(MapIterator *iterator)
{
  return iterator->bucket->key;
}

void *map_iterator_value(MapIterator *iterator)
{
  return iterator->bucket->value;
}

void map_iterator_update(MapIterator *iterator, void *value)
{
  iterator->bucket->value = value;
}

static void map_entry_init(MapEntry *entry, Map *map, void *key)
{
  entry->parent = map;
  entry->key    = key;
  entry->bucket = map->buckets + (map->hasher(key) & map->mask);
  entry->slot   = NULL;
}

int map_entry(Map *map, void *key, MapEntry *entry)
{
  map_entry_init(entry, map, key);
  {
    MapBucket *candidate = entry->bucket;
    MapBucket *sentinel  = entry->bucket + NEIGHBORHOOD - 1;
    for (; candidate < sentinel; ++candidate) {
      if (entry->bucket->hop & (1ul << (candidate - entry->bucket)) && map->comparator(entry->key, candidate->key)) {
        entry->slot = candidate;
        break;
      }
    }
  }
  return !!entry->slot;
}

void *map_entry_key(MapEntry *entry)
{
  return entry->key;
}

void *map_entry_value(MapEntry *entry)
{
  if (!entry->slot) {
    map_entry_update(entry, NULL);
  }
  return entry->slot->value;
}

void map_entry_update(MapEntry *entry, void *value)
{
  MapBucket *empty = entry->slot;

  if (!empty) {
    MapBucket *candidate = entry->bucket;
    MapBucket *sentinel  = entry->bucket + NEIGHBORHOOD * 8;
    for (; candidate < sentinel; ++candidate) {
      if (!(candidate->hop & (1ul << (NEIGHBORHOOD - 1)))) {
        empty = candidate;
        break;
      }
    }

    if (empty) {
      while (empty - entry->bucket >= NEIGHBORHOOD - 1) {
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
    empty->key   = entry->key;
    empty->value = value;
    entry->bucket->hop |= 1ul << (empty - entry->bucket);
    entry->slot = empty;
    ++entry->parent->size;
  } else {
    Map         old = *entry->parent;
    MapIterator iterator;

    map_init_with_capacity(entry->parent, (old.mask + 1) << 1, old.hasher, old.comparator);
    map_iterator(&iterator, &old);
    while (map_iterator_next(&iterator)) {
      MapEntry e;
      map_entry_init(&e, entry->parent, map_iterator_key(&iterator));
      map_entry_update(&e, map_iterator_value(&iterator));
    }
    map_entry_init(entry, entry->parent, entry->key);
    map_entry_update(entry, value);
    map_deinit(&old);
  }
}

void map_entry_erase(MapEntry *entry)
{
  if (entry->slot) {
    entry->bucket->hop &= ~(1ul << (entry->slot - entry->bucket));
    entry->slot->hop &= ~(1ul << (NEIGHBORHOOD - 1));
    entry->slot = NULL;
    --entry->parent->size;
  }
}

unsigned long map_size(Map *map)
{
  return map->size;
}

void *map_value(Map *map, void *key)
{
  MapEntry entry;
  map_entry(map, key, &entry);
  return map_entry_value(&entry);
}

void map_update(Map *map, void *key, void *value)
{
  MapEntry entry;
  map_entry(map, key, &entry);
  map_entry_update(&entry, value);
}

void map_erase(Map *map, void *key)
{
  MapEntry index;
  map_entry(map, key, &index);
  map_entry_erase(&index);
}
