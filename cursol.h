#ifndef CURSOL_H
#define CURSOL_H

#include <stdio.h>
#include <stddef.h>

#include "strref.h"

typedef struct {
    size_t initial_size;
    strref_t strref;
} cursol_t;

cursol_t cursol_new(strref_t strref);

int cursol_nth(cursol_t cur, size_t index);

int cursol_first(cursol_t cur);

int cursol_second(cursol_t cur);

int cursol_eof(cursol_t cur);

cursol_t cursol_next(cursol_t cur);

size_t cursol_consumed(cursol_t cur);

#endif