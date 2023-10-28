#ifndef LEXER_H
#define LEXER_H

#include "token.h"

typedef struct Lexer Lexer;

struct Lexer {
  const char   *_source;
  unsigned long _size;
  unsigned long _offset;
  unsigned long _index;
};

void lexer_init(Lexer *lexer, const char *source, unsigned long size);
int  lexer_next_token(Lexer *lexer, Token *token);

#endif
