#ifndef CURSOL_H
#define CURSOL_H

#include <stdio.h>
#include <stddef.h>

typedef struct {
    size_t init_len;
    const char *ptr;
    size_t len;
} cursol_t;

void cursol_init(cursol_t *cur, const char *src, size_t len);

int cursol_nth(const cursol_t *cur, size_t index);

int cursol_first(const cursol_t *cur);

int cursol_second(const cursol_t *cur);

int cursol_eof(const cursol_t *cur);

void cursol_next(cursol_t *cur);

size_t cursol_consumed(const cursol_t *cur);

#endif
