#ifndef SCANNER_H
#define SCANNER_H

#include <stdio.h>

typedef struct {
    FILE *file;
    int top, next;
    int line_num;
    int col_num;
} scanner_t;

int scanner_init(scanner_t *si, char *filename);

int scanner_free(scanner_t *si);

void scanner_advance(scanner_t *si);

void scanner_advance_line(scanner_t *si);

int scanner_top(scanner_t *si);

int scanner_next(scanner_t *si);

int scanner_line_number(scanner_t *si);

int scanner_col_number(scanner_t *si);

#endif /* SCANNER_H */
