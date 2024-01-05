#ifndef LEXER_H
#define LEXER_H

#include "source.h"
#include "syntax_kind.h"

typedef struct LexedToken LexedToken;

typedef enum {
  LEX_OK,
  LEX_EOF,
  LEX_ERROR_STRAY_CHAR,
  LEX_ERROR_NONGRAPHIC_CHAR,
  LEX_ERROR_UNTERMINATED_STRING,
  LEX_ERROR_UNTERMINATED_COMMENT
} LexStatus;

struct LexedToken {
  SyntaxKind    kind;
  unsigned long offset;
  unsigned long length;
};

LexStatus mppl_lex(const Source *source, unsigned long offset, LexedToken *token);

#endif
