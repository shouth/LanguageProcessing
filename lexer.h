#ifndef LEXER_H
#define LEXER_H

#include "strref.h"
#include "token.h"

void lex(const strref_t *strref, token_t *ret);

#endif /* LEXER_H */
