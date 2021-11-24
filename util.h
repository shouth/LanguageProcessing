#ifndef UTIL_H
#define UTIL_H

#include <stdint.h>

int is_alphabet(int c);

int is_number(int c);

int is_space(int c);

int is_graphical(int c);

uint64_t msb64(uint64_t n);

size_t popcount64(uint64_t n);

#endif /* UTIL_H */
