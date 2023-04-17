#define _POSIX_SOURCE

#include <assert.h>
#include <memory.h>
#include <stdio.h>
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

unsigned long fnv1a(const void *ptr, long len)
{
  unsigned long hash  = 0x811C9DC5;
  unsigned long prime = 0x01000193;
  long          i;

  for (i = 0; i < len; i++) {
    hash ^= ((char *) ptr)[i];
    hash *= prime;
  }
  return hash;
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

int bit_popcount(unsigned long n)
{
  int count = 0;
  while (n) {
    ++count;
    n &= n - 1;
  }
  return count;
}

int bit_right_most(unsigned long n)
{
  return bit_popcount((n & (~n + 1)) - 1);
}

int bit_left_most(unsigned long n)
{
  int count = 0;
  for (; n; n >>= 1) {
    ++count;
  }
  return count;
}

#endif

#if defined(__unix__) || defined(__APPLE__)

#include <unistd.h>

static int term_check(FILE *stream)
{
  return isatty(fileno(stream));
}

#elif defined(_WIN32)

#include <io.h>

int term_check(FILE *stream)
{
  return _isatty(_fileno(stream));
}

#else

int term_check(FILE *stream)
{
  /* do nothing */
  (void) stream;
  return 0;
}

#endif

static int ansi_stdout = 0;
static int ansi_stderr = 0;

void term_ansi_stdout(int flag)
{
  ansi_stdout = flag == -1 ? term_check(stdout) : flag;
}

void term_ansi_stderr(int flag)
{
  ansi_stderr = flag == -1 ? term_check(stderr) : flag;
}

void term_set(unsigned long code)
{
  if (ansi_stdout) {
    if (code & SGR__FLAG) {
      printf("\033[%ldm", code ^ SGR__FLAG);
    } else {
      printf("\033[38;2;%ld;%ld;%ldm", (code >> 16) & 0xFF, (code >> 8) & 0xFF, code & 0xFF);
    }
  }
}
