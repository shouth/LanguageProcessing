/*
   Copyright 2022 Shota Minami

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

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
int           map_entry(Map *map, void *key, MapIndex *index);
void          map_iterator(Map *map, MapIndex *index);
int           map_next(Map *map, MapIndex *index);
void         *map_key(Map *map, MapIndex *index);
void         *map_value(Map *map, MapIndex *index);
void          map_update(Map *map, MapIndex *index, void *key, void *value);
void          map_erase(Map *map, MapIndex *index);

#endif
