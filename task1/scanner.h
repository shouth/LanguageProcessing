#ifndef SCANNER_H
#define SCANNER_H

#include <stdio.h>
#include "token-list.h"

typedef struct {
    FILE *file;
    int top, next;

    char buf[MAXSTRSIZE];
    size_t buf_capacity;
    size_t buf_end;
    int buf_overflow;

    size_t line_num;
    size_t col_num;
} scanner_t;

int scanner_init(scanner_t *sc, char *filename);

int scanner_free(scanner_t *sc);

void scanner_advance(scanner_t *sc);

void scanner_advance_line(scanner_t *sc);

int scanner_top(scanner_t *sc);

int scanner_next(scanner_t *sc);

const char *scanner_buf_data(scanner_t *sc);

int scanner_buf_overflow(scanner_t *sc);

void scanner_clear_buf(scanner_t *sc);

size_t scanner_line_number(scanner_t *sc);

size_t scanner_col_number(scanner_t *sc);

#endif /* SCANNER_H */
