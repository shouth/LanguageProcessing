#include <errno.h>
#include <string.h>

#include "lexer.h"
#include "message.h"
#include "source.h"

static struct {
  lexer_t *lexer;
  token_t *token;
} ctx;

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

static int peek(void)
{
  if (ctx.lexer->pos >= ctx.lexer->src->src_len) {
    return EOF;
  } else {
    return ctx.lexer->src->src[ctx.lexer->pos];
  }
}

static int bump(void)
{
  int result = peek();
  if (result != EOF) {
    ++ctx.lexer->pos;
  }
  return result;
}

static int check(int c)
{
  return peek() == c;
}

static int check_if(int (*pred)(int))
{
  return pred(peek());
}

static int eat(int c)
{
  int result = check(c);
  if (result) {
    ++ctx.lexer->pos;
  }
  return result;
}

static int eat_if(int (*pred)(int))
{
  int result = check_if(pred);
  if (result) {
    ++ctx.lexer->pos;
  }
  return result;
}

static token_kind_t lex_delimited(void)
{
  if (eat_if(is_space)) {
    while (eat_if(is_space)) { }
    return TOKEN_WHITESPACE;
  } else if (eat_if(is_alphabet)) {
    while (eat_if(is_alphabet) || eat_if(is_number)) { }
    return TOKEN_NAME;
  } else if (eat_if(is_number)) {
    while (eat_if(is_number)) { }
    return TOKEN_NUMBER;
  } else if (eat('{')) {
    token_braces_comment_t *token = (token_braces_comment_t *) ctx.token;
    while (1) {
      if (eat('}')) {
        token->terminated = 1;
        break;
      } else if (check(EOF) || !check_if(is_graphic)) {
        token->terminated = 0;
        break;
      } else {
        bump();
      }
    }
    return TOKEN_BRACES_COMMENT;
  } else if (eat('/')) {
    if (eat('*')) {
      token_cstyle_comment_t *token = (token_cstyle_comment_t *) ctx.token;
      while (1) {
        if (eat('*') && eat('/')) {
          token->terminated = 1;
          break;
        } else if (check(EOF) || !check_if(is_graphic)) {
          token->terminated = 0;
          break;
        } else {
          bump();
        }
      }
      return TOKEN_CSTYLE_COMMENT;
    } else {
      return TOKEN_UNKNOWN;
    }
  } else if (eat('\'')) {
    token_string_t *token = (token_string_t *) ctx.token;
    token->len            = 0;
    token->str_len        = 0;
    token->ptr            = ctx.token->ptr + 1;
    while (1) {
      if (eat('\'')) {
        if (!eat('\'')) {
          token->terminated = 1;
          break;
        }
        ++token->len;
      } else if (check('\r') || check('\n') || check(EOF) || !check_if(is_graphic)) {
        token->terminated = 0;
        break;
      } else {
        bump();
      }
      ++token->len;
      ++token->str_len;
    }
    return TOKEN_STRING;
  } else if (eat('+')) {
    return TOKEN_PLUS;
  } else if (eat('-')) {
    return TOKEN_MINUS;
  } else if (eat('*')) {
    return TOKEN_STAR;
  } else if (eat('=')) {
    return TOKEN_EQUAL;
  } else if (eat('(')) {
    return TOKEN_LPAREN;
  } else if (eat(')')) {
    return TOKEN_RPAREN;
  } else if (eat('[')) {
    return TOKEN_LSQPAREN;
  } else if (eat(']')) {
    return TOKEN_RSQPAREN;
  } else if (eat('.')) {
    return TOKEN_DOT;
  } else if (eat(',')) {
    return TOKEN_COMMA;
  } else if (eat(';')) {
    return TOKEN_SEMI;
  } else if (eat('<')) {
    if (eat('>')) {
      return TOKEN_NOTEQ;
    } else if (eat('=')) {
      return TOKEN_LEEQ;
    } else {
      return TOKEN_LE;
    }
  } else if (eat('>')) {
    if (eat('=')) {
      return TOKEN_GREQ;
    } else {
      return TOKEN_GR;
    }
  } else if (eat(':')) {
    if (eat('=')) {
      return TOKEN_ASSIGN;
    } else {
      return TOKEN_COLON;
    }
  } else if (check(EOF)) {
    return TOKEN_EOF;
  } else {
    return TOKEN_UNKNOWN;
  }
}

static token_kind_t keywords[] = {
  TOKEN_PROGRAM,
  TOKEN_VAR,
  TOKEN_ARRAY,
  TOKEN_OF,
  TOKEN_BEGIN,
  TOKEN_END,
  TOKEN_IF,
  TOKEN_THEN,
  TOKEN_ELSE,
  TOKEN_PROCEDURE,
  TOKEN_RETURN,
  TOKEN_CALL,
  TOKEN_WHILE,
  TOKEN_DO,
  TOKEN_NOT,
  TOKEN_OR,
  TOKEN_DIV,
  TOKEN_AND,
  TOKEN_CHAR,
  TOKEN_INTEGER,
  TOKEN_BOOLEAN,
  TOKEN_READ,
  TOKEN_WRITE,
  TOKEN_READLN,
  TOKEN_WRITELN,
  TOKEN_TRUE,
  TOKEN_FALSE,
  TOKEN_BREAK,
};

static const long keywords_size = sizeof(keywords) / sizeof(*keywords);

void lex_token(lexer_t *lexer, token_t *token)
{
  ctx.lexer = lexer;
  ctx.token = token;

  {
    long pos      = ctx.lexer->pos;
    token->ptr    = lexer->src->src + pos;
    token->type   = lex_delimited();
    token->region = region_from(pos, ctx.lexer->pos - pos);
  }

  switch (token->type) {
  case TOKEN_NAME: {
    long i;
    for (i = 0; i < keywords_size; ++i) {
      const char *ptr = token_to_str(keywords[i]);
      long        len = (long) token->region.len;
      if (!strncmp(ptr, token->ptr, len) && !ptr[len]) {
        token->type = keywords[i];
        break;
      }
    }
    return;
  }
  case TOKEN_NUMBER: {
    token_number_t *token = (token_number_t *) ctx.token;
    errno                 = 0;
    token->value          = strtoul(ctx.token->ptr, NULL, 10);
    if (errno == ERANGE || token->value > 32767) {
      msg_t *msg = new_msg(lexer->src, ctx.token->region, MSG_ERROR, "number is too large");
      msg_add_inline_entry(msg, ctx.token->region, "number needs to be less than 32768");
      msg_emit(msg);
      ctx.token->type = TOKEN_ERROR;
    }
    return;
  }
  case TOKEN_STRING: {
    token_string_t *token = (token_string_t *) ctx.token;
    if (!token->terminated) {
      if (check(EOF)) {
        msg_emit(new_msg(lexer->src, ctx.token->region, MSG_ERROR, "string is unterminated"));
      } else {
        msg_emit(new_msg(lexer->src, region_from(ctx.lexer->pos, 1),
          MSG_ERROR, "nongraphical character"));
      }
      ctx.token->type = TOKEN_ERROR;
    }
    return;
  }
  case TOKEN_BRACES_COMMENT: {
    token_braces_comment_t *token = (token_braces_comment_t *) ctx.token;
    if (!token->terminated) {
      if (check(EOF)) {
        msg_emit(new_msg(lexer->src, region_from(ctx.token->region.pos, 1),
          MSG_ERROR, "comment is unterminated"));
      } else {
        msg_emit(new_msg(lexer->src, region_from(ctx.lexer->pos, 1),
          MSG_ERROR, "nongraphical character"));
      }
      ctx.token->type = TOKEN_ERROR;
    }
    return;
  }
  case TOKEN_CSTYLE_COMMENT: {
    token_cstyle_comment_t *token = (token_cstyle_comment_t *) ctx.token;
    if (!token->terminated) {
      if (check(EOF)) {
        msg_emit(new_msg(lexer->src, region_from(ctx.token->region.pos, 2),
          MSG_ERROR, "comment is unterminated"));
      } else {
        msg_emit(new_msg(lexer->src, region_from(ctx.lexer->pos, 1),
          MSG_ERROR, "nongraphical character"));
      }
      ctx.token->type = TOKEN_ERROR;
    }
    return;
  }
  case TOKEN_UNKNOWN: {
    if (is_graphic(token->ptr[0])) {
      msg_emit(new_msg(lexer->src, token->region,
        MSG_ERROR, "stray `%c` in program", token->ptr[0]));
    } else {
      msg_emit(new_msg(lexer->src, token->region,
        MSG_ERROR, "stray \\%03o in program", (unsigned char) token->ptr[0]));
    }
    return;
  }
  default:
    /* do nothing */
    return;
  }
}

const char *token_to_str(token_kind_t type)
{
  /* clang-format off */
  switch (type) {
  case TOKEN_NAME:      return "NAME";
  case TOKEN_PROGRAM:   return "program";
  case TOKEN_VAR:       return "var";
  case TOKEN_ARRAY:     return "array";
  case TOKEN_OF:        return "of";
  case TOKEN_BEGIN:     return "begin";
  case TOKEN_END:       return "end";
  case TOKEN_IF:        return "if";
  case TOKEN_THEN:      return "then";
  case TOKEN_ELSE:      return "else";
  case TOKEN_PROCEDURE: return "procedure";
  case TOKEN_RETURN:    return "return";
  case TOKEN_CALL:      return "call";
  case TOKEN_WHILE:     return "while";
  case TOKEN_DO:        return "do";
  case TOKEN_NOT:       return "not";
  case TOKEN_OR:        return "or";
  case TOKEN_DIV:       return "div";
  case TOKEN_AND:       return "and";
  case TOKEN_CHAR:      return "char";
  case TOKEN_INTEGER:   return "integer";
  case TOKEN_BOOLEAN:   return "boolean";
  case TOKEN_READLN:    return "readln";
  case TOKEN_WRITELN:   return "writeln";
  case TOKEN_TRUE:      return "true";
  case TOKEN_FALSE:     return "false";
  case TOKEN_NUMBER:    return "NUMBER";
  case TOKEN_STRING:    return "STRING";
  case TOKEN_PLUS:      return "+";
  case TOKEN_MINUS:     return "-";
  case TOKEN_STAR:      return "*";
  case TOKEN_EQUAL:     return "=";
  case TOKEN_NOTEQ:     return "<>";
  case TOKEN_LE:        return "<";
  case TOKEN_LEEQ:      return "<=";
  case TOKEN_GR:        return ">";
  case TOKEN_GREQ:      return ">=";
  case TOKEN_LPAREN:    return "(";
  case TOKEN_RPAREN:    return ")";
  case TOKEN_LSQPAREN:  return "[";
  case TOKEN_RSQPAREN:  return "]";
  case TOKEN_ASSIGN:    return ":=";
  case TOKEN_DOT:       return ".";
  case TOKEN_COMMA:     return ",";
  case TOKEN_COLON:     return ":";
  case TOKEN_SEMI:      return ";";
  case TOKEN_READ:      return "read";
  case TOKEN_WRITE:     return "write";
  case TOKEN_BREAK:     return "break";
  case TOKEN_EOF:       return "EOF";

  case TOKEN_UNKNOWN:   return "UNKNOWN";
  case TOKEN_ERROR:     return "ERROR";

  case TOKEN_BRACES_COMMENT:
  case TOKEN_CSTYLE_COMMENT:
  default:              return "";
  }
  /* clang-format on */
}
