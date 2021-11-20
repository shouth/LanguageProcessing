#ifndef LEXER_H
#define LEXER_H

#include "token.h"

void lex(const char *src, size_t len, token_t *ret);

#endif /* LEXER_H */
