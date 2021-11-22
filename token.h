#ifndef TOKEN_H
#define TOKEN_H

#include <stddef.h>

#include "source.h"

typedef enum {
    TOKEN_NAME_OR_KEYWORD,
    TOKEN_NUMBER,
    TOKEN_STRING,
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_STAR,
    TOKEN_EQUAL,
    TOKEN_NOTEQ,
    TOKEN_LE,
    TOKEN_LEEQ,
    TOKEN_GR,
    TOKEN_GREQ,
    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_LSQPAREN,
    TOKEN_RSQPAREN,
    TOKEN_ASSIGN,
    TOKEN_DOT,
    TOKEN_COMMA,
    TOKEN_COLON,
    TOKEN_SEMI,
    TOKEN_BRACES_COMMENT,
    TOKEN_CSTYLE_COMMENT,
    TOKEN_WHITESPACE,
    TOKEN_UNKNOWN,
    TOKEN_EOF
} token_type_t;

typedef struct {
    token_type_t type;

    union token_data {
        struct {
            int terminated : 1;
        } string;

        struct {
            int terminated : 1;
        } braces_comment;

        struct {
            int terminated : 1;
        } cstyle_comment;
    } data;
} token_data_t;

typedef struct {
    const char *ptr;
    size_t len;
    token_data_t data;

    const source_t *src;
    size_t pos;
} token_t;

void token_data_init(token_data_t *data, token_type_t type, ...);

#endif
