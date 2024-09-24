/* SPDX-License-Identifier: Apache-2.0 */

#include <stdio.h>
#include <time.h>

#include "util.h"

unsigned long int_hash(const void *key)
{
  return hash_fnv1a(NULL, key, sizeof(int));
}

int int_eq(const void *a, const void *b)
{
  return *(int *) a == *(int *) b;
}

#define TEST_SIZE 20000000

int main(void)
{
  long i;

  struct timespec start, end, diff;

  HashMap(int, int) map;

  {
    hashmap_alloc(&map, &int_hash, &int_eq);

    printf("Insertion Test w/o Reservation ... \n");

    clock_gettime(CLOCK_MONOTONIC, &start);
    for (i = 0; i < TEST_SIZE; ++i) {
      HashMapEntry entry;
      hashmap_entry(&map, &i, &entry);
      hashmap_update(&map, &entry, &i, &i);
    }
    clock_gettime(CLOCK_MONOTONIC, &end);

    diff.tv_sec  = end.tv_sec - start.tv_sec;
    diff.tv_nsec = end.tv_nsec - start.tv_nsec;

    printf("  Elapsed Time: %.3lf ms, %.3lf ns/op\n",
      diff.tv_sec * 1e3 + diff.tv_nsec / 1e6,
      (diff.tv_sec * 1e9 + diff.tv_nsec) / (double) TEST_SIZE);

    hashmap_free(&map);
  }

  printf("\n");

  {
    hashmap_alloc(&map, &int_hash, &int_eq);
    hashmap_reserve(&map, TEST_SIZE);

    printf("Insertion Test w/ Reservation ... \n");

    clock_gettime(CLOCK_MONOTONIC, &start);
    for (i = 0; i < TEST_SIZE; ++i) {
      HashMapEntry entry;
      hashmap_entry(&map, &i, &entry);
      hashmap_update(&map, &entry, &i, &i);
    }
    clock_gettime(CLOCK_MONOTONIC, &end);

    diff.tv_sec  = end.tv_sec - start.tv_sec;
    diff.tv_nsec = end.tv_nsec - start.tv_nsec;

    printf("  Elapsed Time: %.3lf ms, %.3lf ns/op\n",
      diff.tv_sec * 1e3 + diff.tv_nsec / 1e6,
      (diff.tv_sec * 1e9 + diff.tv_nsec) / (double) TEST_SIZE);

    hashmap_free(&map);
  }

  printf("\n");

  {
    unsigned long capacity = 0;

    unsigned long prev_count    = 0;
    unsigned long prev_capacity = 0;

    hashmap_alloc(&map, &int_hash, &int_eq);

    printf("Rehash Test ... \n");

    for (i = 0; i < TEST_SIZE; ++i) {
      HashMapEntry entry;

      prev_count    = map.count;
      prev_capacity = map.metadata.count ? map.metadata.count + HOPSCOTCH_BUCKET_SIZE - 1 : 0;

      hashmap_entry(&map, &i, &entry);
      hashmap_update(&map, &entry, &i, &i);

      capacity = map.metadata.count ? map.metadata.count + HOPSCOTCH_BUCKET_SIZE - 1 : 0;

      if (capacity != prev_capacity) {
        printf("  %lu -> %lu\n", prev_capacity, capacity);
        if (prev_capacity) {
          printf("    Load Factor: %.3lf (%lu / %lu)\n",
            prev_count / (double) prev_capacity, prev_count, prev_capacity);
        } else {
          printf("    Load Factor: N/A\n");
        }
      }
    }
  }

  printf("\n");

  {
    long not_found = 0;

    printf("Integration Test ... ");
    fflush(stdout);

    clock_gettime(CLOCK_MONOTONIC, &start);
    for (i = 0; i < TEST_SIZE; ++i) {
      HashMapEntry entry;
      if (!hashmap_entry(&map, &i, &entry)) {
        ++not_found;
      }
    }
    clock_gettime(CLOCK_MONOTONIC, &end);

    diff.tv_sec  = end.tv_sec - start.tv_sec;
    diff.tv_nsec = end.tv_nsec - start.tv_nsec;

    if (not_found == 0) {
      printf("PASS\n");
    } else {
      printf("FAIL\n");
      printf("  Not Found: %ld\n", not_found);
    }
    printf("  Elapsed Time: %.3lf ms, %.3lf ns/op\n",
      diff.tv_sec * 1e3 + diff.tv_nsec / 1e6,
      (diff.tv_sec * 1e9 + diff.tv_nsec) / (double) TEST_SIZE);

    hashmap_free(&map);
  }
}
