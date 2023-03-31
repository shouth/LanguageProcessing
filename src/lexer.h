#ifndef LEXER_H
#define LEXER_H

#include "source.h"

typedef enum {
  TOKEN_NAME,
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
  TOKEN_READLN,
  TOKEN_WRITELN,
  TOKEN_TRUE,
  TOKEN_FALSE,
  TOKEN_NUMBER,
  TOKEN_STRING,
  TOKEN_PLUS,
  TOKEN_MINUS,
  TOKEN_STAR,
  TOKEN_EQUAL,
  TOKEN_NOTEQ,
  TOKEN_LE,
  TOKEN_LEEQ,
  TOKEN_GR,
  TOKEN_GREQ,
  TOKEN_LPAREN,
  TOKEN_RPAREN,
  TOKEN_LSQPAREN,
  TOKEN_RSQPAREN,
  TOKEN_ASSIGN,
  TOKEN_DOT,
  TOKEN_COMMA,
  TOKEN_COLON,
  TOKEN_SEMI,
  TOKEN_READ,
  TOKEN_WRITE,
  TOKEN_BREAK,
  TOKEN_WHITESPACE,
  TOKEN_BRACES_COMMENT,
  TOKEN_CSTYLE_COMMENT,
  TOKEN_EOF,
  TOKEN_UNKNOWN,
  TOKEN_ERROR
} token_kind_t;

typedef struct token__number_s         token_number_t;
typedef struct token__string_s         token_string_t;
typedef struct token__braces_comment_s token_braces_comment_t;
typedef struct token__cstyle_comment_s token_cstyle_comment_t;

struct token__number_s {
  unsigned long value;
};

struct token__string_s {
  const char *ptr;
  long        len;
  long        str_len;
  int         terminated;
};

struct token__braces_comment_s {
  int terminated;
};

struct token__cstyle_comment_s {
  int terminated;
};

typedef struct {
  union {
    token_number_t         number;
    token_string_t         string;
    token_braces_comment_t braces_comment;
    token_cstyle_comment_t cstyle_comment;
  } data;

  const char  *ptr;
  region_t     region;
  token_kind_t type;
} token_t;

typedef struct {
  const source_t *src;
  long            pos;
} lexer_t;

void        lexer_init(lexer_t *lexer, const source_t *src);
void        lex_token(lexer_t *lexer, token_t *ret);
const char *token_to_str(token_kind_t type);

#endif
