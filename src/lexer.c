#include <assert.h>
#include <errno.h>
#include <stdlib.h>

#include "lexer.h"
#include "message.h"
#include "source.h"

static lexer_t *ctx;

void lexer_init(lexer_t *lexer, const source_t *src)
{
  lexer->init_len = src->src_len;
  lexer->ptr      = src->src;
  lexer->len      = src->src_len;
  lexer->src      = src;
}

static int is_alphabet(int c)
{
  /* clang-format off */
  switch (c) {
  case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g': case 'h':
  case 'i': case 'j': case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
  case 'q': case 'r': case 's': case 't': case 'u': case 'v': case 'w': case 'x':
  case 'y': case 'z':
  case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
  case 'I': case 'J': case 'K': case 'L': case 'M': case 'N': case 'O': case 'P':
  case 'Q': case 'R': case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
  case 'Y': case 'Z':
    return 1;
  }
  /* clang-format on */
  return 0;
}

static int is_number(int c)
{
  return c >= '0' && c <= '9';
}

static int is_space(int c)
{
  /* clang-format off */
  switch (c) {
  case ' ': case '\t': case '\n': case '\r':
    return 1;
  }
  /* clang-format on */
  return 0;
}

static int is_graphical(int c)
{
  /* clang-format off */
  switch (c) {
  case '!': case '"': case '#': case '$': case '%': case '&': case '\'': case '(':
  case ')': case '*': case '+': case ',': case '-': case '.': case '/': case ':':
  case ';': case '<': case '=': case '>': case '?': case '@': case '[': case '\\':
  case ']': case '^': case '_': case '`': case '{': case '|': case '}': case '~':
    return 1;
  }
  /* clang-format on */
  return is_alphabet(c) || is_number(c) || is_space(c);
}

static int eof(void)
{
  return ctx->len == 0;
}

static void bump(void)
{
  if (!eof()) {
    ++ctx->ptr;
    --ctx->len;
  }
}

static int nth(long index)
{
  if (index >= ctx->len) {
    return EOF;
  } else {
    return ctx->ptr[index];
  }
}

static int first(void)
{
  return nth(0);
}

static int second(void)
{
  return nth(1);
}

static long position(void)
{
  return ctx->init_len - ctx->len;
}

static token_kind_t lex_space(void)
{
  assert(is_space(first()));

  bump();
  while (is_space(first())) {
    bump();
  }
  return TOKEN_WHITESPACE;
}

static token_kind_t lex_braces_comment(void)
{
  assert(first() == '{');

  bump();
  while (1) {
    if (first() == '}') {
      bump();
      ctx->info.braces_comment.terminated = 1;
      return TOKEN_BRACES_COMMENT;
    } else if (eof() || !is_graphical(first())) {
      ctx->info.braces_comment.terminated = 0;
      return TOKEN_BRACES_COMMENT;
    }

    bump();
  }
}

static token_kind_t lex_cstyle_comment(void)
{
  assert(first() == '/' && second() == '*');

  bump();
  bump();
  while (1) {
    if (first() == '*' && second() == '/') {
      bump();
      bump();
      ctx->info.cstyle_comment.terminated = 1;
      return TOKEN_CSTYLE_COMMENT;
    } else if (eof() || !is_graphical(first())) {
      ctx->info.cstyle_comment.terminated = 0;
      return TOKEN_CSTYLE_COMMENT;
    }

    bump();
  }
}

static token_kind_t lex_string(void)
{
  assert(first() == '\'');

  ctx->info.string.len     = 0;
  ctx->info.string.str_len = 0;
  bump();
  while (1) {
    if (first() == '\'') {
      bump();

      if (first() != '\'') {
        ctx->info.string.terminated = 1;
        return TOKEN_STRING;
      } else {
        ctx->info.string.len++;
      }
    } else if (eof() || !is_graphical(first()) || first() == '\r' || first() == '\n') {
      ctx->info.string.terminated = 0;
      return TOKEN_STRING;
    }

    ctx->info.string.len++;
    ctx->info.string.str_len++;
    bump();
  }
}

static token_kind_t lex_name_or_keyword(void)
{
  assert(is_alphabet(first()));

  bump();
  while (is_alphabet(first()) || is_number(first())) {
    bump();
  }
  return TOKEN_NAME;
}

static token_kind_t lex_number(void)
{
  assert(is_number(first()));

  bump();
  while (is_number(first())) {
    bump();
  }
  return TOKEN_NUMBER;
}

static token_kind_t lex_symbol(void)
{

  switch (first()) {
  case '+':
    bump();
    return TOKEN_PLUS;
  case '-':
    bump();
    return TOKEN_MINUS;
  case '*':
    bump();
    return TOKEN_STAR;
  case '=':
    bump();
    return TOKEN_EQUAL;
  case '(':
    bump();
    return TOKEN_LPAREN;
  case ')':
    bump();
    return TOKEN_RPAREN;
  case '[':
    bump();
    return TOKEN_LSQPAREN;
  case ']':
    bump();
    return TOKEN_RSQPAREN;
  case '.':
    bump();
    return TOKEN_DOT;
  case ',':
    bump();
    return TOKEN_COMMA;
  case ';':
    bump();
    return TOKEN_SEMI;

  case '<':
    bump();
    switch (first()) {
    case '>':
      bump();
      return TOKEN_NOTEQ;
    case '=':
      bump();
      return TOKEN_LEEQ;
    default:
      return TOKEN_LE;
    }

  case '>':
    bump();
    switch (first()) {
    case '=':
      bump();
      return TOKEN_GREQ;
    default:
      return TOKEN_GR;
    }

  case ':':
    bump();
    switch (first()) {
    case '=':
      bump();
      return TOKEN_ASSIGN;
    default:
      return TOKEN_COLON;
    }

  default:
    bump();
    return TOKEN_UNKNOWN;
  }
}

static token_kind_t lex_delimited(void)
{
  if (eof()) {
    return TOKEN_EOF;
  } else if (is_space(first())) {
    return lex_space();
  } else if (first() == '{') {
    return lex_braces_comment();
  } else if (first() == '/' && second() == '*') {
    return lex_cstyle_comment();
  } else if (first() == '\'') {
    return lex_string();
  } else if (is_alphabet(first())) {
    return lex_name_or_keyword();
  } else if (is_number(first())) {
    return lex_number();
  } else {
    return lex_symbol();
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

void lex_token(lexer_t *lexer, token_t *ret)
{
  assert(lexer && ret);

  ctx = lexer;
  {
    long pos    = position();
    ret->ptr    = lexer->ptr;
    ret->type   = lex_delimited();
    ret->region = region_from(pos, position() - pos);
  }

  switch (ret->type) {
  case TOKEN_NAME: {
    long i, j;
    for (i = 0; i < keywords_size; i++) {
      const char *ptr = token_to_str(keywords[i]);
      for (j = 0; j < ret->region.len; j++) {
        if (ret->ptr[j] != ptr[j]) {
          break;
        }
      }
      if (j == ret->region.len && ptr[j] == '\0') {
        ret->type = keywords[i];
        break;
      }
    }
    return;
  }

  case TOKEN_NUMBER: {
    errno                  = 0;
    ret->data.number.value = strtoul(ret->ptr, NULL, 10);
    if (errno == ERANGE || ret->data.number.value > 32767) {
      msg_t *msg = new_msg(lexer->src, ret->region, MSG_ERROR, "number is too large");
      msg_add_inline_entry(msg, ret->region, "number needs to be less than 32768");
      msg_emit(msg);
      ret->type = TOKEN_ERROR;
    }
    return;
  }

  case TOKEN_STRING: {
    if (!ctx->info.string.terminated) {
      if (eof()) {
        msg_emit(new_msg(lexer->src, ret->region, MSG_ERROR, "string is unterminated"));
      } else {
        msg_emit(new_msg(lexer->src, region_from(position(), 1),
          MSG_ERROR, "nongraphical character"));
      }
      ret->type = TOKEN_ERROR;
    } else {
      ret->data.string.str_len = ctx->info.string.str_len;
      ret->data.string.ptr     = ret->ptr + 1;
      ret->data.string.len     = ctx->info.string.len;
    }
    return;
  }

  case TOKEN_BRACES_COMMENT: {
    if (!ctx->info.braces_comment.terminated) {
      if (eof()) {
        msg_emit(new_msg(lexer->src, region_from(ret->region.pos, 1),
          MSG_ERROR, "comment is unterminated"));
      } else {
        msg_emit(new_msg(lexer->src, region_from(position(), 1),
          MSG_ERROR, "nongraphical character"));
      }
      ret->type = TOKEN_ERROR;
    }
    return;
  }

  case TOKEN_CSTYLE_COMMENT: {
    if (!ctx->info.cstyle_comment.terminated) {
      if (eof()) {
        msg_emit(new_msg(lexer->src, region_from(ret->region.pos, 2),
          MSG_ERROR, "comment is unterminated"));
      } else {
        msg_emit(new_msg(lexer->src, region_from(position(), 1),
          MSG_ERROR, "nongraphical character"));
      }
      ret->type = TOKEN_ERROR;
    }
    return;
  }

  case TOKEN_UNKNOWN: {
    if (is_graphical(ret->ptr[0])) {
      msg_t *msg = new_msg(lexer->src, ret->region,
        MSG_ERROR, "stray `%c` in program", ret->ptr[0]);
      msg_emit(msg);
    } else {
      msg_t *msg = new_msg(lexer->src, ret->region,
        MSG_ERROR, "stray \\%03o in program", (unsigned char) ret->ptr[0]);
      msg_emit(msg);
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
