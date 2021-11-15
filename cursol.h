#ifndef CURSOL_H
#define CURSOL_H

#include <stdio.h>
#include <stddef.h>

#include "strref.h"

typedef struct {
    size_t initial_size;
    strref_t strref;
} cursol_t;

void cursol_init(cursol_t *cur, strref_t *strref);

int cursol_nth(const cursol_t *cur, size_t index);

int cursol_first(const cursol_t *cur);

int cursol_second(const cursol_t *cur);

int cursol_eof(const cursol_t *cur);

void cursol_next(cursol_t *cur);

size_t cursol_consumed(const cursol_t *cur);

#endif