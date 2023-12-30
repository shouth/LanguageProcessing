#ifndef LEXER_H
#define LEXER_H

#include "source.h"
#include "syntax_kind.h"
#include "token_tree.h"

typedef struct LexedToken LexedToken;

struct LexedToken {
  SyntaxKind    kind;
  unsigned long offset;
  unsigned long length;
};

TokenStatus mppl_lex(const Source *source, unsigned long offset, LexedToken *token);

#endif
