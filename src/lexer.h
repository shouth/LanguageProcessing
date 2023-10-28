#ifndef LEXER_H
#define LEXER_H

#include "token.h"

typedef struct Lexer Lexer;

struct Lexer {
  const char *_source;
  long        _size;
  long        _index;
};

void lexer_init(Lexer *lexer, const char *source, long size);
int  lexer_next_token(Lexer *lexer, Token *token);

#endif
