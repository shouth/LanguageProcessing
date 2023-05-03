#ifndef TOKEN_H
#define TOKEN_H

#include "source.h"

typedef enum {
  TOKEN_CATEGORY_KEYWORD,
  TOKEN_CATEGORY_TOKEN,
  TOKEN_CATEGORY_ERROR,

  TOKEN_CATEGORY_COUNT
} token_category_t;

#define token_category(kind) (kind & 0xFF00)

typedef enum {
  TOKEN_TYPE_KEYWORD = TOKEN_CATEGORY_KEYWORD << 8,
  TOKEN_PROGRAM      = TOKEN_TYPE_KEYWORD,
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
  TOKEN_READ,
  TOKEN_WRITE,
  TOKEN_BREAK,
  TOKEN_TYPE_KEYWORD_END,

  TOKEN_TYPE_TOKEN = TOKEN_CATEGORY_TOKEN << 8,
  TOKEN_IDENT      = TOKEN_TYPE_TOKEN,
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
  TOKEN_WHITESPACE,
  TOKEN_BRACES_COMMENT,
  TOKEN_CSTYLE_COMMENT,
  TOKEN_EOF,
  TOKEN_TYPE_TOKEN_END,

  TOKEN_TYPE_ERROR = TOKEN_CATEGORY_ERROR << 8,
  TOKEN_UNKNOWN    = TOKEN_TYPE_ERROR,
  TOKEN_ERROR,
  TOKEN_TYPE_ERROR_END
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

const char *token_to_str(token_kind_t type);

#endif
