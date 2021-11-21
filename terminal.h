#ifndef TERMINAL_H
#define TERMINAL_H

#include <stdint.h>

#include "token.h"

typedef enum {
    TERMINAL_NAME,
    TERMINAL_PROGRAM,
    TERMINAL_VAR,
    TERMINAL_ARRAY,
    TERMINAL_OF,
    TERMINAL_BEGIN,
    TERMINAL_END,
    TERMINAL_IF,
    TERMINAL_THEN,
    TERMINAL_ELSE,
    TERMINAL_PROCEDURE,
    TERMINAL_RETURN,
    TERMINAL_CALL,
    TERMINAL_WHILE,
    TERMINAL_DO,
    TERMINAL_NOT,
    TERMINAL_OR,
    TERMINAL_DIV,
    TERMINAL_AND,
    TERMINAL_CHAR,
    TERMINAL_INTEGER,
    TERMINAL_BOOLEAN,
    TERMINAL_READLN,
    TERMINAL_WRITELN,
    TERMINAL_TRUE,
    TERMINAL_FALSE,
    TERMINAL_NUMBER,
    TERMINAL_STRING,
    TERMINAL_PLUS,
    TERMINAL_MINUS,
    TERMINAL_STAR,
    TERMINAL_EQUAL,
    TERMINAL_NOTEQ,
    TERMINAL_LE,
    TERMINAL_LEEQ,
    TERMINAL_GR,
    TERMINAL_GREQ,
    TERMINAL_LPAREN,
    TERMINAL_RPAREN,
    TERMINAL_LSQPAREN,
    TERMINAL_RSQPAREN,
    TERMINAL_ASSIGN,
    TERMINAL_DOT,
    TERMINAL_COMMA,
    TERMINAL_COLON,
    TERMINAL_SEMI,
    TERMINAL_READ,
    TERMINAL_WRITE,
    TERMINAL_BREAK,
    TERMINAL_NONE,
    TERMINAL_EOF,
} terminal_type_t;

typedef struct {
    terminal_type_t type;

    union {
        struct {
            int32_t value;
        } number;

        struct {
            const char *ptr;
            size_t len;
        } string;
    };
} terminal_data_t;

typedef struct {
    const char *ptr;
    size_t len;
    terminal_data_t data;
} terminal_t;

void terminal_from_token(const token_t *token, terminal_t *terminal);

#endif /* TERMINAL_H */
