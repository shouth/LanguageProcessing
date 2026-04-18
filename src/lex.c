/*
 * lex.c -- lexical analyzer
 *
 * SPDX-FileCopyrightText: 2026 Shota Minami
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include "compiler.h"
#include "syn.h"

struct lexer {
  char const *text;
  size_t len;
  size_t cursor;
};

static int bump(struct lexer *l)
{
  int result = l->cursor < l->len;
  if (result) {
    l->cursor++;
  }
  return result;
}

static int check(struct lexer *l, int c)
{
  return l->cursor < l->len && l->text[l->cursor] == c;
}

static int check_if(struct lexer *l, int (*p)(int))
{
  return l->cursor < l->len && p(l->text[l->cursor]);
}

static int eat(struct lexer *l, int c)
{
  return check(l, c) && bump(l);
}

static int eat_if(struct lexer *l, int (*p)(int))
{
  return check_if(l, p) && bump(l);
}

static int eof(struct lexer *l)
{
  return l->cursor >= l->len;
}

static int is_alpha(int c)
{
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

static int is_digit(int c)
{
  return c >= '0' && c <= '9';
}

static int is_space(int c)
{
  return c == ' ' || c == '\t';
}

static int is_graphic(int c)
{
  return 0x20 <= c && c <= 0x7E;
}

static void tokenize(struct lexer *l, struct token *token, enum syn_kind kind)
{
  token->kind = kind;
  token->len = l->cursor;
}

static void tokenize_ident_or_keyword(struct lexer *l, struct token *token)
{
  size_t i;
  while (eat_if(l, is_alpha) || eat_if(l, is_digit));
  for (i = KW_BEGIN; i <= KW_END; i++) {
    char const *lexeme = syn_kind_to_lexeme(i);
    if (strncmp(l->text, lexeme, l->cursor) == 0 && !lexeme[l->cursor]) {
      tokenize(l, token, i);
      return;
    }
  }
  tokenize(l, token, SYN_IDENT);
}

static void tokenize_number(struct lexer *l, struct token *token)
{
  while (eat_if(l, is_digit));
  tokenize(l, token, SYN_NUMBER_LIT);
}

static void tokenize_string(struct lexer *l, struct token *token)
{
  while (1) {
    if (eof(l) || check(l, '\r') || check(l, '\n')) {
      token->nonclosed = 1;
      break;
    } else if (eat(l, '\'') && !eat(l, '\'')) {
      break;
    } else if (!eat_if(l, is_graphic)) {
      token->nongraphic = 1;
      bump(l);
    }
  }
  tokenize(l, token, SYN_STRING_LIT);
}

static void tokenize_space(struct lexer *l, struct token *token)
{
  while (eat_if(l, is_space));
  tokenize(l, token, SYN_WHITESPACE);
}

static void tokenize_bracket_comment(struct lexer *l, struct token *token)
{
  while (1) {
    if (eof(l)) {
      token->nonclosed = 1;
      break;
    } else if (eat(l, '}')) {
      break;
    } else if (!eat_if(l, is_graphic)) {
      token->nongraphic = 1;
      bump(l);
    }
  }
  tokenize(l, token, SYN_BRACKET_COMMENT);
}

static void tokenize_slash_star_comment(struct lexer *l, struct token *token)
{
  while (1) {
    if (eof(l)) {
      token->nonclosed = 1;
      break;
    } else if (eat(l, '*') && eat(l, '/')) {
      break;
    } else if (!eat_if(l, is_graphic)) {
      token->nongraphic = 1;
      bump(l);
    }
  }
  tokenize(l, token, SYN_SLASH_STAR_COMMENT);
}

int lex(char const *text, size_t len, struct token *token)
{
  struct lexer l;
  l.text = text;
  l.len = len;
  l.cursor = 0;

  token->nonclosed = 0;
  token->nongraphic = 0;

  if (len == 0) {
    tokenize(&l, token, SYN_EOF);
  } else if (eat_if(&l, is_alpha)) {
    tokenize_ident_or_keyword(&l, token);
  } else if (eat_if(&l, is_digit)) {
    tokenize_number(&l, token);
  } else if (eat(&l, '\'')) {
    tokenize_string(&l, token);
  } else if (eat_if(&l, is_space)) {
    tokenize_space(&l, token);
  } else if (eat(&l, '{')) {
    tokenize_bracket_comment(&l, token);
  } else if (eat(&l, '/')) {
    if (eat(&l, '*')) {
      tokenize_slash_star_comment(&l, token);
    } else {
      tokenize(&l, token, SYN_ERROR);
    }
  } else if (eat(&l, '\r')) {
    eat(&l, '\n');
    tokenize(&l, token, SYN_NEWLINE);
  } else if (eat(&l, '\n')) {
    eat(&l, '\r');
    tokenize(&l, token, SYN_NEWLINE);
  } else if (eat(&l, '+')) {
    tokenize(&l, token, SYN_PLUS);
  } else if (eat(&l, '-')) {
    tokenize(&l, token, SYN_MINUS);
  } else if (eat(&l, '*')) {
    tokenize(&l, token, SYN_STAR);
  } else if (eat(&l, '=')) {
    tokenize(&l, token, SYN_EQ);
  } else if (eat(&l, '<')) {
    if (eat(&l, '>')) {
      tokenize(&l, token, SYN_NEQ);
    } else if (eat(&l, '=')) {
      tokenize(&l, token, SYN_LTEQ);
    } else {
      tokenize(&l, token, SYN_LT);
    }
  } else if (eat(&l, '>')) {
    if (eat(&l, '=')) {
      tokenize(&l, token, SYN_GTEQ);
    } else {
      tokenize(&l, token, SYN_GT);
    }
  } else if (eat(&l, '(')) {
    tokenize(&l, token, SYN_LPAREN);
  } else if (eat(&l, ')')) {
    tokenize(&l, token, SYN_RPAREN);
  } else if (eat(&l, '[')) {
    tokenize(&l, token, SYN_LBRACE);
  } else if (eat(&l, ']')) {
    tokenize(&l, token, SYN_RBRACE);
  } else if (eat(&l, ':')) {
    if (eat(&l, '=')) {
      tokenize(&l, token, SYN_ASSIGN);
    } else {
      tokenize(&l, token, SYN_COLON);
    }
  } else if (eat(&l, '.')) {
    tokenize(&l, token, SYN_DOT);
  } else if (eat(&l, ',')) {
    tokenize(&l, token, SYN_COMMA);
  } else if (eat(&l, ';')) {
    tokenize(&l, token, SYN_SEMI);
  } else {
    bump(&l);
    tokenize(&l, token, SYN_ERROR);
  }

  return token->kind != SYN_EOF;
}
