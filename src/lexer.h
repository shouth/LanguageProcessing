#ifndef LEXER_H
#define LEXER_H

#include "token.h"

int mppl_lex(const char *source, unsigned long size, TokenInfo *info);

#endif
