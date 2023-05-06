#include <errno.h>
#include <string.h>

#include "lexer.h"
#include "message.h"
#include "source.h"
#include "token.h"

void lexer_init(lexer_t *lexer, const source_t *src)
{
  lexer->pos = 0;
  lexer->src = src;
}

static int is_alphabet(int c)
{
  return !!strchr("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijlmnopqrstuvwxyz", c);
}

static int is_number(int c)
{
  return c >= '0' && c <= '9';
}

static int is_space(int c)
{
  return !!strchr(" \t\n\r", c);
}

static int is_graphic(int c)
{
  return !!strchr("!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~", c) || is_alphabet(c) || is_number(c) || is_space(c);
}

static int peek(lexer_t *lexer)
{
  if (lexer->pos >= lexer->src->src_len) {
    return EOF;
  } else {
    return lexer->src->src[lexer->pos];
  }
}

static int bump(lexer_t *lexer)
{
  int result = peek(lexer);
  if (result != EOF) {
    ++lexer->pos;
  }
  return result;
}

static int check(lexer_t *lexer, int c)
{
  return peek(lexer) == c;
}

static int check_if(lexer_t *lexer, int (*pred)(int))
{
  return pred(peek(lexer));
}

static int eat(lexer_t *lexer, int c)
{
  int result = check(lexer, c);
  if (result) {
    ++lexer->pos;
  }
  return result;
}

static int eat_if(lexer_t *lexer, int (*pred)(int))
{
  int result = check_if(lexer, pred);
  if (result) {
    ++lexer->pos;
  }
  return result;
}

static token_kind_t lex_delimited(lexer_t *lexer, token_t *token)
{
  if (eat_if(lexer, is_space)) {
    while (eat_if(lexer, is_space)) { }
    return TOKEN_WHITESPACE;
  } else if (eat_if(lexer, is_alphabet)) {
    while (eat_if(lexer, is_alphabet) || eat_if(lexer, is_number)) { }
    return TOKEN_IDENT;
  } else if (eat_if(lexer, is_number)) {
    while (eat_if(lexer, is_number)) { }
    return TOKEN_NUMBER;
  } else if (eat(lexer, '{')) {
    token_braces_comment_t *comment = (token_braces_comment_t *) token;
    while (1) {
      if (eat(lexer, '}')) {
        comment->terminated = 1;
        break;
      } else if (check(lexer, EOF) || !check_if(lexer, is_graphic)) {
        comment->terminated = 0;
        break;
      } else {
        bump(lexer);
      }
    }
    return TOKEN_BRACES_COMMENT;
  } else if (eat(lexer, '/')) {
    if (eat(lexer, '*')) {
      token_cstyle_comment_t *comment = (token_cstyle_comment_t *) token;
      while (1) {
        if (eat(lexer, '*') && eat(lexer, '/')) {
          comment->terminated = 1;
          break;
        } else if (check(lexer, EOF) || !check_if(lexer, is_graphic)) {
          comment->terminated = 0;
          break;
        } else {
          bump(lexer);
        }
      }
      return TOKEN_CSTYLE_COMMENT;
    } else {
      return TOKEN_UNKNOWN;
    }
  } else if (eat(lexer, '\'')) {
    token_string_t *comment = (token_string_t *) token;
    comment->len            = 0;
    comment->str_len        = 0;
    comment->ptr            = token->ptr + 1;
    while (1) {
      if (eat(lexer, '\'')) {
        if (!eat(lexer, '\'')) {
          comment->terminated = 1;
          break;
        }
        ++comment->len;
      } else if (check(lexer, '\r') || check(lexer, '\n') || check(lexer, EOF) || !check_if(lexer, is_graphic)) {
        comment->terminated = 0;
        break;
      } else {
        bump(lexer);
      }
      ++comment->len;
      ++comment->str_len;
    }
    return TOKEN_STRING;
  } else if (eat(lexer, '+')) {
    return TOKEN_PLUS;
  } else if (eat(lexer, '-')) {
    return TOKEN_MINUS;
  } else if (eat(lexer, '*')) {
    return TOKEN_STAR;
  } else if (eat(lexer, '=')) {
    return TOKEN_EQUAL;
  } else if (eat(lexer, '(')) {
    return TOKEN_LPAREN;
  } else if (eat(lexer, ')')) {
    return TOKEN_RPAREN;
  } else if (eat(lexer, '[')) {
    return TOKEN_LSQPAREN;
  } else if (eat(lexer, ']')) {
    return TOKEN_RSQPAREN;
  } else if (eat(lexer, '.')) {
    return TOKEN_DOT;
  } else if (eat(lexer, ',')) {
    return TOKEN_COMMA;
  } else if (eat(lexer, ';')) {
    return TOKEN_SEMI;
  } else if (eat(lexer, '<')) {
    if (eat(lexer, '>')) {
      return TOKEN_NOTEQ;
    } else if (eat(lexer, '=')) {
      return TOKEN_LEEQ;
    } else {
      return TOKEN_LE;
    }
  } else if (eat(lexer, '>')) {
    if (eat(lexer, '=')) {
      return TOKEN_GREQ;
    } else {
      return TOKEN_GR;
    }
  } else if (eat(lexer, ':')) {
    if (eat(lexer, '=')) {
      return TOKEN_ASSIGN;
    } else {
      return TOKEN_COLON;
    }
  } else if (check(lexer, EOF)) {
    return TOKEN_EOF;
  } else {
    bump(lexer);
    return TOKEN_UNKNOWN;
  }
}

void lex_token(lexer_t *lexer, token_t *token)
{
  long pos      = lexer->pos;
  token->ptr    = lexer->src->src + pos;
  token->type   = lex_delimited(lexer, token);
  token->region = region_from(pos, lexer->pos - pos);

  switch (token->type) {
  case TOKEN_NUMBER: {
    token_number_t *number = (token_number_t *) token;
    errno                  = 0;
    number->value          = strtoul(token->ptr, NULL, 10);
    if (errno == ERANGE || number->value > 32767) {
      msg_t *msg = msg_new(lexer->src, token->region, MSG_ERROR, "number is too large");
      msg_add_inline(msg, token->region, "number needs to be less than 32768");
      msg_emit(msg);
      token->type = TOKEN_ERROR;
    }
    return;
  }
  case TOKEN_STRING: {
    token_string_t *string = (token_string_t *) token;
    if (!string->terminated) {
      if (check(lexer, EOF)) {
        msg_emit(msg_new(lexer->src, token->region, MSG_ERROR, "string is unterminated"));
      } else {
        msg_emit(msg_new(lexer->src, region_from(lexer->pos, 1),
          MSG_ERROR, "nongraphical character"));
      }
      token->type = TOKEN_ERROR;
    }
    return;
  }
  case TOKEN_BRACES_COMMENT: {
    token_braces_comment_t *comment = (token_braces_comment_t *) token;
    if (!comment->terminated) {
      if (check(lexer, EOF)) {
        msg_emit(msg_new(lexer->src, region_from(token->region.pos, 1),
          MSG_ERROR, "comment is unterminated"));
      } else {
        msg_emit(msg_new(lexer->src, region_from(lexer->pos, 1),
          MSG_ERROR, "nongraphical character"));
      }
      token->type = TOKEN_ERROR;
    }
    return;
  }
  case TOKEN_CSTYLE_COMMENT: {
    token_cstyle_comment_t *comment = (token_cstyle_comment_t *) token;
    if (!comment->terminated) {
      if (check(lexer, EOF)) {
        msg_emit(msg_new(lexer->src, region_from(token->region.pos, 2),
          MSG_ERROR, "comment is unterminated"));
      } else {
        msg_emit(msg_new(lexer->src, region_from(lexer->pos, 1),
          MSG_ERROR, "nongraphical character"));
      }
      token->type = TOKEN_ERROR;
    }
    return;
  }
  case TOKEN_UNKNOWN: {
    if (is_graphic(token->ptr[0])) {
      msg_emit(msg_new(lexer->src, token->region,
        MSG_ERROR, "stray `%c` in program", token->ptr[0]));
    } else {
      msg_emit(msg_new(lexer->src, token->region,
        MSG_ERROR, "stray \\%03o in program", (unsigned char) token->ptr[0]));
    }
    return;
  }
  default:
    /* do nothing */
    return;
  }
}
