#ifndef SCANNER_H
#define SCANNER_H

#include <stdio.h>
#include "token-list.h"

typedef struct {
    fpos_t fpos;
    size_t line;
    size_t col;
} scanner_loc_t;

typedef struct {
    FILE *file;
    char *filename;
    int top, next;

    char buf[MAXSTRSIZE];
    size_t buf_capacity;
    size_t buf_end;
    int buf_overflow;

    scanner_loc_t preloc, loc;
} scanner_t;

int scanner_init(scanner_t *sc, char *filename);

void scanner_free(scanner_t *sc);

void scanner_advance(scanner_t *sc);

void scanner_advance_line(scanner_t *sc);

int scanner_top(scanner_t *sc);

int scanner_next(scanner_t *sc);

const char *scanner_buf_data(scanner_t *sc);

int scanner_buf_overflow(scanner_t *sc);

void scanner_clear_buf(scanner_t *sc);

const scanner_loc_t *scanner_pre_location(scanner_t *sc);

const scanner_loc_t *scanner_location(scanner_t *sc);

#endif /* SCANNER_H */
