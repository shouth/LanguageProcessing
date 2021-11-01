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

int scanner_init(scanner_t *sc, const char *filename);

void scanner_free(scanner_t *sc);

void scanner_consume(scanner_t *sc);

void scanner_newline(scanner_t *sc);

int scanner_top(const scanner_t *sc);

int scanner_next(const scanner_t *sc);

const char *scanner_buf_data(const scanner_t *sc);

size_t scanner_buf_size(const scanner_t *sc);

int scanner_buf_overflow(const scanner_t *sc);

void scanner_clear_buf(scanner_t *sc);

const scanner_loc_t *scanner_pre_location(const scanner_t *sc);

const scanner_loc_t *scanner_location(const scanner_t *sc);

#endif /* SCANNER_H */
