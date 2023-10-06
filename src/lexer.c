#include <string.h>

#include "mpplc/lexer.h"
#include "mpplc/token.h"

#define EOS -1

struct Lexer {
  const char *source;
  long        length;
  long        index;
};

static void bump(Lexer *lexer)
{
  if (lexer->index < lexer->length) {
    ++lexer->index;
  }
}

static int first(Lexer *lexer)
{
  return lexer->index < lexer->length ? lexer->source[lexer->index] : EOS;
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
  token->kind   = kind;
  token->length = lexer->index;

  lexer->source += lexer->index;
  lexer->length -= lexer->index;
  lexer->index = 0;
}

static void token_identifier(Lexer *lexer, Token *token)
{
  if (is_alphabet(first(lexer))) {
    bump(lexer);
    while (is_alphabet(first(lexer)) || is_number(first(lexer))) {
      bump(lexer);
    }
    build_token(lexer, token, TOKEN_KIND_IDENTIFIER);
  } else {
    bump(lexer);
    build_token(lexer, token, TOKEN_KIND_ERROR);
  }
}

static void token_integer(Lexer *lexer, Token *token)
{
  if (is_number(first(lexer))) {
    bump(lexer);
    while (is_number(first(lexer))) {
      bump(lexer);
    }
    build_token(lexer, token, TOKEN_KIND_INTEGER);
  } else {
    bump(lexer);
    build_token(lexer, token, TOKEN_KIND_ERROR);
  }
}

static void token_string(Lexer *lexer, Token *token)
{
  if (first(lexer) == '\'') {
    int terminated = 1;
    bump(lexer);
    while (1) {
      if (first(lexer) == '\'') {
        bump(lexer);
        if (first(lexer) == '\'') {
          bump(lexer);
        } else {
          break;
        }
      } else if (first(lexer) == EOS) {
        terminated = 0;
        break;
      } else if (is_graphic(first(lexer))) {
        bump(lexer);
      }
    }
    build_token(lexer, token, TOKEN_KIND_STRING);
    token->terminated = terminated;
  } else {
    bump(lexer);
    build_token(lexer, token, TOKEN_KIND_ERROR);
  }
}

static void token_whitespace(Lexer *lexer, Token *token)
{
  if (is_space(first(lexer))) {
    bump(lexer);
    while (is_space(first(lexer))) {
      bump(lexer);
    }
    build_token(lexer, token, TOKEN_KIND_SPACE);
  } else if (first(lexer) == '\r') {
    bump(lexer);
    if (first(lexer) == '\n') {
      bump(lexer);
    }
    build_token(lexer, token, TOKEN_KIND_NEWLINE);
  } else if (first(lexer) == '\n') {
    bump(lexer);
    if (first(lexer) == '\r') {
      bump(lexer);
    }
    build_token(lexer, token, TOKEN_KIND_NEWLINE);
  } else {
    bump(lexer);
    build_token(lexer, token, TOKEN_KIND_ERROR);
  }
}

static void token_comment(Lexer *lexer, Token *token)
{
  if (first(lexer) == '{') {
    int terminated = 1;
    bump(lexer);
    while (1) {
      if (first(lexer) == '}') {
        bump(lexer);
        break;
      } else if (first(lexer) == EOS) {
        terminated = 0;
        break;
      } else {
        bump(lexer);
      }
    }
    build_token(lexer, token, TOKEN_KIND_BRACES_COMMENT);
    token->terminated = terminated;
  } else if (first(lexer) == '/') {
    bump(lexer);
    if (first(lexer) == '*') {
      int terminated = 1;
      bump(lexer);
      while (1) {
        if (first(lexer) == '*') {
          bump(lexer);
          if (first(lexer) == '/') {
            bump(lexer);
            break;
          }
        } else if (first(lexer) == EOS) {
          terminated = 0;
          break;
        } else {
          bump(lexer);
        }
      }
      build_token(lexer, token, TOKEN_KIND_C_COMMENT);
      token->terminated = terminated;
    } else {
      bump(lexer);
      build_token(lexer, token, TOKEN_KIND_ERROR);
    }
  }
}

static void token_symbol(Lexer *lexer, Token *token)
{
}

void lexer_init(Lexer *lexer, const char *source, long length)
{
  lexer->source = source;
  lexer->length = length;
  lexer->index  = 0;
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
  } else {
    token_symbol(lexer, token);
  }
}
