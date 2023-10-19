#ifndef LEXER_H
#define LEXER_H

typedef enum {
  LEXER_TOKEN_KIND_IDENTIFIER,
  LEXER_TOKEN_KIND_INTEGER,
  LEXER_TOKEN_KIND_STRING,

  LEXER_TOKEN_KIND_PLUS,
  LEXER_TOKEN_KIND_MINUS,
  LEXER_TOKEN_KIND_STAR,
  LEXER_TOKEN_KIND_EQUAL,
  LEXER_TOKEN_KIND_NOT_EQUAL,
  LEXER_TOKEN_KIND_LESS_THAN,
  LEXER_TOKEN_KIND_LESS_THAN_EQUAL,
  LEXER_TOKEN_KIND_GREATER_THAN,
  LEXER_TOKEN_KIND_GREATER_THAN_EQUAL,
  LEXER_TOKEN_KIND_LEFT_PARENTHESIS,
  LEXER_TOKEN_KIND_RIGHT_PARENTHESIS,
  LEXER_TOKEN_KIND_LEFT_BRACKET,
  LEXER_TOKEN_KIND_RIGHT_BRACKET,
  LEXER_TOKEN_KIND_ASSIGN,
  LEXER_TOKEN_KIND_DOT,
  LEXER_TOKEN_KIND_COMMA,
  LEXER_TOKEN_KIND_COLON,
  LEXER_TOKEN_KIND_SEMICOLON,

  LEXER_TOKEN_KIND_SPACE,
  LEXER_TOKEN_KIND_NEWLINE,
  LEXER_TOKEN_KIND_BRACES_COMMENT,
  LEXER_TOKEN_KIND_C_COMMENT,

  LEXER_TOKEN_KIND_EOF,
  LEXER_TOKEN_KIND_ERROR
} LexerTokenKind;

typedef struct LexerToken LexerToken;
typedef struct Lexer      Lexer;

struct LexerToken {
  LexerTokenKind kind;
  unsigned long  size;
  int            terminated;
};

struct Lexer {
  const char *_source;
  long        _size;
  long        _index;
};

int lexer_token_trivial(LexerToken *token);

void lexer_init(Lexer *lexer, const char *source, long size);
void lexer_next_token(Lexer *lexer, LexerToken *token);
int  lexer_eof(Lexer *lexer);

#endif
