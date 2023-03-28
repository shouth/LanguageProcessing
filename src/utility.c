#include <assert.h>
#include <memory.h>
#include <stdint.h>
#include <stdlib.h>

#include "utility.h"

void *xmalloc(size_t size)
{
  void *ret = malloc(size);
  if (ret == NULL) {
    fprintf(stderr, "memory allocation failed!");
    exit(1);
  }
  return ret;
}

uint64_t fnv1(const char *ptr, size_t len)
{
  uint64_t ret   = 0xcbf29ce484222325;
  uint64_t prime = 0x00000100000001b3;
  size_t   i;

  for (i = 0; i < len; i++) {
    ret *= prime;
    ret ^= ptr[i];
  }
  return ret;
}

uint64_t fnv1_int(uint64_t value)
{
  char buf[sizeof(value)];
  memcpy(buf, &value, sizeof(value));
  return fnv1(buf, sizeof(value));
}

uint64_t fnv1_ptr(const void *ptr)
{
  char buf[sizeof(ptr)];
  memcpy(buf, &ptr, sizeof(ptr));
  return fnv1(buf, sizeof(ptr));
}

#if defined(__GNUC__) || defined(__clang__)

int popcount(uint64_t n)
{
  return __builtin_popcountll(n);
}

int trailing0(uint64_t n)
{
  return __builtin_ctzll(n);
}

int leading0(uint64_t n)
{
  return 63 - __builtin_clzll(n);
}

#else

int popcount(uint64_t n)
{
  n -= (n >> 1) & 0x5555555555555555;
  n = (n & 0x3333333333333333) + ((n >> 2) & 0x3333333333333333);
  n = (n + (n >> 4)) & 0x0f0f0f0f0f0f0f0f;
  n += n >> 8;
  n += n >> 16;
  n += n >> 32;
  return n & 0x000000000000007f;
}

int trailing0(uint64_t)
{
  return popcount((n & (~n + 1)) - 1);
}

int leading0(uint64_t n)
{
  n |= n >> 1;
  n |= n >> 2;
  n |= n >> 4;
  n |= n >> 8;
  n |= n >> 16;
  n |= n >> 32;
  return popcount(n) - 1;
}

#endif

static int use_ansi = 0;

void console_ansi(int flag)
{
  use_ansi = flag;
}

void console_set(sgr_t code)
{
  if (use_ansi) {
    printf("\033[%dm", code);
  }
}

void console_reset(void)
{
  console_set(SGR_RESET);
}

void console_24bit(color_t color)
{
  if (use_ansi) {
    printf("\033[38;2;%ld;%ld;%ldm", (color >> 16) & 0xff, (color >> 8) & 0xff, color & 0xff);
  }
}
