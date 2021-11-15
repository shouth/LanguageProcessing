#ifndef strref_H
#define strref_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
    const char *data;
    size_t size;
} strref_t;

#define STRREF_NPOS SIZE_MAX

size_t strref_size(strref_t str);

int strref_empty(strref_t str);

strref_t strref_new(const char *data, size_t size);

int strref_at(strref_t str, size_t index);

strref_t strref_slice(strref_t str, size_t begin, size_t end);

#endif /* strref_H */