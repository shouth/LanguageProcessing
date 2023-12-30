#include "token_cursor.h"
#include "array.h"
#include "lexer.h"
#include "source.h"
#include "syntax_kind.h"
#include "token_tree.h"

static TokenStatus token_cursor_lex(TokenCursor *cursor, TokenInfo *info)
{
  TokenStatus status = mppl_lex(cursor->_source, cursor->_offset, info);
  cursor->_offset += info->text_length;
  if (info->kind == SYNTAX_KIND_EOF_TOKEN) {
    cursor->_offset = -1ul;
  }
  return status;
}

void token_cursor_init(TokenCursor *cursor, const Source *source)
{
  cursor->_source = source;
  cursor->_offset = 0;
}

TokenStatus token_cursor_next(TokenCursor *cursor, Token **token)
{
  if (cursor->_offset == -1ul) {
    return 0;
  } else {
    Array      *trivia = array_new(sizeof(TokenInfo));
    TokenInfo   info;
    TokenStatus status;

    while (1) {
      status = token_cursor_lex(cursor, &info);
      if (!syntax_kind_is_trivia(info.kind)) {
        break;
      }
      array_push(trivia, &info);
    }
    *token = token_new(&info, array_data(trivia), array_count(trivia));
    array_free(trivia);
    return status;
  }
}
