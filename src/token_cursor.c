#include "token_cursor.h"
#include "lexer.h"
#include "token.h"

void token_cursor_init(TokenCursor *cursor, TokenContext *context, const char *source, unsigned long size)
{
  cursor->_context = context;
  cursor->_source  = source;
  cursor->_size    = size;
  cursor->_offset  = 0;
}

const Token *token_cursor_next(TokenCursor *cursor)
{
  const Token *token = lexer_lex(cursor->_context, cursor->_source + cursor->_offset, cursor->_size - cursor->_offset);
  if (token) {
    cursor->_offset += token->size;
  }
  return token;
}
