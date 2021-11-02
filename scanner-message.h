#ifndef SCANNER_ERROR_H
#define SCANNER_ERROR_H

#include "scanner.h"

typedef enum {
    SCAN_WARNING,
    SCAN_ERROR
} scan_message_t;

void message(const location_t *loc, const scan_message_t type, const char *format, ...);
void message_warning(const location_t *loc, const char *format, ...);
void message_error(const location_t *loc, const char *format, ...);

void message_token(const location_t *begin, const location_t *end, const scan_message_t type, const char *format, ...);
void message_token_warning(const location_t *begin, const location_t *end, const char *format, ...);
void message_token_error(const location_t *begin, const location_t *end, const char *format, ...);

#endif /* SCANNER_ERROR_H */