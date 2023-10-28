#ifndef LEXER_H
#define LEXER_H

#include "symbol.h"
#include "syntax_kind.h"

typedef struct Token Token;
typedef struct Lexer Lexer;

struct Token {
  SyntaxKind kind;
  Symbol     symbol;
};

struct Lexer {
  const char *_source;
  long        _size;
  long        _index;
};

void lexer_init(Lexer *lexer, const char *source, long size);
int  lexer_next_token(Lexer *lexer, Token *token);
int  lexer_eof(Lexer *lexer);

#endif
