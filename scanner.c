#include "scanner.h"
#include <stddef.h>
#include <stdio.h>

int scanner_init(scanner_t *sc, const char *filename)
{
    if (sc == NULL || filename == NULL) {
        return -1;
    }
    sc->file = fopen(filename, "r");
    if (sc->file == NULL) {
        return -1;
    }
    sc->filename = malloc(sizeof(filename[0]) * (strlen(filename) + 1));
    strcpy(sc->filename, filename);

    fgetpos(sc->file, &sc->loc.fpos);
    sc->loc.line = 1;
    sc->loc.col = 1;
    sc->preloc = sc->loc;

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
    if (sc == NULL) {
        return;
    }
    free(sc->filename);
    fclose(sc->file);
}

void scanner_consume(scanner_t *sc)
{
    if (sc == NULL) {
        return;
    }
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

void scanner_newline(scanner_t *sc)
{
    if (sc == NULL) {
        return;
    }
    fgetpos(sc->file, &sc->loc.fpos);
    sc->loc.line++;
    sc->loc.col = 1;
}

int scanner_top(const scanner_t *sc)
{
    if (sc == NULL) {
        return EOF;
    }
    return sc->top;
}

int scanner_next(const scanner_t *sc)
{
    if (sc == NULL) {
        return EOF;
    }
    return sc->next;
}

const char *scanner_buf_data(const scanner_t *sc)
{
    if (sc == NULL) {
        return NULL;
    }
    return sc->buf;
}

size_t scanner_buf_size(const scanner_t *sc)
{
    if (sc == NULL) {
        return 0;
    }
    return sc->buf_end;
}

int scanner_buf_overflow(const scanner_t *sc)
{
    if (sc == NULL) {
        return 0;
    }
    return sc->buf_overflow;
}

void scanner_clear_buf(scanner_t *sc)
{
    if (sc == NULL) {
        return;
    }
    sc->buf[0] = '\0';
    sc->buf_end = 0;
    sc->buf_overflow = 0;
    sc->preloc = sc->loc;
}

const scanner_loc_t *scanner_pre_location(const scanner_t *sc)
{
    if (sc == NULL) {
        return NULL;
    }
    return &sc->preloc;
}

const scanner_loc_t *scanner_location(const scanner_t *sc)
{
    if (sc == NULL) {
        return NULL;
    }
    return &sc->loc;
}