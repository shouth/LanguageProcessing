#include "scanner.h"
#include <stddef.h>
#include <stdio.h>

int scanner_init(scanner_t *sc, char *filename)
{
    sc->file = fopen(filename, "r");
    if (sc->file == NULL) {
        return -1;
    }

    sc->top = fgetc(sc->file);
    sc->next = fgetc(sc->file);

    sc->buf[0] = '\0';
    sc->buf_capacity = sizeof(sc->buf) / sizeof(sc->buf[0]);
    sc->buf_end = 0;
    sc->buf_overflow = 0;

    sc->line_num = 1;
    sc->col_num = 1;
    return 0;
}

int scanner_free(scanner_t *sc)
{
    return fclose(sc->file);
}

void scanner_advance(scanner_t *sc)
{
    if (sc->buf_end + 1 < sc->buf_capacity) {
        sc->buf[sc->buf_end] = sc->top;
        sc->buf_end++;
        sc->buf[sc->buf_end] = '\0';
    } else {
        sc->buf_overflow = 1;
    }
    sc->top = sc->next;
    sc->next = fgetc(sc->file);
    sc->col_num++;
}

void scanner_advance_line(scanner_t *sc)
{
    sc->line_num++;
    sc->col_num = 1;
}

int scanner_top(scanner_t *sc)
{
    return sc->top;
}

int scanner_next(scanner_t *sc)
{
    return sc->next;
}

const char *scanner_buf_data(scanner_t *sc)
{
    return sc->buf;
}

int scanner_buf_overflow(scanner_t *sc)
{
    return sc->buf_overflow;
}

void scanner_clear_buf(scanner_t *sc)
{
    sc->buf[0] = '\0';
    sc->buf_end = 0;
    sc->buf_overflow = 0;
}

size_t scanner_line_number(scanner_t *sc)
{
    return sc->line_num;
}

size_t scanner_col_number(scanner_t *sc)
{
    return sc->col_num;
}
