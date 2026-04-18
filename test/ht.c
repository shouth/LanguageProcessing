/*
 * ht.c -- hash table test
 *
 * SPDX-FileCopyrightText: 2026 Shota Minami
 * SPDX-License-Identifier: Apache-2.0
 */

#define _POSIX_C_SOURCE 200809L

#include <assert.h>
#include <limits.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "ds.h"

#define SIZE 10000000

hash_t hash(void const *x)
{
  hash_t h;
  hash_init(&h);
  hash_add(&h, x, sizeof(unsigned long));
  return h;
}

int eq(void const *l, void const *r)
{
  return *(unsigned long const *) l == *(unsigned long const *) r;
}

unsigned long *make_shuffled(size_t n)
{
  unsigned long *a = malloc(sizeof(unsigned long) * n);
  size_t i;

  for (i = 0; i < n; ++i) {
    a[i] = i;
  }
  for (i = 0; i + 1 < n; ++i) {
    size_t j = rand() % (n - (i + 1)) + i;
    unsigned long t = a[i];
    a[i] = a[j];
    a[j] = t;
  }
  return a;
}

int main(void)
{
  unsigned long i;
  struct ht_entry e;

  unsigned long *keys, *values;
  hm(unsigned long, unsigned long) m;

  struct timespec start, end;
  unsigned long ns;

  keys = make_shuffled(SIZE);
  values = make_shuffled(SIZE);

  ht_init(&m, hash, eq);

  clock_gettime(CLOCK_MONOTONIC, &start);

  for (i = 0; i < SIZE; ++i) {
    ht_entry(&m, &keys[i], &e);
    ht_occupy(&m, &e, &keys[i]);
    ht_at(&m, &e)->value = values[i];
  }

  clock_gettime(CLOCK_MONOTONIC, &end);
  ns = (end.tv_sec - start.tv_sec) * 1000000000UL + (end.tv_nsec - start.tv_nsec);
  printf("insert:\n  total: %lu ns\n  average: %lu ns\n", ns, ns / SIZE);

  assert(m.count == SIZE);

  clock_gettime(CLOCK_MONOTONIC, &start);

  for (i = 0; i < SIZE; ++i) {
    if (ht_entry(&m, &keys[i], &e)) {
      if (ht_at(&m, &e)->value != values[i]) {
        fprintf(stderr, "error: %lu != %lu\n", ht_at(&m, &e)->value, values[i]);
        return EXIT_FAILURE;
      }
    } else {
      fprintf(stderr, "error: key %lu not found\n", keys[i]);
      return EXIT_FAILURE;
    }
  }

  clock_gettime(CLOCK_MONOTONIC, &end);
  ns = (end.tv_sec - start.tv_sec) * 1000000000UL + (end.tv_nsec - start.tv_nsec);
  printf("lookup:\n  total: %lu ns\n  average: %lu ns\n", ns, ns / SIZE);

  ht_deinit(&m);
  free(keys);
  free(values);

  return EXIT_SUCCESS;
}
