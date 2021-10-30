#ifndef SCANNER_ERROR_H
#define SCANNER_ERROR_H

#include "scanner.h"

typedef enum {
    SCAN_WARNING,
    SCAN_ERROR
} scan_message_t;

void message(const scanner_t *sc, const scanner_loc_t *loc, const scan_message_t type, const char *format, ...);
void message_warning(const scanner_t *sc, const scanner_loc_t *loc, const char *format, ...);
void message_error(const scanner_t *sc, const scanner_loc_t *loc, const char *format, ...);

void message_token(const scanner_t *sc, const scanner_loc_t *begin, const scanner_loc_t *end, const scan_message_t type, const char *format, ...);
void message_token_warning(const scanner_t *sc, const scanner_loc_t *begin, const scanner_loc_t *end, const char *format, ...);
void message_token_error(const scanner_t *sc, const scanner_loc_t *begin, const scanner_loc_t *end, const char *format, ...);

#endif /* SCANNER_ERROR_H */