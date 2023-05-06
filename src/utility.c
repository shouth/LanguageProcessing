#define _POSIX_SOURCE

#include <assert.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "types.h"
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

static unsigned long str_type_inner(char *buf, const type_t *type)
{
  unsigned long offset = 0;
  switch (type->kind) {
  case TYPE_INTEGER:
    offset += sprintf(buf + offset, "integer");
    break;
  case TYPE_BOOLEAN:
    offset += sprintf(buf + offset, "boolean");
    break;
  case TYPE_CHAR:
    offset += sprintf(buf + offset, "char");
    break;
  case TYPE_STRING:
    offset += sprintf(buf + offset, "string");
    break;
  case TYPE_ARRAY: {
    const type_array_t *array = (type_array_t *) type;
    offset += sprintf(buf + offset, "array[%ld] of ", array->size);
    offset += str_type_inner(buf + offset, array->base->type);
    break;
  }
  case TYPE_PROCEDURE: {
    const type_procedure_t *proc   = (type_procedure_t *) type;
    const subst_t          *params = proc->params;
    offset += sprintf(buf + offset, "procedure(");
    for (; params; params = params->next) {
      offset += sprintf(buf + offset, params == proc->params ? "" : ", ");
      offset += str_type_inner(buf + offset, params->type);
    }
    offset += sprintf(buf + offset, ")");
    break;
  }
  case TYPE_PROGRAM:
    offset += sprintf(buf + offset, "program");
    break;
  }
  return offset;
}

const char *str_type(const type_t *type)
{
  static char buf[1024];
  str_type_inner(buf, type);
  return buf;
}

unsigned long fnv1a(unsigned long hash, const void *ptr, long len)
{
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
