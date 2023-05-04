#ifndef SOURCE_H
#define SOURCE_H

#include "utility.h"

typedef struct {
  char *filename;
  char *src;
  long  src_len;
  long *lines;
  long  lines_len;
} source_t;

source_t *src_new(const char *filename);
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

#endif
