#ifndef BIT_SET_H
#define BIT_SET_H

#include <limits.h>

typedef unsigned long BitSet;

#define BIT_SET_SIZE \
  (CHAR_BIT * sizeof(BitSet))

#define bit_set_bits_to_buckets(bits) \
  (((bits) + BIT_SET_SIZE - 1) / BIT_SET_SIZE)

void          bit_set_zero(BitSet *bit_set, unsigned long bits);
void          bit_set_set(BitSet *bit_set, unsigned long index, int value);
int           bit_set_get(BitSet *bit_set, unsigned long index);
unsigned long bit_set_count(BitSet *bit_set, unsigned long bits);

#endif
