#ifndef SCANNER_H
#define SCANNER_H

#include <stdio.h>
#include "token-list.h"

typedef struct {
    fpos_t fpos;
    size_t line;
    size_t col;
} location_t;

typedef struct {
    FILE *file;
    char *filename;
    int lookahead[2];

    size_t line;
    size_t col;
} scanner_t;

int scanner_init(scanner_t *sc, const char *filename);

void scanner_free(scanner_t *sc);

void scanner_next(scanner_t *sc);

void scanner_next_line(scanner_t *sc);

int scanner_lookahead_1(const scanner_t *sc);

int scanner_lookahead_2(const scanner_t *sc);

int scanner_location(const scanner_t *sc, location_t *loc);

#endif /* SCANNER_H */
