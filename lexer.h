#ifndef LEXER_H
#define LEXER_H

#include <ctype.h>
#include "scanner.h"

#define LEX_SUCCESS 0
#define LEX_FAILURE -1

typedef struct {
    scanner_t scanner;
    int last_token;

    char buf[MAXSTRSIZE + 10];
    size_t buf_capacity;
    size_t buf_size;

    char string_attr[MAXSTRSIZE];
    int num_attr;
} lexer_t;

int iscrlf(int c);

int isgraphical(int c);

int lexer_init(lexer_t *le, const char *filename);

int lexer_free(lexer_t *le);

int lexer_lookahead(const lexer_t *le);

void lexer_next(lexer_t *le);

const scanner_t *lexer_scanner(const lexer_t *le);

int lexer_num_attr(const lexer_t *le);

const char *lexer_string_attr(const lexer_t *le);

#endif /* LEXER_H */