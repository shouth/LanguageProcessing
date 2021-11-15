#ifndef SOURCE_H
#define SOURCE_H

#include "str.h"

typedef struct {
    char *filename;

    char *str_ptr;
    stroff_t str_size;

    stroff_t *lines_ptr;
    size_t lines_size;
} source_t;

source_t *source_new(const char *filename);

int source_free(source_t *src);

#endif /* SOURCE_H */