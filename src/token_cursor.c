#include "token_cursor.h"
#include "lexer.h"
#include "syntax_kind.h"
#include "token.h"
#include "vector.h"

static void token_cursor_lex(TokenCursor *cursor)
{
  lexer_lex(cursor->_source + cursor->_offset, cursor->_size - cursor->_offset, &cursor->_info);
  cursor->_offset += cursor->_info.text_length;
}

void token_cursor_init(TokenCursor *cursor, const char *source, unsigned long size)
{
  cursor->_source = source;
  cursor->_size   = size;
  cursor->_offset = 0;
  token_cursor_lex(cursor);
}
int token_cursor_next(TokenCursor *cursor, Token *token)
{
  TokenInfo info;
  Vector    leading_trivia;
  Vector    trailing_trivia;

  if (cursor->_offset == -1ul) {
    return 0;
  }

  vector_init(&leading_trivia, sizeof(TokenInfo));
  vector_init(&trailing_trivia, sizeof(TokenInfo));

  while (syntax_kind_is_trivia(cursor->_info.kind)) {
    vector_push(&leading_trivia, &cursor->_info);
    token_cursor_lex(cursor);
  }

  info = cursor->_info;
  token_cursor_lex(cursor);

  while (syntax_kind_is_trivia(cursor->_info.kind)) {
    vector_push(&trailing_trivia, &cursor->_info);
    token_cursor_lex(cursor);
  }

  token_init(token, &info,
    vector_data(&leading_trivia), vector_length(&leading_trivia),
    vector_data(&trailing_trivia), vector_length(&trailing_trivia));

  vector_deinit(&leading_trivia);
  vector_deinit(&trailing_trivia);

  return 1;
}
