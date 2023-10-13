#ifndef MAP_H
#define MAP_H

typedef unsigned long      MapHasher(const void *);
typedef int                MapComparator(const void *, const void *);
typedef struct MapBucket   MapBucket;
typedef struct MapIterator MapIterator;
typedef struct MapEntry    MapEntry;
typedef struct Map         Map;

struct MapBucket {
  unsigned long hop;
  void         *key;
  void         *value;
};

struct MapIterator {
  Map       *parent;
  MapBucket *bucket;
};

struct MapEntry {
  Map       *parent;
  void      *key;
  MapBucket *bucket;
  MapBucket *slot;
};

struct Map {
  unsigned long  size;
  unsigned long  mask;
  MapBucket     *buckets;
  MapHasher     *hasher;
  MapComparator *comparator;
};

void map_init(Map *map, MapHasher *hasher, MapComparator *comparator);
void map_init_with_capacity(Map *map, unsigned long capacity, MapHasher *hasher, MapComparator *comparator);
void map_deinit(Map *map);

void  map_iterator(MapIterator *iterator, Map *map);
int   map_iterator_next(MapIterator *iterator);
void *map_iterator_key(MapIterator *iterator);
void *map_iterator_value(MapIterator *iterator);
void  map_iterator_update(MapIterator *iterator, void *value);

void *map_entry_key(MapEntry *entry);
void *map_entry_value(MapEntry *entry);
void  map_entry_update(MapEntry *entry, void *value);
void  map_entry_erase(MapEntry *entry);

unsigned long map_size(Map *map);
int           map_find(Map *map, void *key, MapEntry *entry);
void         *map_value(Map *map, void *key);
void          map_update(Map *map, void *key, void *value);
void          map_erase(Map *map, void *key);

#endif
