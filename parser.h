#ifndef PARSER_H
#define PARSER_H

#include <stdarg.h>

#define PARSE_SUCCESS 0
#define PARSE_FAILURE -1

typedef int (parser_cb_t)(lexer_t *le, va_list args);

typedef struct {
    lexer_t lexer;
    parser_cb_t *on_success;
    parser_cb_t *on_failure;
} parser_t;

int parser_init(parser_t *pa, const char *filename, parser_cb_t *on_success, parser_cb_t *on_failure);

void parser_free(parser_t *pa);

#endif /* PARSER_H */