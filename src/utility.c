#include <assert.h>
#include <memory.h>
#include <stdint.h>
#include <stdlib.h>

#include "utility.h"

void *xmalloc(long size)
{
  void *ret = malloc(size);
  if (ret == NULL) {
    fprintf(stderr, "memory allocation failed!");
    exit(EXIT_FAILURE);
  }
  return ret;
}

uint64_t fnv1(const void *ptr, long len)
{
  uint64_t ret   = 0xcbf29ce484222325;
  uint64_t prime = 0x00000100000001b3;
  long     i;

  for (i = 0; i < len; i++) {
    ret *= prime;
    ret ^= ((char *) ptr)[i];
  }
  return ret;
}

#if defined(__GNUC__) || defined(__clang__)

int bit_popcount(unsigned long n)
{
  return __builtin_popcountl(n);
}

int bit_right_most(unsigned long n)
{
  return __builtin_ctzl(n);
}

int bit_left_most(unsigned long n)
{
  return 63 - __builtin_clzl(n);
}

#else

int popcount(unsigned long n)
{
  int count = 0;
  while (n) {
    ++count;
    n ^= n & (~n + 1);
  }
  return count;
}

int right_most_bit(unsigned long n)
{
  return popcount((n & (~n + 1)) - 1);
}

int left_most_bit(unsigned long n)
{
  int count = 0;
  for (; n; n >>= 1) {
    ++count;
  }
  return count;
}

#endif

static int use_ansi = 0;

void term_use_ansi(int flag)
{
  use_ansi = flag;
}

void term_set(unsigned long code)
{
  if (use_ansi) {
    if (code & SGR__FLAG) {
      printf("\033[%ldm", code ^ SGR__FLAG);
    } else {
      printf("\033[38;2;%ld;%ld;%ldm", (code >> 16) & 0xff, (code >> 8) & 0xff, code & 0xff);
    }
  }
}
