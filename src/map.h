#ifndef MAP_H
#define MAP_H

typedef unsigned long      MapHasher(const void *);
typedef int                MapComparator(const void *, const void *);
typedef struct MapBucket   MapBucket;
typedef struct MapIterator MapIterator;
typedef struct Map         Map;

struct MapBucket {
  unsigned long hop;
  void         *key;
  void         *value;
};

struct MapIterator {
  void      *key;
  MapBucket *bucket;
  MapBucket *slot;
};

struct Map {
  long           size;
  long           capacity;
  MapBucket     *buckets;
  MapHasher     *hasher;
  MapComparator *comparator;
};

void  map_init(Map *map, MapHasher *hasher, MapComparator *comparator);
void  map_init_with_capacity(Map *map, unsigned long capacity, MapHasher *hasher, MapComparator *comparator);
void  map_deinit(Map *map);
int   map_find(const Map *map, void *key, MapIterator *iterator);
void *map_value(Map *map, void *key);
void *map_value_at(Map *map, MapIterator *iterator);
void  map_update(Map *map, void *key, void *value);
void  map_update_at(Map *map, MapIterator *iterator, void *value);
void  map_erase(Map *map, void *key);
void  map_erase_at(Map *map, MapIterator *iterator);

#endif
