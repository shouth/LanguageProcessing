#ifndef SOURCE_H
#define SOURCE_H

#include "strref.h"

typedef struct {
    char *filename;

    char *strref_ptr;
    size_t strref_size;

    size_t *lines_ptr;
    size_t lines_size;
} source_t;

source_t *source_new(const char *filename);

void source_free(source_t *src);

void source_str(source_t *src, strref_t *strref);

size_t source_line_at(source_t *src, size_t index);

#endif /* SOURCE_H */