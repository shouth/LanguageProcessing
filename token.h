#ifndef TOKEN_H
#define TOKEN_H

#include "strref.h"

typedef enum {
    TOKNE_NAME,
    TOKEN_PROGRAM,
    TOKEN_VAR,
    TOKEN_ARRAY,
    TOKEN_OF,
    TOKEN_BEGIN,
    TOKEN_END,
    TOKEN_IF,
    TOKEN_THEN,
    TOKEN_ELSE,
    TOKEN_PROCEDURE,
    TOKEN_RETURN,
    TOKEN_CALL,
    TOKEN_WHILE,
    TOKEN_DO,
    TOKEN_NOT,
    TOKEN_OR,
    TOKEN_DIV,
    TOKEN_AND,
    TOKEN_CHAR,
    TOKEN_INTEGER,
    TOKEN_BOOLEAN,
    TOKEN_READLN,
    TOKEN_WRITELN,
    TOKEN_TRUE,
    TOKEN_FALSE,
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
    TOKEN_READ,
    TOKEN_WRITE,
    TOKEN_BREAK,
    TOKEN_BRACES_COMMENT,
    TOKEN_CSTYLE_COMMENT,
} token_type_t;

typedef struct {
    token_type_t type : 8;
    strref_t strref;

    union {
        struct {
            int terminated : 1;
        } string;

        struct {
            int terminated : 1;
        } braces_comment;

        struct {
            int terminated : 1;
        } cstyle_comment;
    };
} token_t;

void token_init(token_t *token, token_type_t type, ...);

#endif