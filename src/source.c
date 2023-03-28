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

static size_t source_size(const char *filename)
{
#if defined(__unix__) || defined(__APPLE__)
  struct stat s;

  assert(filename);
  if (stat(filename, &s) == 0 && S_ISREG(s.st_mode)) {
    return s.st_size;
  } else {
    fprintf(stderr, "error: failed to get file size; maybe `%s` is not a file\n", filename);
    exit(1);
  }

#elif defined(_WIN32)
  struct __stat64 s;

  assert(filename);
  if (_stat64(filename, &s) == 0 && (s.st_mode & _S_IFREG)) {
    return s.st_size;
  } else {
    fprintf(stderr, "error: failed to get file size; maybe `%s` is not a file\n", filename);
    exit(1);
  }

#else

#define BLOCK_SIZE 4096

  char   buf[BLOCK_SIZE];
  FILE  *file;
  size_t size;
  size_t ret = 0;

  assert(filename);
  file = fopen(filename, "r");
  if (!file) {
    fprintf(stderr, "error: failed to open `%s`\n", filename);
    exit(1);
  }

  setvbuf(file, NULL, _IOFBF, BLOCK_SIZE);
  do {
    size = fread(buf, sizeof(*buf), BLOCK_SIZE, file);
    if (ferror(file)) {
      fprintf(stderr, "error: failed to get file size\n", filename);
      fclose(file);
      exit(1);
    }
    ret += size;
  } while (size == BLOCK_SIZE);

  fclose(file);
  return ret;

#undef BLOCK_SIZE

#endif
}

static const char *source_next_line(const char *str)
{
  assert(str);
  str += strcspn(str, "\r\n");
  if (str[0] == '\0') {
    return str;
  }
  if (strncmp("\r\n", str, 2) == 0 || strncmp("\n\r", str, 2) == 0) {
    return str + 2;
  }
  return str + 1;
}

source_t *new_source(const char *filename, const char *output)
{
  source_t *src;
  size_t    filename_len;
  assert(filename);

  src                  = new (source_t);
  src->input_filename  = NULL;
  src->output_filename = NULL;
  src->lines_ptr       = NULL;
  src->src_ptr         = NULL;

  filename_len = strlen(filename);

  src->input_filename = new_arr(char, filename_len + 1);
  {
    strcpy(src->input_filename, filename);
  }

  src->output_filename = new_arr(char, (output ? strlen(output) : filename_len) + 1);
  if (output) {
    strcpy(src->output_filename, output);
  } else {
    if (strncmp(filename + filename_len - 4, ".mpl", 4) != 0) {
      fprintf(stderr, "error: filename needs to end with `.mpl`\n");
      exit(1);
    }
    sprintf(src->output_filename, "%.*s.csl", (int) (filename_len - 4), filename);
  }

  src->src_size = source_size(filename);
  src->src_ptr  = new_arr(char, src->src_size + 1);
  {
    FILE *file = fopen(filename, "r");
    if (!file) {
      fprintf(stderr, "error: failed to open `%s`\n", filename);
      exit(1);
    }
    fread(src->src_ptr, sizeof(*src->src_ptr), src->src_size, file);
    if (ferror(file)) {
      fprintf(stderr, "error: failed to read `%s`\n", filename);
      exit(1);
    }
    fclose(file);
    src->src_ptr[src->src_size] = '\0';
  }

  src->lines_size = 0;
  {
    const char *cur;
    for (cur = src->src_ptr; *cur; cur = source_next_line(cur)) {
      src->lines_size++;
    }
  }

  src->lines_ptr = new_arr(size_t, src->lines_size + 1);
  {
    const char *cur;
    size_t      linecnt = 0;
    for (cur = src->src_ptr; *cur; cur = source_next_line(cur)) {
      src->lines_ptr[linecnt] = cur - src->src_ptr;
      linecnt++;
    }
    src->lines_ptr[linecnt] = cur - src->src_ptr;
  }

  return src;
}

void delete_source(source_t *src)
{
  free(src->input_filename);
  free(src->output_filename);
  free(src->src_ptr);
  free(src->lines_ptr);
  free(src);
}

location_t location_from(size_t line, size_t col)
{
  location_t ret;
  ret.line = line;
  ret.col  = col;
  return ret;
}

location_t source_location(const source_t *src, size_t index)
{
  size_t left, right, middle;
  assert(src && src->lines_ptr);

  left  = 0;
  right = src->lines_size;

  while (right - left > 1) {
    middle = (right - left) / 2 + left;

    if (src->lines_ptr[middle] <= index) {
      left = middle;
    } else {
      right = middle;
    }
  }

  return location_from(left + 1, index - src->lines_ptr[left] + 1);
}

region_t region_from(size_t pos, size_t len)
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

void cursol_init(cursol_t *cur, const source_t *src, const char *ptr, size_t len)
{
  assert(cur && src);
  cur->init_len = len;
  cur->ptr      = ptr;
  cur->len      = len;
  cur->src      = src;
}

int cursol_nth(const cursol_t *cur, size_t index)
{
  assert(cur);
  if (index >= cur->len) {
    return EOF;
  }
  return cur->ptr[index];
}

int cursol_first(const cursol_t *cur)
{
  assert(cur);
  return cursol_nth(cur, 0);
}

int cursol_second(const cursol_t *cur)
{
  assert(cur);
  return cursol_nth(cur, 1);
}

int cursol_eof(const cursol_t *cur)
{
  assert(cur);
  return cur->len == 0;
}

void cursol_next(cursol_t *cur)
{
  assert(cur);
  if (!cursol_eof(cur)) {
    cur->ptr++;
    cur->len--;
  }
}

size_t cursol_position(const cursol_t *cur)
{
  assert(cur);
  return cur->init_len - cur->len;
}

int symbol_compare(const void *lhs, const void *rhs)
{
  const symbol_t *l = lhs, *r = rhs;
  size_t          len = l->len < r->len ? l->len : r->len;
  int             ret = strncmp(l->ptr, r->ptr, len);
  return ret ? ret : l->len - r->len;
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

symbol_storage_t *new_symbol_storage(void)
{
  symbol_storage_t *ret = new (symbol_storage_t);
  ret->table            = hash_new(symbol_comparator, symbol_hasher);
  return ret;
}

void delete_symbol_storage(symbol_storage_t *storage)
{
  if (!storage) {
    return;
  }
  hash_delete(storage->table, free, NULL);
  free(storage);
}

const symbol_t *symbol_intern(symbol_storage_t *storage, const char *ptr, size_t len)
{
  const hash_entry_t *entry;
  symbol_t           *ret;
  assert(storage && ptr);

  ret      = new (symbol_t);
  ret->ptr = ptr;
  ret->len = len;
  if (entry = hash_find(storage->table, ret)) {
    free(ret);
    return entry->value;
  }
  hash_insert_unsafe(storage->table, ret, ret);
  return ret;
}
