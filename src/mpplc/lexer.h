#ifndef MPPLC_LEXER_H
#define MPPLC_LEXER_H

#include "token.h"

typedef struct Lexer Lexer;

void lexer_init(Lexer *lexer, const char *source, long length);
void lexer_next_token(Lexer *lexer, Token *token);

#endif
