#include <string.h>

#include "syntax_kind.h"
#include "token.h"

typedef struct Lexer Lexer;

struct Lexer {
  TokenContext *_context;
  const char   *_source;
  unsigned long _size;
  unsigned long _index;
};

#define EOS -1

static void bump(Lexer *lexer)
{
  if (lexer->_index < lexer->_size) {
    ++lexer->_index;
  }
}

static int first(Lexer *lexer)
{
  return lexer->_index < lexer->_size ? lexer->_source[lexer->_index] : EOS;
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

static const Token *tokenize(Lexer *lexer, SyntaxKind kind)
{
  return token_context_token(lexer->_context, kind, lexer->_source, lexer->_index);
}

static const Token *token_error(Lexer *lexer)
{
  bump(lexer);
  return tokenize(lexer, SYNTAX_KIND_ERROR);
}

static const Token *token_identifier_and_keyword(Lexer *lexer)
{
  if (eat_if(lexer, &is_alphabet)) {
    SyntaxKind kind;
    while (eat_if(lexer, &is_alphabet) || eat_if(lexer, &is_number)) { }
    kind = syntax_kind_from_keyword(lexer->_source, lexer->_index);
    return tokenize(lexer, kind != SYNTAX_KIND_ERROR ? kind : SYNTAX_KIND_IDENTIFIER);
  } else {
    return token_error(lexer);
  }
}

static const Token *token_integer(Lexer *lexer)
{
  if (eat_if(lexer, &is_number)) {
    while (eat_if(lexer, &is_number)) { }
    return tokenize(lexer, SYNTAX_KIND_INTEGER);
  } else {
    return token_error(lexer);
  }
}

static const Token *token_string(Lexer *lexer)
{
  if (eat(lexer, '\'')) {
    while (1) {
      if (eat(lexer, '\'')) {
        if (!eat(lexer, '\'')) {
          break;
        }
      } else if (is_newline(first(lexer)) || first(lexer) == EOS) {
        break;
      } else if (!eat_if(lexer, &is_graphic)) {
        break;
      }
    }
    return tokenize(lexer, SYNTAX_KIND_STRING);
  } else {
    return token_error(lexer);
  }
}

static const Token *token_whitespace(Lexer *lexer)
{
  if (eat_if(lexer, &is_space)) {
    while (eat_if(lexer, &is_space)) { }
    return tokenize(lexer, SYNTAX_KIND_SPACE);
  } else if (eat(lexer, '\r')) {
    eat(lexer, '\n');
    return tokenize(lexer, SYNTAX_KIND_NEWLINE);
  } else if (eat(lexer, '\n')) {
    eat(lexer, '\r');
    return tokenize(lexer, SYNTAX_KIND_NEWLINE);
  } else {
    return token_error(lexer);
  }
}

static const Token *token_comment(Lexer *lexer)
{
  if (eat(lexer, '{')) {
    while (1) {
      if (eat(lexer, '}')) {
        break;
      } else if (first(lexer) == EOS) {
        break;
      } else {
        bump(lexer);
      }
    }
    return tokenize(lexer, SYNTAX_KIND_BRACES_COMMENT);
  } else if (eat(lexer, '/')) {
    if (eat(lexer, '*')) {
      while (1) {
        if (eat(lexer, '*')) {
          if (eat(lexer, '/')) {
            break;
          }
        } else if (first(lexer) == EOS) {
          break;
        } else {
          bump(lexer);
        }
      }
      return tokenize(lexer, SYNTAX_KIND_C_COMMENT);
    } else {
      return token_error(lexer);
    }
  } else {
    return token_error(lexer);
  }
}

static const Token *token_symbol(Lexer *lexer)
{
  if (eat(lexer, '+')) {
    return tokenize(lexer, SYNTAX_KIND_PLUS);
  } else if (eat(lexer, '-')) {
    return tokenize(lexer, SYNTAX_KIND_MINUS);
  } else if (eat(lexer, '*')) {
    return tokenize(lexer, SYNTAX_KIND_STAR);
  } else if (eat(lexer, '=')) {
    return tokenize(lexer, SYNTAX_KIND_EQUAL);
  } else if (eat(lexer, '<')) {
    if (eat(lexer, '>')) {
      return tokenize(lexer, SYNTAX_KIND_NOT_EQUAL);
    } else if (eat(lexer, '=')) {
      return tokenize(lexer, SYNTAX_KIND_LESS_THAN_EQUAL);
    } else {
      return tokenize(lexer, SYNTAX_KIND_LESS_THAN);
    }
  } else if (eat(lexer, '>')) {
    if (eat(lexer, '=')) {
      return tokenize(lexer, SYNTAX_KIND_GREATER_THAN_EQUAL);
    } else {
      return tokenize(lexer, SYNTAX_KIND_GREATER_THAN);
    }
  } else if (eat(lexer, '(')) {
    return tokenize(lexer, SYNTAX_KIND_LEFT_PARENTHESIS);
  } else if (eat(lexer, ')')) {
    return tokenize(lexer, SYNTAX_KIND_RIGHT_PARENTHESIS);
  } else if (eat(lexer, '[')) {
    return tokenize(lexer, SYNTAX_KIND_LEFT_BRACKET);
  } else if (eat(lexer, ']')) {
    return tokenize(lexer, SYNTAX_KIND_RIGHT_BRACKET);
  } else if (eat(lexer, ':')) {
    if (eat(lexer, '=')) {
      return tokenize(lexer, SYNTAX_KIND_ASSIGN);
    } else {
      return tokenize(lexer, SYNTAX_KIND_EQUAL);
    }
  } else if (eat(lexer, '.')) {
    return tokenize(lexer, SYNTAX_KIND_DOT);
  } else if (eat(lexer, ',')) {
    return tokenize(lexer, SYNTAX_KIND_COMMA);
  } else if (eat(lexer, ';')) {
    return tokenize(lexer, SYNTAX_KIND_SEMICOLON);
  } else {
    return token_error(lexer);
  }
}

const Token *lexer_lex(TokenContext *context, const char *source, unsigned long size)
{
  Lexer lexer;
  lexer._context = context;
  lexer._source  = source;
  lexer._size    = size;
  lexer._index   = 0;

  if (first(&lexer) == EOS) {
    return NULL;
  } else if (is_alphabet(first(&lexer))) {
    return token_identifier_and_keyword(&lexer);
  } else if (is_number(first(&lexer))) {
    return token_integer(&lexer);
  } else if (first(&lexer) == '\'') {
    return token_string(&lexer);
  } else if (is_space(first(&lexer)) || is_newline(first(&lexer))) {
    return token_whitespace(&lexer);
  } else if (first(&lexer) == '{' || first(&lexer) == '/') {
    return token_comment(&lexer);
  } else {
    return token_symbol(&lexer);
  }
}
