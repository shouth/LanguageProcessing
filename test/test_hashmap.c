#include "utility.h"
#include <stdio.h>

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
  int i;

  HashMap(int, int) map;
  hashmap_alloc(&map, &int_hash, &int_eq);

#define TEST_SIZE 100

  for (i = 0; i < TEST_SIZE; ++i) {
    HashMapEntry entry;
    hashmap_entry(&map, &i, &entry);
    hashmap_update(&map, &entry, &i, &i);
  }

  printf("Size: %lu\n", map.count);

  for (i = 0; i < TEST_SIZE; ++i) {
    HashMapEntry entry;

    if (!hashmap_entry(&map, &i, &entry)) {
      printf("Error: %d\n", i);
    }
  }

  hashmap_free(&map);
}
