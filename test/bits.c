/*
 * bs.c -- bitset test
 *
 * SPDX-FileCopyrightText: 2026 Shota Minami
 * SPDX-License-Identifier: Apache-2.0
 */

#include <assert.h>

#include "ds.h"

#define ULONG_BIT (sizeof(unsigned long) * CHAR_BIT)

int main(void)
{
  size_t i;
  unsigned long bits[] = {0, 0};
  unsigned long values[] = {0x0123456789abcdef, 0xfedcba9876543210};

  for (i = 0; i < sizeof(bits) * CHAR_BIT; ++i) {
    assert(!bits_test(bits, i));
    bits_set(bits, i);
    assert(bits_test(bits, i));
    bits_unset(bits, i);
    assert(!bits_test(bits, i));
  }

  for (i = 0; i < sizeof(bits) * CHAR_BIT; ++i) {
    if ((values[i / ULONG_BIT] >> (i % ULONG_BIT)) & 1) {
      bits_set(bits, i);
    } else {
      bits_unset(bits, i);
    }
  }

  for (i = 0; i < sizeof(bits) / sizeof(*bits); ++i) {
    assert(bits[i] == values[i]);
  }

  {
    unsigned long tmp;

    tmp = values[0];
    bits_and(&tmp, &values[1]);
    assert(tmp == 0x0000000000000000);

    tmp = values[0];
    bits_or(&tmp, &values[1]);
    assert(tmp == 0xffffffffffffffff);
  }

  assert(bits_count(&values[0]) == 32);
  assert(bits_count(&values[1]) == 32);

  return EXIT_SUCCESS;
}
