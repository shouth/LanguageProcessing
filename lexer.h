#ifndef LEXER_H
#define LEXER_H

#include <ctype.h>

#include "strref.h"
#include "token.h"

#define LEX_SUCCESS 0
#define LEX_FAILURE -1

void lex(const strref_t *strref, token_t *ret);

#endif /* LEXER_H */