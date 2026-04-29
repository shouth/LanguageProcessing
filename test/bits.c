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
  bits_t values[2] = {0x0123456789abcdef, 0xfedcba9876543210};

  for (i = 0; i < sizeof(bits) * CHAR_BIT; ++i) {
    assert(!bits_test(bits, i));
    bits_set(bits, i);
    assert(bits_test(bits, i));
    bits_unset(bits, i);
    assert(!bits_test(bits, i));
  }

  for (i = 0; i < sizeof(bits) * CHAR_BIT; ++i) {
    if ((values[i / (sizeof(bits_t) * CHAR_BIT)] >> (i % (sizeof(bits_t) * CHAR_BIT))) & 1) {
      bits_set(bits, i);
    } else {
      bits_unset(bits, i);
    }
  }

  for (i = 0; i < sizeof(bits) / sizeof(*bits); ++i) {
    assert(bits[i] == values[i]);
  }

  {
    bits_t tmp;

    tmp = values[0];
    bits_and(&tmp, &values[1]);
    assert(tmp == 0x0000000000000000);

    tmp = values[0];
    bits_or(&tmp, &values[1]);
    assert(tmp == 0xffffffffffffffff);
  }

  return EXIT_SUCCESS;
}
