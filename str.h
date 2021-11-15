#ifndef STR_H
#define STR_H

#include <stddef.h>
#include <stdint.h>

typedef uint64_t stroff_t;

typedef struct {
    const char *data;
    stroff_t size;
} strref_t;

#define STR_NPOS UINT64_MAX

stroff_t str_size(strref_t str);

strref_t str_new(const char *data, stroff_t size);

int str_at(strref_t str, stroff_t index);

strref_t str_slice(strref_t str, stroff_t begin, stroff_t end);

#endif /* STR_H */