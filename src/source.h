#ifndef SOURCE_H
#define SOURCE_H

#include "utility.h"
#include <stddef.h>

typedef struct {
  char   *input_filename;
  char   *output_filename;
  char   *src_ptr;
  size_t  src_size;
  size_t *lines_ptr;
  size_t  lines_size;
} source_t;

source_t *new_source(const char *filename, const char *output);
void      delete_source(source_t *src);

typedef struct {
  size_t line;
  size_t col;
} location_t;

location_t location_from(size_t line, size_t col);
location_t source_location(const source_t *src, size_t index);

typedef struct {
  size_t pos;
  size_t len;
} region_t;

region_t region_from(size_t pos, size_t len);
region_t region_unite(region_t a, region_t b);
int      region_compare(region_t a, region_t b);

typedef struct {
  size_t          init_len;
  const char     *ptr;
  size_t          len;
  const source_t *src;
} cursol_t;

void   cursol_init(cursol_t *cur, const source_t *src, const char *ptr, size_t len);
int    cursol_nth(const cursol_t *cur, size_t index);
int    cursol_first(const cursol_t *cur);
int    cursol_second(const cursol_t *cur);
int    cursol_eof(const cursol_t *cur);
void   cursol_next(cursol_t *cur);
size_t cursol_position(const cursol_t *cur);

typedef struct {
  const char *ptr;
  size_t      len;
} symbol_t;

typedef struct {
  hash_t *table;
} symbol_storage_t;

int               symbol_compare(const void *lhs, const void *rhs);
symbol_storage_t *new_symbol_storage(void);
void              delete_symbol_storage(symbol_storage_t *storage);
const symbol_t   *symbol_intern(symbol_storage_t *storage, const char *ptr, size_t len);

#endif
