#include "token_cursor.h"
#include "lexer.h"
#include "report.h"
#include "source.h"
#include "syntax_kind.h"
#include "token.h"
#include "vector.h"

static SyntaxKind token_cursor_lex(TokenCursor *cursor, TokenInfo *info, Report *report)
{
  mppl_lex(cursor->_source, cursor->_offset, info, report);
  cursor->_offset += info->text_length;
  if (info->kind == SYNTAX_KIND_EOF_TOKEN) {
    cursor->_offset = -1ul;
  }
  return info->kind;
}

void token_cursor_init(TokenCursor *cursor, const Source *source)
{
  cursor->_source = source;
  cursor->_offset = 0;
}

int token_cursor_next(TokenCursor *cursor, Token *token, Report *report)
{
  if (cursor->_offset == -1ul) {
    return 0;
  } else {
    Vector    trivia;
    TokenInfo info;

    vector_init(&trivia, sizeof(TokenInfo));
    while (syntax_kind_is_trivia(token_cursor_lex(cursor, &info, report))) {
      vector_push(&trivia, &info);
    }
    token_init(token, &info, vector_data(&trivia), vector_count(&trivia));
    vector_deinit(&trivia);
    return 1;
  }
}

unsigned long token_cursor_offset(TokenCursor *cursor)
{
  return cursor->_offset;
}
