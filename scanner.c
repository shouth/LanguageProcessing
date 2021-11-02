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
    if (sc->filename != NULL) {
        strcpy(sc->filename, filename);
    }

    fgetpos(sc->file, &sc->loc.fpos);
    sc->loc.line = 1;
    sc->loc.col = 1;

    sc->lookahead[0] = fgetc(sc->file);
    sc->lookahead[1] = fgetc(sc->file);

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

void scanner_next(scanner_t *sc)
{
    if (sc == NULL) {
        return;
    }
    sc->lookahead[0] = sc->lookahead[1];
    sc->lookahead[1] = fgetc(sc->file);
    sc->loc.col++;
}

void scanner_next_line(scanner_t *sc)
{
    if (sc == NULL) {
        return;
    }
    fgetpos(sc->file, &sc->loc.fpos);
    sc->loc.line++;
    sc->loc.col = 1;
}

int scanner_lookahead_1(const scanner_t *sc)
{
    if (sc == NULL) {
        return EOF;
    }
    return sc->lookahead[0];
}

int scanner_lookahead_2(const scanner_t *sc)
{
    if (sc == NULL) {
        return EOF;
    }
    return sc->lookahead[1];
}

const location_t *scanner_location(const scanner_t *sc)
{
    if (sc == NULL) {
        return NULL;
    }
    return &sc->loc;
}