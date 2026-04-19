/*
 * vec.c -- vector test
 *
 * SPDX-FileCopyrightText: 2026 Shota Minami
 * SPDX-License-Identifier: Apache-2.0
 */

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#include "ds.h"

#define SIZE 1000000

int main(void)
{
  unsigned long i;
  unsigned long *values;

  vec(unsigned long) v;

  values = malloc(sizeof(unsigned long) * SIZE);
  for (i = 0; i < SIZE; ++i) {
    values[i] = i;
  }
  for (i = 0; i + 1 < SIZE; ++i) {
    unsigned long j = rand() % (SIZE - (i + 1)) + i;
    unsigned long t = values[i];
    values[i] = values[j];
    values[j] = t;
  }
  for (i = 0; i < SIZE; ++i) {
    vec_push(&v, &values[i]);
  }

  assert(v.count == SIZE);

  for (i = 0; i < SIZE; ++i) {
    if (*vec_at(&v, i) != values[i]) {
      fprintf(stderr, "error: expected %lu, got %lu\n", values[i], *vec_at(&v, i));
      return EXIT_FAILURE;
    }
  }

  vec_deinit(&v);
  free(values);

  return EXIT_SUCCESS;
}
