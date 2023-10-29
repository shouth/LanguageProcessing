#ifndef TOKEN_CURSOR_H
#define TOKEN_CURSOR_H

#include "token.h"

typedef struct TokenCursor TokenCursor;

struct TokenCursor {
  TokenContext *_context;
  const char   *_source;
  unsigned long _size;
  unsigned long _offset;
};

void         token_cursor_init(TokenCursor *cursor, TokenContext *context, const char *source, unsigned long size);
const Token *token_cursor_next(TokenCursor *cursor);

#endif
