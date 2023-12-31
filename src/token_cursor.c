#include <string.h>

#include "array.h"
#include "lexer.h"
#include "source.h"
#include "syntax_kind.h"
#include "token_cursor.h"
#include "token_tree.h"
#include "utility.h"

static TokenStatus token_cursor_lex(TokenCursor *cursor, LexedToken *lexed)
{
  TokenStatus status = mppl_lex(cursor->_source, cursor->_offset, lexed);
  cursor->_offset += lexed->length;
  if (lexed->kind == SYNTAX_KIND_EOF_TOKEN) {
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
    Array      *trivials = array_new(sizeof(TrivialToken));
    LexedToken  lexed;
    TokenStatus status;

    while (1) {
      TrivialToken trivia;
      status = token_cursor_lex(cursor, &lexed);
      if (!syntax_kind_is_trivia(lexed.kind)) {
        break;
      }
      trivia.kind        = lexed.kind;
      trivia.text_length = lexed.length;
      trivia.text        = xmalloc(sizeof(char) * (lexed.length + 1));
      strncpy(trivia.text, cursor->_source->text + lexed.offset, sizeof(char) * lexed.length);
      trivia.text[lexed.length] = '\0';
      array_push(trivials, &trivia);
    }

    {
      unsigned long trivia_count = array_count(trivials);
      TrivialToken *trivia_data  = array_steal(trivials);
      char         *text         = xmalloc(sizeof(char) * (lexed.length + 1));
      strncpy(text, cursor->_source->text + lexed.offset, sizeof(char) * lexed.length);
      text[lexed.length] = '\0';

      *token = token_new(lexed.kind, text, lexed.length, trivia_data, trivia_count);
    }
    return status;
  }
}
