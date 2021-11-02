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

    sc->line = 1;
    sc->col = 1;

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
    sc->col++;
}

void scanner_next_line(scanner_t *sc)
{
    if (sc == NULL) {
        return;
    }
    sc->line++;
    sc->col = 1;
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

int scanner_location(const scanner_t *sc, location_t *loc)
{
    if (sc == NULL || loc == NULL) {
        return -1;
    }
    loc->file = sc->file;
    loc->filename = sc->filename;
    fgetpos(loc->file, &loc->fpos);
    loc->line = sc->line;
    loc->col = sc->col;
    return 0;
}
