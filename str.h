#ifndef STR_H
#define STR_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
    const char *data;
    size_t size;
} strref_t;

#define STR_NPOS SIZE_MAX

size_t str_size(strref_t str);

strref_t str_new(const char *data, size_t size);

int str_at(strref_t str, size_t index);

strref_t str_slice(strref_t str, size_t begin, size_t end);

#endif /* STR_H */