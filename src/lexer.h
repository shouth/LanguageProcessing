#ifndef LEXER_H
#define LEXER_H

#include "token.h"

int lexer_lex(const char *source, unsigned long size, TokenInfo *info);

#endif
