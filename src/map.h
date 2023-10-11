#ifndef MAP_H
#define MAP_H

typedef unsigned long    MapHasher(const void *);
typedef int              MapComparator(const void *, const void *);
typedef struct MapBucket MapBucket;
typedef struct MapIndex  MapIndex;
typedef struct Map       Map;

struct MapBucket {
  unsigned long hop;
  void         *key;
  void         *value;
};

struct MapIndex {
  unsigned long hash;
  void         *key;
  unsigned long offset;
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
int   map_find(const Map *map, void *key, MapIndex *index);
void *map_value(Map *map, void *key);
void *map_value_at(Map *map, MapIndex *index);
void  map_insert(Map *map, void *key, void *value);
void  map_insert_at(Map *map, MapIndex *index, void *value);
void  map_erase(Map *map, void *key);
void  map_erase_at(Map *map, MapIndex *index);

#endif
