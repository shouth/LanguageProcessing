#include "token_cursor.h"
#include "lexer.h"
#include "syntax_kind.h"
#include "token.h"
#include "vector.h"

void token_cursor_init(TokenCursor *cursor, const char *source, unsigned long size)
{
  cursor->_source = source;
  cursor->_size   = size;
  cursor->_offset = 0;
}

int token_cursor_next(TokenCursor *cursor, Token *token)
{
  TokenInfo info;
  Vector    trivia;

  if (cursor->_offset == -1ul) {
    return 0;
  }

  vector_init(&trivia, sizeof(TokenInfo));
  while (1) {
    lexer_lex(cursor->_source + cursor->_offset, cursor->_size - cursor->_offset, &info);
    cursor->_offset += info.text_length;
    if (!syntax_kind_is_trivia(info.kind)) {
      if (info.kind == SYNTAX_KIND_EOF) {
        cursor->_offset = -1ul;
      }
      vector_fit(&trivia);
      token_init(token, &info, vector_data(&trivia), vector_length(&trivia));
      vector_deinit(&trivia);
      return 1;
    }
    vector_push(&trivia, &info);
  }
}
