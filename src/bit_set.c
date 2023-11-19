#include <string.h>

#include "bit_set.h"

void bit_set_zero(BitSet *bit_set, unsigned long bits)
{
  memset(bit_set, 0, sizeof(BitSet) * bit_set_bits_to_buckets(bits));
}

void bit_set_set(BitSet *bit_set, unsigned long index, int value)
{
  if (value) {
    bit_set[index / BIT_SET_SIZE] |= 1ul << (index % BIT_SET_SIZE);
  } else {
    bit_set[index / BIT_SET_SIZE] &= ~(1ul << (index % BIT_SET_SIZE));
  }
}

int bit_set_get(BitSet *bit_set, unsigned long index)
{
  return (bit_set[index / BIT_SET_SIZE] >> (index % BIT_SET_SIZE)) & 1ul;
}

unsigned long bit_set_count(BitSet *bit_set, unsigned long bits)
{
  unsigned long result = 0;
  unsigned long i;
  bits = BIT_SET_SIZE * bit_set_bits_to_buckets(bits);
  for (i = 0; i < bits; ++i) {
    result += bit_set_get(bit_set, i);
  }
  return result;
}
