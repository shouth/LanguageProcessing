#include "token_cursor.h"
#include "lexer.h"
#include "report.h"
#include "syntax_kind.h"
#include "token.h"
#include "vector.h"

static SyntaxKind token_cursor_lex(TokenCursor *cursor, TokenInfo *info, Report *report)
{
  mppl_lex(cursor->_source, cursor->_offset, cursor->_size, info, report);
  cursor->_offset += info->text_length;
  if (info->kind == SYNTAX_KIND_EOF) {
    cursor->_offset = -1ul;
  }
  return info->kind;
}

void token_cursor_init(TokenCursor *cursor, const char *source, unsigned long size)
{
  cursor->_source = source;
  cursor->_size   = size;
  cursor->_offset = 0;
}

int token_cursor_next(TokenCursor *cursor, Token *token)
{
  if (cursor->_offset == -1ul) {
    return 0;
  } else {
    Vector    trivia;
    TokenInfo info;
    Report    report;
    vector_init(&trivia, sizeof(TokenInfo));
    while (syntax_kind_is_trivia(token_cursor_lex(cursor, &info, &report))) {
      vector_push(&trivia, &info);
    }
    token_init(token, &info, vector_data(&trivia), vector_length(&trivia));
    vector_deinit(&trivia);
    return 1;
  }
}
