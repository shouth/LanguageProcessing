#include "scanner.h"
#include <stddef.h>
#include <stdio.h>

int scanner_init(scanner_t *sc, char *filename)
{
    sc->file = fopen(filename, "r");
    if (sc->file == NULL) {
        return -1;
    }
    sc->filename = malloc(sizeof(filename[0]) * (strlen(filename) + 1));
    strcpy(sc->filename, filename);

    fgetpos(sc->file, &sc->loc.fpos);
    sc->loc.line = 1;
    sc->loc.col = 1;

    sc->top = fgetc(sc->file);
    sc->next = fgetc(sc->file);

    sc->buf[0] = '\0';
    sc->buf_capacity = sizeof(sc->buf) / sizeof(sc->buf[0]);
    sc->buf_end = 0;
    sc->buf_overflow = 0;

    return 0;
}

void scanner_free(scanner_t *sc)
{
    free(sc->filename);
    fclose(sc->file);
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
    sc->loc.col++;
}

void scanner_advance_line(scanner_t *sc)
{
    fgetpos(sc->file, &sc->loc.fpos);
    sc->loc.line++;
    sc->loc.col = 1;
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

const scanner_loc_t *scanner_location(scanner_t *sc)
{
    return &sc->loc;
}