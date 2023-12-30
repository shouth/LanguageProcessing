#include <stdio.h>

#include "lexer.h"
#include "source.h"
#include "syntax_kind.h"
#include "token_tree.h"
#include "utility.h"

typedef struct Lexer Lexer;

struct Lexer {
  const Source *source;
  unsigned long offset;
  unsigned long index;
};

static void bump(Lexer *lexer)
{
  if (lexer->offset + lexer->index < lexer->source->text_length) {
    ++lexer->index;
  }
}

static int first(Lexer *lexer)
{
  return lexer->offset + lexer->index < lexer->source->text_length
    ? lexer->source->text[lexer->offset + lexer->index]
    : EOF;
}

static int eat(Lexer *lexer, int c)
{
  int result = first(lexer) == c;
  if (result) {
    bump(lexer);
  }
  return result;
}

static int eat_if(Lexer *lexer, int (*predicate)(int))
{
  int result = predicate(first(lexer));
  if (result) {
    bump(lexer);
  }
  return result;
}

static TokenStatus tokenize(Lexer *lexer, SyntaxKind kind, LexedToken *lexed)
{
  lexed->kind   = kind;
  lexed->offset = lexer->offset;
  lexed->length = lexer->index;

  lexer->offset += lexer->index;
  lexer->index = 0;
  return TOKEN_OK;
}

static TokenStatus token_unexpected(Lexer *lexer, LexedToken *lexed)
{
  tokenize(lexer, SYNTAX_KIND_BAD_TOKEN, lexed);
  return TOKEN_ERROR_STRAY_CHAR;
}

static TokenStatus token_identifier_and_keyword(Lexer *lexer, LexedToken *lexed)
{
  if (eat_if(lexer, &is_alphabet)) {
    SyntaxKind kind;
    while (eat_if(lexer, &is_alphabet) || eat_if(lexer, &is_number)) { }
    kind = syntax_kind_from_keyword(lexer->source->text + lexer->offset, lexer->index);
    return tokenize(lexer, kind != SYNTAX_KIND_BAD_TOKEN ? kind : SYNTAX_KIND_IDENTIFIER_TOKEN, lexed);
  } else {
    return token_unexpected(lexer, lexed);
  }
}

static TokenStatus token_integer(Lexer *lexer, LexedToken *lexed)
{
  if (eat_if(lexer, &is_number)) {
    while (eat_if(lexer, &is_number)) { }
    return tokenize(lexer, SYNTAX_KIND_INTEGER_LITERAL, lexed);
  } else {
    return token_unexpected(lexer, lexed);
  }
}

static TokenStatus token_string(Lexer *lexer, LexedToken *lexed)
{
  if (eat(lexer, '\'')) {
    int contain_non_graphic = 0;
    while (1) {
      if (eat(lexer, '\'') && !eat(lexer, '\'')) {
        if (contain_non_graphic) {
          tokenize(lexer, SYNTAX_KIND_BAD_TOKEN, lexed);
          return TOKEN_ERROR_NONGRAPHIC_CHAR;
        } else {
          return tokenize(lexer, SYNTAX_KIND_STRING_LITERAL, lexed);
        }
      } else if (is_newline(first(lexer)) || first(lexer) == EOF) {
        tokenize(lexer, SYNTAX_KIND_BAD_TOKEN, lexed);
        return TOKEN_ERROR_UNTERMINATED_STRING;
      } else if (!eat_if(lexer, &is_graphic)) {
        contain_non_graphic = 1;
        bump(lexer);
      }
    }
  } else {
    return token_unexpected(lexer, lexed);
  }
}

static int token_whitespace(Lexer *lexer, LexedToken *lexed)
{
  if (eat_if(lexer, &is_space)) {
    while (eat_if(lexer, &is_space)) { }
    return tokenize(lexer, SYNTAX_KIND_SPACE_TRIVIA, lexed);
  } else if (eat(lexer, '\r')) {
    eat(lexer, '\n');
    return tokenize(lexer, SYNTAX_KIND_NEWLINE_TRIVIA, lexed);
  } else if (eat(lexer, '\n')) {
    eat(lexer, '\r');
    return tokenize(lexer, SYNTAX_KIND_NEWLINE_TRIVIA, lexed);
  } else {
    return token_unexpected(lexer, lexed);
  }
}

static int token_comment(Lexer *lexer, LexedToken *lexed)
{
  if (eat(lexer, '{')) {
    while (1) {
      if (eat(lexer, '}')) {
        return tokenize(lexer, SYNTAX_KIND_BRACES_COMMENT_TRIVIA, lexed);
      } else if (first(lexer) == EOF) {
        tokenize(lexer, SYNTAX_KIND_BAD_TOKEN, lexed);
        return TOKEN_ERROR_UNTERMINATED_COMMENT;
      } else {
        bump(lexer);
      }
    }
  } else if (eat(lexer, '/')) {
    if (eat(lexer, '*')) {
      while (1) {
        if (eat(lexer, '*') && eat(lexer, '/')) {
          return tokenize(lexer, SYNTAX_KIND_C_COMMENT_TRIVIA, lexed);
        } else if (first(lexer) == EOF) {
          tokenize(lexer, SYNTAX_KIND_BAD_TOKEN, lexed);
          return TOKEN_ERROR_UNTERMINATED_COMMENT;
        } else {
          bump(lexer);
        }
      }
    } else {
      return token_unexpected(lexer, lexed);
    }
  } else {
    return token_unexpected(lexer, lexed);
  }
}

static int token_symbol(Lexer *lexer, LexedToken *lexed)
{
  if (eat(lexer, '+')) {
    return tokenize(lexer, SYNTAX_KIND_PLUS_TOKEN, lexed);
  } else if (eat(lexer, '-')) {
    return tokenize(lexer, SYNTAX_KIND_MINUS_TOKEN, lexed);
  } else if (eat(lexer, '*')) {
    return tokenize(lexer, SYNTAX_KIND_STAR_TOKEN, lexed);
  } else if (eat(lexer, '=')) {
    return tokenize(lexer, SYNTAX_KIND_EQUAL_TOKEN, lexed);
  } else if (eat(lexer, '<')) {
    if (eat(lexer, '>')) {
      return tokenize(lexer, SYNTAX_KIND_NOT_EQUAL_TOKEN, lexed);
    } else if (eat(lexer, '=')) {
      return tokenize(lexer, SYNTAX_KIND_LESS_THAN_EQUAL_TOKEN, lexed);
    } else {
      return tokenize(lexer, SYNTAX_KIND_LESS_THAN_TOKEN, lexed);
    }
  } else if (eat(lexer, '>')) {
    if (eat(lexer, '=')) {
      return tokenize(lexer, SYNTAX_KIND_GREATER_THAN_EQUAL_TOKEN, lexed);
    } else {
      return tokenize(lexer, SYNTAX_KIND_GREATER_THAN_TOKEN, lexed);
    }
  } else if (eat(lexer, '(')) {
    return tokenize(lexer, SYNTAX_KIND_LEFT_PARENTHESIS_TOKEN, lexed);
  } else if (eat(lexer, ')')) {
    return tokenize(lexer, SYNTAX_KIND_RIGHT_PARENTHESIS_TOKEN, lexed);
  } else if (eat(lexer, '[')) {
    return tokenize(lexer, SYNTAX_KIND_LEFT_BRACKET_TOKEN, lexed);
  } else if (eat(lexer, ']')) {
    return tokenize(lexer, SYNTAX_KIND_RIGHT_BRACKET_TOKEN, lexed);
  } else if (eat(lexer, ':')) {
    if (eat(lexer, '=')) {
      return tokenize(lexer, SYNTAX_KIND_ASSIGN_TOKEN, lexed);
    } else {
      return tokenize(lexer, SYNTAX_KIND_COLON_TOKEN, lexed);
    }
  } else if (eat(lexer, '.')) {
    return tokenize(lexer, SYNTAX_KIND_DOT_TOKEN, lexed);
  } else if (eat(lexer, ',')) {
    return tokenize(lexer, SYNTAX_KIND_COMMA_TOKEN, lexed);
  } else if (eat(lexer, ';')) {
    return tokenize(lexer, SYNTAX_KIND_SEMICOLON_TOKEN, lexed);
  } else {
    return token_unexpected(lexer, lexed);
  }
}

TokenStatus mppl_lex(const Source *source, unsigned long offset, LexedToken *lexed)
{
  Lexer lexer;
  lexer.source = source;
  lexer.offset = offset;
  lexer.index  = 0;

  if (first(&lexer) == EOF) {
    tokenize(&lexer, SYNTAX_KIND_EOF_TOKEN, lexed);
    return TOKEN_EOF;
  } else if (is_alphabet(first(&lexer))) {
    return token_identifier_and_keyword(&lexer, lexed);
  } else if (is_number(first(&lexer))) {
    return token_integer(&lexer, lexed);
  } else if (first(&lexer) == '\'') {
    return token_string(&lexer, lexed);
  } else if (is_space(first(&lexer)) || is_newline(first(&lexer))) {
    return token_whitespace(&lexer, lexed);
  } else if (first(&lexer) == '{' || first(&lexer) == '/') {
    return token_comment(&lexer, lexed);
  } else {
    return token_symbol(&lexer, lexed);
  }
}
