/*
 * bs.c -- bitset test
 *
 * SPDX-FileCopyrightText: 2026 Shota Minami
 * SPDX-License-Identifier: Apache-2.0
 */

#include <assert.h>

#include "ds.h"

int main(void)
{
  size_t i;
  bits_t bits[2] = {0};
  bits_t expected[2] = {0x0123456789abcdef, 0xfedcba9876543210};

  for (i = 0; i < sizeof(bits) * CHAR_BIT; ++i) {
    assert(!bits_test(bits, i));
    bits_set(bits, i, 1);
    assert(bits_test(bits, i));
    bits_set(bits, i, 0);
    assert(!bits_test(bits, i));
  }

  for (i = 0; i < sizeof(bits) * CHAR_BIT; ++i) {
    bits_set(bits, i, (expected[i / (sizeof(bits_t) * CHAR_BIT)] >> (i % (sizeof(bits_t) * CHAR_BIT))) & 1);
  }

  for (i = 0; i < sizeof(bits) / sizeof(*bits); ++i) {
    assert(bits[i] == expected[i]);
  }

  return EXIT_SUCCESS;
}
