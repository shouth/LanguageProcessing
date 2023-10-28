#ifndef TOKEN_CURSOR_H
#define TOKEN_CURSOR_H

#include "lexer.h"
#include "token.h"

typedef struct TokenCursor TokenCursor;

struct TokenCursor {
  Lexer lexer;
};

void token_cursor_init(TokenCursor *cursor, const char *source, unsigned long size);
int  token_cursor_next(TokenCursor *cursor, Token *token);

#endif
