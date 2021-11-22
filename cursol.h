#ifndef CURSOL_H
#define CURSOL_H

#include <stdio.h>
#include <stddef.h>

#include "source.h"

typedef struct {
    size_t init_len;
    const char *ptr;
    size_t len;

    const source_t *src;
} cursol_t;

void cursol_init(cursol_t *cur, const source_t *src, const char *ptr, size_t len);

int cursol_nth(const cursol_t *cur, size_t index);

int cursol_first(const cursol_t *cur);

int cursol_second(const cursol_t *cur);

int cursol_eof(const cursol_t *cur);

void cursol_next(cursol_t *cur);

size_t cursol_position(const cursol_t *cur);

#endif
