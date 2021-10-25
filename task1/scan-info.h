#ifndef SCAN_INFO_H
#define SCAN_INFO_H

#include <stdio.h>

typedef struct {
    FILE *file;
    int c0, c1;
    int line_number;
} scan_info_t;

int scan_info_init(scan_info_t *si, char *filename);

int scan_info_free(scan_info_t *si);

void scan_info_advance(scan_info_t *si);

void scan_info_advance_line(scan_info_t *si);

int scan_info_top(scan_info_t *si);

int scan_info_next(scan_info_t *si);

int scan_info_line_number(scan_info_t *si);

#endif /* SCAN_INFO_H */
