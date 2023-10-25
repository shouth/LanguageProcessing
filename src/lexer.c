#include <string.h>

#include "lexer.h"

#define EOS -1

int TOKEN_trivial(Token *token)
{
  switch (token->kind) {
  case TOKEN_KIND_NEWLINE:
  case TOKEN_KIND_SPACE:
  case TOKEN_KIND_C_COMMENT:
  case TOKEN_KIND_BRACES_COMMENT:
  case TOKEN_KIND_EOF:
    return 1;
  default:
    return 0;
  }
}

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

static void build_token(Lexer *lexer, Token *token, TokenKind kind)
{
  token->kind = kind;
  token->size = lexer->_index;

  lexer->_source += lexer->_index;
  lexer->_size -= lexer->_index;
  lexer->_index = 0;
}

static void token_error(Lexer *lexer, Token *token)
{
  bump(lexer);
  build_token(lexer, token, TOKEN_KIND_ERROR);
}

static void token_identifier(Lexer *lexer, Token *token)
{
  if (eat_if(lexer, &is_alphabet)) {
    while (eat_if(lexer, &is_alphabet) || eat_if(lexer, &is_number)) { }
    build_token(lexer, token, TOKEN_KIND_IDENTIFIER);
  } else {
    token_error(lexer, token);
  }
}

static void token_integer(Lexer *lexer, Token *token)
{
  if (eat_if(lexer, &is_number)) {
    while (eat_if(lexer, &is_number)) { }
    build_token(lexer, token, TOKEN_KIND_INTEGER);
  } else {
    token_error(lexer, token);
  }
}

static void token_string(Lexer *lexer, Token *token)
{
  if (eat(lexer, '\'')) {
    token->terminated = 1;
    while (1) {
      if (eat(lexer, '\'')) {
        if (!eat(lexer, '\'')) {
          break;
        }
      } else if (is_newline(first(lexer)) || first(lexer) == EOS) {
        token->terminated = 0;
        break;
      } else if (!eat_if(lexer, &is_graphic)) {
        token->terminated = 0;
        break;
      }
    }
    build_token(lexer, token, TOKEN_KIND_STRING);
  } else {
    token_error(lexer, token);
  }
}

static void token_whitespace(Lexer *lexer, Token *token)
{
  if (eat_if(lexer, &is_space)) {
    while (eat_if(lexer, &is_space)) { }
    build_token(lexer, token, TOKEN_KIND_SPACE);
  } else if (eat(lexer, '\r')) {
    eat(lexer, '\n');
    build_token(lexer, token, TOKEN_KIND_NEWLINE);
  } else if (eat(lexer, '\n')) {
    eat(lexer, '\r');
    build_token(lexer, token, TOKEN_KIND_NEWLINE);
  } else {
    token_error(lexer, token);
  }
}

static void token_comment(Lexer *lexer, Token *token)
{
  if (eat(lexer, '{')) {
    token->terminated = 1;
    while (1) {
      if (eat(lexer, '}')) {
        break;
      } else if (first(lexer) == EOS) {
        token->terminated = 0;
        break;
      } else {
        bump(lexer);
      }
    }
    build_token(lexer, token, TOKEN_KIND_BRACES_COMMENT);
  } else if (eat(lexer, '/')) {
    if (eat(lexer, '*')) {
      token->terminated = 1;
      while (1) {
        if (eat(lexer, '*')) {
          if (eat(lexer, '/')) {
            break;
          }
        } else if (first(lexer) == EOS) {
          token->terminated = 0;
          break;
        } else {
          bump(lexer);
        }
      }
      build_token(lexer, token, TOKEN_KIND_C_COMMENT);
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
    build_token(lexer, token, TOKEN_KIND_PLUS);
  } else if (eat(lexer, '-')) {
    build_token(lexer, token, TOKEN_KIND_MINUS);
  } else if (eat(lexer, '*')) {
    build_token(lexer, token, TOKEN_KIND_STAR);
  } else if (eat(lexer, '=')) {
    build_token(lexer, token, TOKEN_KIND_EQUAL);
  } else if (eat(lexer, '<')) {
    if (eat(lexer, '>')) {
      build_token(lexer, token, TOKEN_KIND_NOT_EQUAL);
    } else if (eat(lexer, '=')) {
      build_token(lexer, token, TOKEN_KIND_LESS_THAN_EQUAL);
    } else {
      build_token(lexer, token, TOKEN_KIND_LESS_THAN);
    }
  } else if (eat(lexer, '>')) {
    if (eat(lexer, '=')) {
      build_token(lexer, token, TOKEN_KIND_GREATER_THAN_EQUAL);
    } else {
      build_token(lexer, token, TOKEN_KIND_GREATER_THAN);
    }
  } else if (eat(lexer, '(')) {
    build_token(lexer, token, TOKEN_KIND_LEFT_PARENTHESIS);
  } else if (eat(lexer, ')')) {
    build_token(lexer, token, TOKEN_KIND_RIGHT_PARENTHESIS);
  } else if (eat(lexer, '[')) {
    build_token(lexer, token, TOKEN_KIND_LEFT_BRACKET);
  } else if (eat(lexer, ']')) {
    build_token(lexer, token, TOKEN_KIND_RIGHT_BRACKET);
  } else if (eat(lexer, ':')) {
    if (eat(lexer, '=')) {
      build_token(lexer, token, TOKEN_KIND_ASSIGN);
    } else {
      build_token(lexer, token, TOKEN_KIND_EQUAL);
    }
  } else if (eat(lexer, '.')) {
    build_token(lexer, token, TOKEN_KIND_DOT);
  } else if (eat(lexer, ',')) {
    build_token(lexer, token, TOKEN_KIND_COMMA);
  } else if (eat(lexer, ';')) {
    build_token(lexer, token, TOKEN_KIND_SEMICOLON);
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

void lexer_next_token(Lexer *lexer, Token *token)
{
  if (is_alphabet(first(lexer))) {
    token_identifier(lexer, token);
  } else if (is_number(first(lexer))) {
    token_integer(lexer, token);
  } else if (first(lexer) == '\'') {
    token_string(lexer, token);
  } else if (is_space(first(lexer)) || is_newline(first(lexer))) {
    token_whitespace(lexer, token);
  } else if (first(lexer) == '{' || first(lexer) == '/') {
    token_comment(lexer, token);
  } else if (first(lexer) == EOS) {
    build_token(lexer, token, TOKEN_KIND_EOF);
  } else {
    token_symbol(lexer, token);
  }
}

int lexer_eof(Lexer *lexer)
{
  return lexer->_index >= lexer->_size;
}
