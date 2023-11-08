#include <stdio.h>
#include <string.h>

#include "syntax_kind.h"
#include "token.h"

typedef struct Lexer Lexer;

struct Lexer {
  const char   *source;
  unsigned long size;
  unsigned long index;
};

static void bump(Lexer *lexer)
{
  if (lexer->index < lexer->size) {
    ++lexer->index;
  }
}

static int first(Lexer *lexer)
{
  return lexer->index < lexer->size ? lexer->source[lexer->index] : EOF;
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

static int tokenize(Lexer *lexer, SyntaxKind kind, TokenInfo *info)
{
  token_info_init(info, kind, lexer->source, lexer->index);
  lexer->source += lexer->index;
  lexer->size -= lexer->index;
  lexer->index = 0;
  return 1;
}

static int token_error(Lexer *lexer, TokenInfo *info)
{
  bump(lexer);
  return tokenize(lexer, SYNTAX_KIND_ERROR, info);
}

static int token_identifier_and_keyword(Lexer *lexer, TokenInfo *info)
{
  if (eat_if(lexer, &is_alphabet)) {
    SyntaxKind kind;
    while (eat_if(lexer, &is_alphabet) || eat_if(lexer, &is_number)) { }
    kind = syntax_kind_from_keyword(lexer->source, lexer->index);
    return tokenize(lexer, kind != SYNTAX_KIND_ERROR ? kind : SYNTAX_KIND_IDENTIFIER, info);
  } else {
    return token_error(lexer, info);
  }
}

static int token_integer(Lexer *lexer, TokenInfo *info)
{
  if (eat_if(lexer, &is_number)) {
    while (eat_if(lexer, &is_number)) { }
    return tokenize(lexer, SYNTAX_KIND_INTEGER, info);
  } else {
    return token_error(lexer, info);
  }
}

static int token_string(Lexer *lexer, TokenInfo *info)
{
  if (eat(lexer, '\'')) {
    while (1) {
      if (eat(lexer, '\'')) {
        if (!eat(lexer, '\'')) {
          break;
        }
      } else if (is_newline(first(lexer)) || first(lexer) == EOF) {
        break;
      } else if (!eat_if(lexer, &is_graphic)) {
        break;
      }
    }
    return tokenize(lexer, SYNTAX_KIND_STRING, info);
  } else {
    return token_error(lexer, info);
  }
}

static int token_whitespace(Lexer *lexer, TokenInfo *info)
{
  if (eat_if(lexer, &is_space)) {
    while (eat_if(lexer, &is_space)) { }
    return tokenize(lexer, SYNTAX_KIND_SPACE, info);
  } else if (eat(lexer, '\r')) {
    eat(lexer, '\n');
    return tokenize(lexer, SYNTAX_KIND_NEWLINE, info);
  } else if (eat(lexer, '\n')) {
    eat(lexer, '\r');
    return tokenize(lexer, SYNTAX_KIND_NEWLINE, info);
  } else {
    return token_error(lexer, info);
  }
}

static int token_comment(Lexer *lexer, TokenInfo *info)
{
  if (eat(lexer, '{')) {
    while (1) {
      if (eat(lexer, '}')) {
        break;
      } else if (first(lexer) == EOF) {
        break;
      } else {
        bump(lexer);
      }
    }
    return tokenize(lexer, SYNTAX_KIND_BRACES_COMMENT, info);
  } else if (eat(lexer, '/')) {
    if (eat(lexer, '*')) {
      while (1) {
        if (eat(lexer, '*')) {
          if (eat(lexer, '/')) {
            break;
          }
        } else if (first(lexer) == EOF) {
          break;
        } else {
          bump(lexer);
        }
      }
      return tokenize(lexer, SYNTAX_KIND_C_COMMENT, info);
    } else {
      return token_error(lexer, info);
    }
  } else {
    return token_error(lexer, info);
  }
}

static int token_symbol(Lexer *lexer, TokenInfo *info)
{
  if (eat(lexer, '+')) {
    return tokenize(lexer, SYNTAX_KIND_PLUS, info);
  } else if (eat(lexer, '-')) {
    return tokenize(lexer, SYNTAX_KIND_MINUS, info);
  } else if (eat(lexer, '*')) {
    return tokenize(lexer, SYNTAX_KIND_STAR, info);
  } else if (eat(lexer, '=')) {
    return tokenize(lexer, SYNTAX_KIND_EQUAL, info);
  } else if (eat(lexer, '<')) {
    if (eat(lexer, '>')) {
      return tokenize(lexer, SYNTAX_KIND_NOT_EQUAL, info);
    } else if (eat(lexer, '=')) {
      return tokenize(lexer, SYNTAX_KIND_LESS_THAN_EQUAL, info);
    } else {
      return tokenize(lexer, SYNTAX_KIND_LESS_THAN, info);
    }
  } else if (eat(lexer, '>')) {
    if (eat(lexer, '=')) {
      return tokenize(lexer, SYNTAX_KIND_GREATER_THAN_EQUAL, info);
    } else {
      return tokenize(lexer, SYNTAX_KIND_GREATER_THAN, info);
    }
  } else if (eat(lexer, '(')) {
    return tokenize(lexer, SYNTAX_KIND_LEFT_PARENTHESIS, info);
  } else if (eat(lexer, ')')) {
    return tokenize(lexer, SYNTAX_KIND_RIGHT_PARENTHESIS, info);
  } else if (eat(lexer, '[')) {
    return tokenize(lexer, SYNTAX_KIND_LEFT_BRACKET, info);
  } else if (eat(lexer, ']')) {
    return tokenize(lexer, SYNTAX_KIND_RIGHT_BRACKET, info);
  } else if (eat(lexer, ':')) {
    if (eat(lexer, '=')) {
      return tokenize(lexer, SYNTAX_KIND_ASSIGN, info);
    } else {
      return tokenize(lexer, SYNTAX_KIND_COLON, info);
    }
  } else if (eat(lexer, '.')) {
    return tokenize(lexer, SYNTAX_KIND_DOT, info);
  } else if (eat(lexer, ',')) {
    return tokenize(lexer, SYNTAX_KIND_COMMA, info);
  } else if (eat(lexer, ';')) {
    return tokenize(lexer, SYNTAX_KIND_SEMICOLON, info);
  } else {
    return token_error(lexer, info);
  }
}

int lexer_lex(const char *source, unsigned long size, TokenInfo *info)
{
  Lexer lexer;
  lexer.source = source;
  lexer.size   = size;
  lexer.index  = 0;

  if (first(&lexer) == EOF) {
    return tokenize(&lexer, SYNTAX_KIND_EOF, info);
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
