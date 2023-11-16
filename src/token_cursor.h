#ifndef TOKEN_CURSOR_H
#define TOKEN_CURSOR_H

#include "report.h"
#include "token.h"

typedef struct TokenCursor TokenCursor;

struct TokenCursor {
  const char   *_source;
  unsigned long _size;
  unsigned long _offset;
};

void          token_cursor_init(TokenCursor *cursor, const char *source, unsigned long size);
int           token_cursor_next(TokenCursor *cursor, Token *token, Report *report);
unsigned long token_cursor_offset(TokenCursor *cursor);

#endif
