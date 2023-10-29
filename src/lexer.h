#ifndef LEXER_H
#define LEXER_H

#include "token.h"

const Token *lexer_lex(TokenContext *context, const char *source, unsigned long size);

#endif
