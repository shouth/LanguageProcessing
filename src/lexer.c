#include <stdio.h>
#include <string.h>

#include "lexer.h"
#include "source.h"
#include "syntax_kind.h"
#include "token_tree.h"

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

static int is_alphabet(int c)
{
  return !!strchr("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz", c);
}

static int is_number(int c)
{
  return c >= '0' && c <= '9';
}

static int is_space(int c)
{
  return !!strchr(" \t", c);
}

static int is_newline(int c)
{
  return !!strchr("\r\n", c);
}

static int is_graphic(int c)
{
  return is_alphabet(c) || is_number(c) || is_space(c) || is_newline(c) || !!strchr("!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~", c);
}

static TokenStatus tokenize(Lexer *lexer, SyntaxKind kind, TokenInfo *info)
{
  token_info_init(info, kind, lexer->source->text + lexer->offset, lexer->index);
  lexer->offset += lexer->index;
  lexer->index = 0;
  return TOKEN_OK;
}

static TokenStatus token_unexpected(Lexer *lexer, TokenInfo *info)
{
  tokenize(lexer, SYNTAX_KIND_BAD_TOKEN, info);
  return TOKEN_ERROR_STRAY_CHAR;
}

static TokenStatus token_identifier_and_keyword(Lexer *lexer, TokenInfo *info)
{
  if (eat_if(lexer, &is_alphabet)) {
    SyntaxKind kind;
    while (eat_if(lexer, &is_alphabet) || eat_if(lexer, &is_number)) { }
    kind = syntax_kind_from_keyword(lexer->source->text + lexer->offset, lexer->index);
    return tokenize(lexer, kind != SYNTAX_KIND_BAD_TOKEN ? kind : SYNTAX_KIND_IDENTIFIER_TOKEN, info);
  } else {
    return token_unexpected(lexer, info);
  }
}

static TokenStatus token_integer(Lexer *lexer, TokenInfo *info)
{
  if (eat_if(lexer, &is_number)) {
    while (eat_if(lexer, &is_number)) { }
    return tokenize(lexer, SYNTAX_KIND_INTEGER_LITERAL, info);
  } else {
    return token_unexpected(lexer, info);
  }
}

static TokenStatus token_string(Lexer *lexer, TokenInfo *info)
{
  if (eat(lexer, '\'')) {
    int contain_non_graphic = 0;
    while (1) {
      if (eat(lexer, '\'') && !eat(lexer, '\'')) {
        if (contain_non_graphic) {
          tokenize(lexer, SYNTAX_KIND_BAD_TOKEN, info);
          return TOKEN_ERROR_NONGRAPHIC_CHAR;
        } else {
          return tokenize(lexer, SYNTAX_KIND_STRING_LITERAL, info);
        }
      } else if (is_newline(first(lexer)) || first(lexer) == EOF) {
        tokenize(lexer, SYNTAX_KIND_BAD_TOKEN, info);
        return TOKEN_ERROR_UNTERMINATED_STRING;
      } else {
        contain_non_graphic |= !eat_if(lexer, &is_graphic);
      }
    }
  } else {
    return token_unexpected(lexer, info);
  }
}

static int token_whitespace(Lexer *lexer, TokenInfo *info)
{
  if (eat_if(lexer, &is_space)) {
    while (eat_if(lexer, &is_space)) { }
    return tokenize(lexer, SYNTAX_KIND_SPACE_TRIVIA, info);
  } else if (eat(lexer, '\r')) {
    eat(lexer, '\n');
    return tokenize(lexer, SYNTAX_KIND_NEWLINE_TRIVIA, info);
  } else if (eat(lexer, '\n')) {
    eat(lexer, '\r');
    return tokenize(lexer, SYNTAX_KIND_NEWLINE_TRIVIA, info);
  } else {
    return token_unexpected(lexer, info);
  }
}

static int token_comment(Lexer *lexer, TokenInfo *info)
{
  if (eat(lexer, '{')) {
    while (1) {
      if (eat(lexer, '}')) {
        return tokenize(lexer, SYNTAX_KIND_BRACES_COMMENT_TRIVIA, info);
      } else if (first(lexer) == EOF) {
        tokenize(lexer, SYNTAX_KIND_BAD_TOKEN, info);
        return TOKEN_ERROR_UNTERMINATED_COMMENT;
      } else {
        bump(lexer);
      }
    }
  } else if (eat(lexer, '/')) {
    if (eat(lexer, '*')) {
      while (1) {
        if (eat(lexer, '*') && eat(lexer, '/')) {
          return tokenize(lexer, SYNTAX_KIND_C_COMMENT_TRIVIA, info);
        } else if (first(lexer) == EOF) {
          tokenize(lexer, SYNTAX_KIND_BAD_TOKEN, info);
          return TOKEN_ERROR_UNTERMINATED_COMMENT;
        } else {
          bump(lexer);
        }
      }
    } else {
      return token_unexpected(lexer, info);
    }
  } else {
    return token_unexpected(lexer, info);
  }
}

static int token_symbol(Lexer *lexer, TokenInfo *info)
{
  if (eat(lexer, '+')) {
    return tokenize(lexer, SYNTAX_KIND_PLUS_TOKEN, info);
  } else if (eat(lexer, '-')) {
    return tokenize(lexer, SYNTAX_KIND_MINUS_TOKEN, info);
  } else if (eat(lexer, '*')) {
    return tokenize(lexer, SYNTAX_KIND_STAR_TOKEN, info);
  } else if (eat(lexer, '=')) {
    return tokenize(lexer, SYNTAX_KIND_EQUAL_TOKEN, info);
  } else if (eat(lexer, '<')) {
    if (eat(lexer, '>')) {
      return tokenize(lexer, SYNTAX_KIND_NOT_EQUAL_TOKEN, info);
    } else if (eat(lexer, '=')) {
      return tokenize(lexer, SYNTAX_KIND_LESS_THAN_EQUAL_TOKEN, info);
    } else {
      return tokenize(lexer, SYNTAX_KIND_LESS_THAN_TOKEN, info);
    }
  } else if (eat(lexer, '>')) {
    if (eat(lexer, '=')) {
      return tokenize(lexer, SYNTAX_KIND_GREATER_THAN_EQUAL_TOKEN, info);
    } else {
      return tokenize(lexer, SYNTAX_KIND_GREATER_THAN_TOKEN, info);
    }
  } else if (eat(lexer, '(')) {
    return tokenize(lexer, SYNTAX_KIND_LEFT_PARENTHESIS_TOKEN, info);
  } else if (eat(lexer, ')')) {
    return tokenize(lexer, SYNTAX_KIND_RIGHT_PARENTHESIS_TOKEN, info);
  } else if (eat(lexer, '[')) {
    return tokenize(lexer, SYNTAX_KIND_LEFT_BRACKET_TOKEN, info);
  } else if (eat(lexer, ']')) {
    return tokenize(lexer, SYNTAX_KIND_RIGHT_BRACKET_TOKEN, info);
  } else if (eat(lexer, ':')) {
    if (eat(lexer, '=')) {
      return tokenize(lexer, SYNTAX_KIND_ASSIGN_TOKEN, info);
    } else {
      return tokenize(lexer, SYNTAX_KIND_COLON_TOKEN, info);
    }
  } else if (eat(lexer, '.')) {
    return tokenize(lexer, SYNTAX_KIND_DOT_TOKEN, info);
  } else if (eat(lexer, ',')) {
    return tokenize(lexer, SYNTAX_KIND_COMMA_TOKEN, info);
  } else if (eat(lexer, ';')) {
    return tokenize(lexer, SYNTAX_KIND_SEMICOLON_TOKEN, info);
  } else {
    return token_unexpected(lexer, info);
  }
}

TokenStatus mppl_lex(const Source *source, unsigned long offset, TokenInfo *info)
{
  Lexer lexer;
  lexer.source = source;
  lexer.offset = offset;
  lexer.index  = 0;

  if (first(&lexer) == EOF) {
    tokenize(&lexer, SYNTAX_KIND_EOF_TOKEN, info);
    return TOKEN_EOF;
  } else if (is_alphabet(first(&lexer))) {
    return token_identifier_and_keyword(&lexer, info);
  } else if (is_number(first(&lexer))) {
    return token_integer(&lexer, info);
  } else if (first(&lexer) == '\'') {
    return token_string(&lexer, info);
  } else if (is_space(first(&lexer)) || is_newline(first(&lexer))) {
    return token_whitespace(&lexer, info);
  } else if (first(&lexer) == '{' || first(&lexer) == '/') {
    return token_comment(&lexer, info);
  } else {
    return token_symbol(&lexer, info);
  }
}
