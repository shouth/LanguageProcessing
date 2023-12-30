#ifndef TOKEN_CURSOR_H
#define TOKEN_CURSOR_H

#include "source.h"
#include "token_tree.h"

typedef struct TokenCursor TokenCursor;

struct TokenCursor {
  const Source *_source;
  unsigned long _offset;
};

void        token_cursor_init(TokenCursor *cursor, const Source *source);
TokenStatus token_cursor_next(TokenCursor *cursor, Token **token);

#endif
