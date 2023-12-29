#ifndef BIT_SET_H
#define BIT_SET_H

#include <limits.h>

typedef struct BitSet BitSet;

BitSet       *bitset_new(unsigned long bits);
void          bitset_free(BitSet *bitset);
int           bitset_set(BitSet *bitset, unsigned long index, int value);
void          bitset_clear(BitSet *bitset);
unsigned long bitset_count(BitSet *bitset);

#endif
