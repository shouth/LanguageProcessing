/* SPDX-License-Identifier: Apache-2.0 */

#include <stdio.h>

#include "utility.h"

unsigned long int_hash(const void *key)
{
  return hash_fnv1a(NULL, key, sizeof(int));
}

int int_eq(const void *a, const void *b)
{
  return *(int *) a == *(int *) b;
}

int main(void)
{
  int           i;
  unsigned long prev_capacity = 0;
  unsigned long prev_count    = 0;

  HashMap(int, int *) map;
  hashmap_alloc(&map, &int_hash, &int_eq);

#define TEST_SIZE 20000000

  for (i = 0; i < TEST_SIZE; ++i) {
    int         *value = malloc(sizeof(int));
    HashMapEntry entry;

    hashmap_entry(&map, &i, &entry);
    *value = i;
    hashmap_update(&map, &entry, &i, &value);

    if (prev_capacity > 0 && prev_capacity < map.metadata.hops.count + sizeof(unsigned long) * CHAR_BIT - 1) {
      printf("Reserve: %lu / %lu (%lf)\n", prev_count, prev_capacity, (double) prev_count / prev_capacity);
    }

    prev_count    = map.count;
    prev_capacity = map.metadata.hops.count + sizeof(unsigned long) * CHAR_BIT - 1;
  }

  for (i = 0; i < TEST_SIZE; ++i) {
    HashMapEntry entry;

    if (hashmap_entry(&map, &i, &entry)) {
      free(*hashmap_value(&map, &entry));
    } else {
      printf("Error: %d\n", i);
    }
  }

  hashmap_free(&map);
}
