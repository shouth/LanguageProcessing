#include "utility.h"
#define _FILE_OFFSET_BITS 64

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/stat.h>
#include <sys/types.h>

#include "mppl.h"
#include "source.h"

#if defined(__unix__) || defined(__APPLE__)

static long src_size(const char *filename)
{
  struct stat s;

  assert(filename);
  if (stat(filename, &s) == 0 && S_ISREG(s.st_mode)) {
    return s.st_size;
  } else {
    fprintf(stderr, "error: failed to get file size; maybe `%s` is not a file\n", filename);
    exit(EXIT_FAILURE);
  }
}

#elif defined(_WIN32)

static long src_size(const char *filename)
{
  struct __stat64 s;

  assert(filename);
  if (_stat64(filename, &s) == 0 && (s.st_mode & _S_IFREG)) {
    return s.st_size;
  } else {
    fprintf(stderr, "error: failed to get file size; maybe `%s` is not a file\n", filename);
    exit(EXIT_FAILURE);
  }
}

#else

#define BLOCK_SIZE 4096

static long src_size(const char *filename)
{
  char  buf[BLOCK_SIZE];
  FILE *file;
  long  size;
  long  ret = 0;

  assert(filename);
  file = fopen(filename, "r");
  if (!file) {
    fprintf(stderr, "error: failed to open `%s`\n", filename);
    exit(EXIT_FAILURE);
  }

  setvbuf(file, NULL, _IOFBF, BLOCK_SIZE);
  do {
    size = fread(buf, sizeof(*buf), BLOCK_SIZE, file);
    if (ferror(file)) {
      fprintf(stderr, "error: failed to get file size\n", filename);
      fclose(file);
      exit(EXIT_FAILURE);
    }
    ret += size;
  } while (size == BLOCK_SIZE);

  fclose(file);
  return ret;
}

#undef BLOCK_SIZE

#endif

static const char *src_next_line(const char *str)
{
  assert(str);
  str += strcspn(str, "\r\n");
  if (*str == '\0') {
    return str;
  }
  if (strncmp("\r\n", str, 2) == 0 || strncmp("\n\r", str, 2) == 0) {
    return str + 2;
  }
  return str + 1;
}

source_t *src_new(const char *in_name, const char *out_name)
{
  source_t *src = xmalloc(sizeof(source_t));
  long      in_name_len;
  assert(in_name);

  in_name_len = strlen(in_name);

  src->in_name = xmalloc(sizeof(*src->in_name) * (in_name_len + 1));
  strcpy(src->in_name, in_name);

  if (out_name) {
    src->out_name = xmalloc(sizeof(*src->out_name) * strlen(out_name));
    strcpy(src->out_name, out_name);
  } else {
    if (strncmp(in_name + in_name_len - 4, ".mpl", 4) != 0) {
      fprintf(stderr, "error: filename needs to end with `.mpl`\n");
      exit(EXIT_FAILURE);
    }
    src->out_name = xmalloc(sizeof(*src->out_name) * (in_name_len + 1));
    sprintf(src->out_name, "%.*s.csl", (int) (in_name_len - 4), in_name);
  }

  src->src_len = src_size(in_name);
  src->src     = xmalloc(sizeof(*src->src) * (src->src_len + 1));
  {
    FILE *file = fopen(in_name, "r");
    if (!file) {
      fprintf(stderr, "error: failed to open `%s`\n", in_name);
      exit(EXIT_FAILURE);
    }
    fread(src->src, sizeof(*src->src), src->src_len, file);
    if (ferror(file)) {
      fprintf(stderr, "error: failed to read `%s`\n", in_name);
      exit(EXIT_FAILURE);
    }
    fclose(file);
    src->src[src->src_len] = '\0';
  }

  src->lines_len = 0;
  {
    const char *cur = src->src;
    while (*cur) {
      src->lines_len++;
      cur = src_next_line(cur);
    }
  }

  src->lines = xmalloc(sizeof(*src->lines) * (src->lines_len + 1));
  {
    const char *cur   = src->src;
    long       *lines = src->lines;
    while (*cur) {
      *lines = cur - src->src;
      cur    = src_next_line(cur);
      ++lines;
    }
    *lines = cur - src->src;
  }

  return src;
}

void src_delete(source_t *src)
{
  free(src->in_name);
  free(src->out_name);
  free(src->src);
  free(src->lines);
  free(src);
}

location_t source_location(const source_t *src, long index)
{
  long left, right, middle;
  assert(src && src->lines);

  left  = 0;
  right = src->lines_len;

  while (right - left > 1) {
    middle = (right - left) / 2 + left;

    if (src->lines[middle] <= index) {
      left = middle;
    } else {
      right = middle;
    }
  }

  {
    location_t loc;
    loc.line = left + 1;
    loc.col  = index - src->lines[left] + 1;
    return loc;
  }
}

region_t region_from(long pos, long len)
{
  region_t ret;
  ret.pos = pos;
  ret.len = len;
  return ret;
}

region_t region_unite(region_t a, region_t b)
{
  return region_from(a.pos, b.pos + b.len - a.pos);
}

int region_compare(region_t a, region_t b)
{
  if (a.pos < b.pos) {
    return -1;
  } else if (a.pos > b.pos) {
    return 1;
  } else if (a.pos + a.len < b.pos + b.len) {
    return -1;
  } else if (a.pos + a.len > b.pos + b.len) {
    return 1;
  }
  return 0;
}

int symbol_compare(const symbol_t *lhs, const symbol_t *rhs)
{
  long diff = lhs->len - rhs->len;
  int  ret  = strncmp(lhs->ptr, rhs->ptr, diff < 0 ? lhs->len : rhs->len);
  return ret ? ret : diff;
}

static int symbol_comparator(const void *lhs, const void *rhs)
{
  return !symbol_compare(lhs, rhs);
}

static uint64_t symbol_hasher(const void *ptr)
{
  const symbol_t *s = ptr;
  return fnv1(s->ptr, s->len);
}

symbol_context_t *symbol_context_new(void)
{
  symbol_context_t *ret = xmalloc(sizeof(symbol_context_t));
  ret->table            = hash_new(symbol_comparator, symbol_hasher);
  return ret;
}

void symbol_context_delete(symbol_context_t *context)
{
  if (!context) {
    return;
  }
  hash_delete(context->table, free, NULL);
  free(context);
}

const symbol_t *symbol_intern(symbol_context_t *context, const char *ptr, long len)
{
  const hash_entry_t *entry;
  symbol_t            symbol;
  assert(context && ptr);

  symbol.ptr = ptr;
  symbol.len = len;
  if ((entry = hash_find(context->table, &symbol))) {
    return entry->value;
  } else {
    symbol_t *nsymbol = xmalloc(sizeof(symbol_t));
    *nsymbol          = symbol;
    hash_insert_unsafe(context->table, nsymbol, nsymbol);
    return nsymbol;
  }
}
