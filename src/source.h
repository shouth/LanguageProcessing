#ifndef SOURCE_H
#define SOURCE_H

#include "utility.h"
#include <stddef.h>

typedef struct {
  char *in_name;
  char *out_name;
  char *src;
  long  src_len;
  long *lines;
  long  lines_len;
} source_t;

source_t *src_new(const char *filename, const char *output);
void      src_delete(source_t *src);

typedef struct {
  long line;
  long col;
} location_t;

location_t source_location(const source_t *src, long index);

typedef struct {
  long pos;
  long len;
} region_t;

region_t region_from(long pos, long len);
region_t region_unite(region_t a, region_t b);
int      region_compare(region_t a, region_t b);

typedef struct {
  long            init_len;
  const char     *ptr;
  long            len;
  const source_t *src;
} cursol_t;

void cursol_init(cursol_t *cur, const source_t *src, const char *ptr, long len);
int  cursol_nth(const cursol_t *cur, long index);
int  cursol_first(const cursol_t *cur);
int  cursol_second(const cursol_t *cur);
int  cursol_eof(const cursol_t *cur);
void cursol_next(cursol_t *cur);
long cursol_position(const cursol_t *cur);

typedef struct {
  const char *ptr;
  long        len;
} symbol_t;

typedef struct {
  hash_t *table;
} symbol_context_t;

int               symbol_compare(const symbol_t *lhs, const symbol_t *rhs);
symbol_context_t *symbol_context_new(void);
void              symbol_context_delete(symbol_context_t *context);
const symbol_t   *symbol_intern(symbol_context_t *context, const char *ptr, long len);

#endif
