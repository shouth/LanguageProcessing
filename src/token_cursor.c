#include "stddef.h"

#include "lexer.h"
#include "syntax_kind.h"
#include "token.h"
#include "token_cursor.h"
#include "vector.h"

static SyntaxKind token_cursor_lex(TokenCursor *cursor)
{
  lexer_lex(cursor->_source + cursor->_offset, cursor->_size - cursor->_offset, &cursor->_info);
  cursor->_offset += cursor->_info.text_length;
  if (cursor->_info.kind == SYNTAX_KIND_EOF) {
    cursor->_offset = -1ul;
  }
  return cursor->_info.kind;
}

void token_cursor_init(TokenCursor *cursor, const char *source, unsigned long size)
{
  cursor->_source = source;
  cursor->_size   = size;
  cursor->_offset = 0;
}

int token_cursor_next(TokenCursor *cursor, Token *token)
{
  Vector trivia;
  if (cursor->_offset == -1ul) {
    return 0;
  } else {
    vector_init(&trivia, sizeof(TokenInfo));
    while (syntax_kind_is_trivia(token_cursor_lex(cursor))) {
      vector_push(&trivia, &cursor->_info);
    }
    token_init(token, &cursor->_info, vector_data(&trivia), vector_length(&trivia));
    vector_deinit(&trivia);
    return 1;
  }
}
