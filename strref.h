#ifndef STRREF_H
#define STRREF_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
    const char *data;
    size_t size;
} strref_t;

#define STRREF_NPOS SIZE_MAX

const char *strref_data(const strref_t *strref);

size_t strref_size(const strref_t *strref);

int strref_empty(const strref_t *strref);

void strref_init(strref_t *strref, const char *data, size_t size);

int strref_at(const strref_t *strref, size_t index);

void strref_slice(const strref_t *strref, strref_t *ret, size_t begin, size_t end);

#endif /* STRREF_H */