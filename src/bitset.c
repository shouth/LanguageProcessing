#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include "bitset.h"

#define ULONG_BITS (CHAR_BIT * sizeof(unsigned long))

struct BitSet {
  unsigned long *data;
  unsigned long  bits;
  unsigned long  count;
};

BitSet *bitset_new(unsigned long bits)
{
  BitSet *bitset = malloc(sizeof(BitSet));
  bitset->bits   = bits;
  bitset->count  = bits / ULONG_BITS + 1;
  bitset->data   = calloc(bitset->count, sizeof(unsigned long));
  return bitset;
}

void bitset_free(BitSet *bitset)
{
  if (bitset) {
    free(bitset->data);
    free(bitset);
  }
}

int bitset_set(BitSet *bitset, unsigned long index, int value)
{
  if (value > 0) {
    bitset->data[index / ULONG_BITS] |= 1ul << (index % ULONG_BITS);
    return 1;
  } else if (value == 0) {
    bitset->data[index / ULONG_BITS] &= ~(1ul << (index % ULONG_BITS));
    return 0;
  } else {
    return (bitset->data[index / ULONG_BITS] >> (index % ULONG_BITS)) & 1ul;
  }
}

void bitset_clear(BitSet *bitset)
{
  memset(bitset->data, 0, bitset->count * sizeof(unsigned long));
}

unsigned long bitset_count(BitSet *bitset)
{
  unsigned long result = 0;
  unsigned long i;
  for (i = 0; i < bitset->bits; ++i) {
    result += bitset_set(bitset, i, -1);
  }
  return result;
}
