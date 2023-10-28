#include <string.h>

#include "lexer.h"
#include "symbol.h"
#include "syntax_kind.h"
#include "token.h"

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

static void tokenize(Lexer *lexer, Token *token, SyntaxKind kind)
{
  token->symbol = symbol_from(lexer->_source, lexer->_index);
  token->kind   = kind;

  lexer->_source += lexer->_index;
  lexer->_size -= lexer->_index;
  lexer->_index = 0;
}

static void token_error(Lexer *lexer, Token *token)
{
  bump(lexer);
  tokenize(lexer, token, SYNTAX_KIND_ERROR);
}

static void token_identifier_like(Lexer *lexer, Token *token)
{
  if (eat_if(lexer, &is_alphabet)) {
    while (eat_if(lexer, &is_alphabet) || eat_if(lexer, &is_number)) { }
    tokenize(lexer, token, SYNTAX_KIND_IDENTIFIER);
  } else {
    token_error(lexer, token);
  }
}

static void token_integer(Lexer *lexer, Token *token)
{
  if (eat_if(lexer, &is_number)) {
    while (eat_if(lexer, &is_number)) { }
    tokenize(lexer, token, SYNTAX_KIND_INTEGER);
  } else {
    token_error(lexer, token);
  }
}

static void token_string(Lexer *lexer, Token *token)
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
    tokenize(lexer, token, SYNTAX_KIND_STRING);
  } else {
    token_error(lexer, token);
  }
}

static void token_whitespace(Lexer *lexer, Token *token)
{
  if (eat_if(lexer, &is_space)) {
    while (eat_if(lexer, &is_space)) { }
    tokenize(lexer, token, SYNTAX_KIND_SPACE);
  } else if (eat(lexer, '\r')) {
    eat(lexer, '\n');
    tokenize(lexer, token, SYNTAX_KIND_NEWLINE);
  } else if (eat(lexer, '\n')) {
    eat(lexer, '\r');
    tokenize(lexer, token, SYNTAX_KIND_NEWLINE);
  } else {
    token_error(lexer, token);
  }
}

static void token_comment(Lexer *lexer, Token *token)
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
    tokenize(lexer, token, SYNTAX_KIND_BRACES_COMMENT);
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
      tokenize(lexer, token, SYNTAX_KIND_C_COMMENT);
    } else {
      token_error(lexer, token);
    }
  } else {
    token_error(lexer, token);
  }
}

static void token_symbol(Lexer *lexer, Token *token)
{
  if (eat(lexer, '+')) {
    tokenize(lexer, token, SYNTAX_KIND_PLUS);
  } else if (eat(lexer, '-')) {
    tokenize(lexer, token, SYNTAX_KIND_MINUS);
  } else if (eat(lexer, '*')) {
    tokenize(lexer, token, SYNTAX_KIND_STAR);
  } else if (eat(lexer, '=')) {
    tokenize(lexer, token, SYNTAX_KIND_EQUAL);
  } else if (eat(lexer, '<')) {
    if (eat(lexer, '>')) {
      tokenize(lexer, token, SYNTAX_KIND_NOT_EQUAL);
    } else if (eat(lexer, '=')) {
      tokenize(lexer, token, SYNTAX_KIND_LESS_THAN_EQUAL);
    } else {
      tokenize(lexer, token, SYNTAX_KIND_LESS_THAN);
    }
  } else if (eat(lexer, '>')) {
    if (eat(lexer, '=')) {
      tokenize(lexer, token, SYNTAX_KIND_GREATER_THAN_EQUAL);
    } else {
      tokenize(lexer, token, SYNTAX_KIND_GREATER_THAN);
    }
  } else if (eat(lexer, '(')) {
    tokenize(lexer, token, SYNTAX_KIND_LEFT_PARENTHESIS);
  } else if (eat(lexer, ')')) {
    tokenize(lexer, token, SYNTAX_KIND_RIGHT_PARENTHESIS);
  } else if (eat(lexer, '[')) {
    tokenize(lexer, token, SYNTAX_KIND_LEFT_BRACKET);
  } else if (eat(lexer, ']')) {
    tokenize(lexer, token, SYNTAX_KIND_RIGHT_BRACKET);
  } else if (eat(lexer, ':')) {
    if (eat(lexer, '=')) {
      tokenize(lexer, token, SYNTAX_KIND_ASSIGN);
    } else {
      tokenize(lexer, token, SYNTAX_KIND_EQUAL);
    }
  } else if (eat(lexer, '.')) {
    tokenize(lexer, token, SYNTAX_KIND_DOT);
  } else if (eat(lexer, ',')) {
    tokenize(lexer, token, SYNTAX_KIND_COMMA);
  } else if (eat(lexer, ';')) {
    tokenize(lexer, token, SYNTAX_KIND_SEMICOLON);
  } else {
    token_error(lexer, token);
  }
}

void lexer_init(Lexer *lexer, const char *source, long size)
{
  lexer->_source = source;
  lexer->_size   = size;
  lexer->_index  = 0;
}

int lexer_next_token(Lexer *lexer, Token *token)
{
  if (lexer->_index >= lexer->_size) {
    return 0;
  } else {
    if (is_alphabet(first(lexer))) {
      token_identifier_like(lexer, token);
    } else if (is_number(first(lexer))) {
      token_integer(lexer, token);
    } else if (first(lexer) == '\'') {
      token_string(lexer, token);
    } else if (is_space(first(lexer)) || is_newline(first(lexer))) {
      token_whitespace(lexer, token);
    } else if (first(lexer) == '{' || first(lexer) == '/') {
      token_comment(lexer, token);
    } else {
      token_symbol(lexer, token);
    }
    return 1;
  }
}
