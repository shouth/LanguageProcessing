#ifndef LEXER_H
#define LEXER_H

typedef enum {
  TOKEN_KIND_IDENTIFIER,
  TOKEN_KIND_INTEGER,
  TOKEN_KIND_STRING,

  TOKEN_KIND_PLUS,
  TOKEN_KIND_MINUS,
  TOKEN_KIND_STAR,
  TOKEN_KIND_EQUAL,
  TOKEN_KIND_NOT_EQUAL,
  TOKEN_KIND_LESS_THAN,
  TOKEN_KIND_LESS_THAN_EQUAL,
  TOKEN_KIND_GREATER_THAN,
  TOKEN_KIND_GREATER_THAN_EQUAL,
  TOKEN_KIND_LEFT_PARENTHESIS,
  TOKEN_KIND_RIGHT_PARENTHESIS,
  TOKEN_KIND_LEFT_BRACKET,
  TOKEN_KIND_RIGHT_BRACKET,
  TOKEN_KIND_ASSIGN,
  TOKEN_KIND_DOT,
  TOKEN_KIND_COMMA,
  TOKEN_KIND_COLON,
  TOKEN_KIND_SEMICOLON,

  TOKEN_KIND_SPACE,
  TOKEN_KIND_NEWLINE,
  TOKEN_KIND_BRACES_COMMENT,
  TOKEN_KIND_C_COMMENT,

  TOKEN_KIND_EOF,
  TOKEN_KIND_ERROR
} TokenKind;

typedef struct Token Token;
typedef struct Lexer Lexer;

struct Token {
  TokenKind     kind;
  unsigned long size;
  int           terminated;
};

struct Lexer {
  const char *_source;
  long        _size;
  long        _index;
};

int token_trivial(Token *token);

void lexer_init(Lexer *lexer, const char *source, long size);
void lexer_next_token(Lexer *lexer, Token *token);
int  lexer_eof(Lexer *lexer);

#endif
