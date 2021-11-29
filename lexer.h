#ifndef LEXER_H
#define LEXER_H

#include "cursol.h"
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

typedef union {
    struct {
        int terminated;
        size_t len;
        size_t str_len;
    } string;

    struct {
        int terminated;
    } braces_comment;

    struct {
        int terminated;
    } cstyle_comment;
} token_data_t;

typedef struct {
    const char *ptr;
    size_t len;
    const source_t *src;
    size_t pos;
    token_type_t type;
    token_data_t data;
} token_t;

void lex(cursol_t *cursol, token_t *ret);

#endif /* LEXER_H */
