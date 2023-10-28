#ifndef TOKEN_H
#define TOKEN_H

#include "symbol.h"
#include "syntax_kind.h"

typedef struct Token Token;

struct Token {
  SyntaxKind kind;
  Symbol     symbol;
};

#endif
