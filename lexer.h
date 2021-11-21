#ifndef LEXER_H
#define LEXER_H

#include "token.h"
#include "source.h"

typedef struct {
    const source_t *src;
    size_t pos;
} lexer_t;

void lexer_init(lexer_t *lexer, source_t *src);

void lexer_next(lexer_t *lexer, token_t *ret);

#endif /* LEXER_H */
