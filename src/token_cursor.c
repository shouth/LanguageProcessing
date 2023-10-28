#include "token_cursor.h"
#include "lexer.h"
#include "symbol.h"
#include "syntax_kind.h"

void token_cursor_init(TokenCursor *cursor, const char *source, unsigned long size)
{
  lexer_init(&cursor->lexer, source, size);
}

static SyntaxKind expand_token_kind(SyntaxKind kind, Symbol symbol)
{
  if (kind == SYNTAX_KIND_IDENTIFIER) {
    switch (symbol) {
    case SYMBOL_KEYWORD_AND:
      return SYNTAX_KIND_KEYWORD_AND;
    case SYMBOL_KEYWORD_ARRAY:
      return SYNTAX_KIND_KEYWORD_ARRAY;
    case SYMBOL_KEYWORD_BEGIN:
      return SYNTAX_KIND_KEYWORD_BEGIN;
    case SYMBOL_KEYWORD_BOOLEAN:
      return SYNTAX_KIND_KEYWORD_BOOLEAN;
    case SYMBOL_KEYWORD_BREAK:
      return SYNTAX_KIND_KEYWORD_BREAK;
    case SYMBOL_KEYWORD_CALL:
      return SYNTAX_KIND_KEYWORD_CALL;
    case SYMBOL_KEYWORD_CHAR:
      return SYNTAX_KIND_KEYWORD_CHAR;
    case SYMBOL_KEYWORD_DIV:
      return SYNTAX_KIND_KEYWORD_DIV;
    case SYMBOL_KEYWORD_DO:
      return SYNTAX_KIND_KEYWORD_DO;
    case SYMBOL_KEYWORD_ELSE:
      return SYNTAX_KIND_KEYWORD_ELSE;
    case SYMBOL_KEYWORD_END:
      return SYNTAX_KIND_KEYWORD_END;
    case SYMBOL_KEYWORD_FALSE:
      return SYNTAX_KIND_KEYWORD_FALSE;
    case SYMBOL_KEYWORD_IF:
      return SYNTAX_KIND_KEYWORD_IF;
    case SYMBOL_KEYWORD_INTEGER:
      return SYNTAX_KIND_KEYWORD_INTEGER;
    case SYMBOL_KEYWORD_NOT:
      return SYNTAX_KIND_KEYWORD_NOT;
    case SYMBOL_KEYWORD_OF:
      return SYNTAX_KIND_KEYWORD_OF;
    case SYMBOL_KEYWORD_OR:
      return SYNTAX_KIND_KEYWORD_OR;
    case SYMBOL_KEYWORD_PROCEDURE:
      return SYNTAX_KIND_KEYWORD_PROCEDURE;
    case SYMBOL_KEYWORD_PROGRAM:
      return SYNTAX_KIND_KEYWORD_PROGRAM;
    case SYMBOL_KEYWORD_READ:
      return SYNTAX_KIND_KEYWORD_READ;
    case SYMBOL_KEYWORD_READLN:
      return SYNTAX_KIND_KEYWORD_READLN;
    case SYMBOL_KEYWORD_RETURN:
      return SYNTAX_KIND_KEYWORD_RETURN;
    case SYMBOL_KEYWORD_THEN:
      return SYNTAX_KIND_KEYWORD_THEN;
    case SYMBOL_KEYWORD_TRUE:
      return SYNTAX_KIND_KEYWORD_TRUE;
    case SYMBOL_KEYWORD_VAR:
      return SYNTAX_KIND_KEYWORD_VAR;
    case SYMBOL_KEYWORD_WHILE:
      return SYNTAX_KIND_KEYWORD_WHILE;
    case SYMBOL_KEYWORD_WRITE:
      return SYNTAX_KIND_KEYWORD_WRITE;
    case SYMBOL_KEYWORD_WRITELN:
      return SYNTAX_KIND_KEYWORD_WRITELN;
    default:
      /* do nothing */
      break;
    }
  }
  return kind;
}

int token_cursor_next(TokenCursor *cursor, Token *token)
{
  int result = lexer_next_token(&cursor->lexer, token);
  if (result) {
    token->kind = expand_token_kind(token->kind, token->symbol);
  }
  return result;
}
