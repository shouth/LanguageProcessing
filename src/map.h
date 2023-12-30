#ifndef MAP_H
#define MAP_H

typedef unsigned long    MapHasher(const void *);
typedef int              MapComparator(const void *, const void *);
typedef struct MapBucket MapBucket;
typedef struct MapIndex  MapIndex;
typedef struct Map       Map;

struct MapIndex {
  MapBucket *_bucket;
  MapBucket *_slot;
};

Map          *map_new(MapHasher *hasher, MapComparator *comparator);
Map          *map_new_with_capacity(unsigned long capacity, MapHasher *hasher, MapComparator *comparator);
void          map_free(Map *map);
unsigned long map_count(Map *map);
void          map_reserve(Map *map, unsigned long capacity);
int           map_find(Map *map, void *key, MapIndex *index);
void          map_index(Map *map, MapIndex *index);
int           map_next(Map *map, MapIndex *index);
void         *map_key(Map *map, MapIndex *index);
void         *map_value(Map *map, MapIndex *index);
void          map_update(Map *map, MapIndex *index, void *key, void *value);
void          map_erase(Map *map, MapIndex *index);

#endif
