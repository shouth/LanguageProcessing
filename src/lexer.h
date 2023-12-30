#ifndef LEXER_H
#define LEXER_H

#include "source.h"
#include "token_tree.h"

TokenStatus mppl_lex(const Source *source, unsigned long offset, TokenInfo *info);

#endif
