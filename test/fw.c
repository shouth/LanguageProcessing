/*
 * fw.c -- fenwick tree test
 *
 * SPDX-FileCopyrightText: 2026 Shota Minami
 * SPDX-License-Identifier: Apache-2.0
 */

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "ds.h"

int main(void)
{
  size_t i;
  size_t tree[10];

  memset(tree, 0, sizeof(tree));
  for (i = 0; i < 10; ++i) {
    tree[i] = i + 1;
  }
  fw_build(tree, 10);
  for (i = 0; i < 10; ++i) {
    printf("%lu ", fw_query(tree, i));
    assert(fw_query(tree, i) == i * (i + 1) / 2);
  }
  printf("%lu\n", fw_query(tree, 10));
  assert(fw_query(tree, 10) == 55);

  memset(tree, 0, sizeof(tree));
  for (i = 0; i < 10; ++i) {
    fw_update(tree, 10, i + 1, i + 1);
  }
  for (i = 0; i < 10; ++i) {
    printf("%lu ", fw_query(tree, i));
    assert(fw_query(tree, i) == i * (i + 1) / 2);
  }
  printf("%lu\n", fw_query(tree, 10));
  assert(fw_query(tree, 10) == 55);

  return EXIT_SUCCESS;
}
