#ifndef LEXER_H
#define LEXER_H

#include "token.h"

typedef struct {
  const source_t *src;
  long            pos;
} lexer_t;

void lexer_init(lexer_t *lexer, const source_t *src);
void lex_token(lexer_t *lexer, token_t *ret);

#endif
