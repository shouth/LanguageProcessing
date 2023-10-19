#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include "list.h"

typedef struct ParserCursor ParserCursor;
typedef struct Parser       Parser;

struct Parser {
  const char   *_source;
  unsigned long _size;
  unsigned long _offset;
  Lexer         _lexer;
  LexerToken    _token;
  List          _errors;
};

void parser_init(Parser *parser, const char *source, unsigned long size);
void parser_token(Parser *parser, LexerToken *token);
void parser_bump(Parser *parser);
int  parser_check(Parser *parser, LexerTokenKind kind);
int  parser_check_keyword(Parser *parser, const char *keyword);
int  parser_eat(Parser *parser, LexerTokenKind kind);
int  parser_eat_keyword(Parser *parser, const char *keyword);
int  parser_expect(Parser *parser, LexerTokenKind kind);
int  parser_expect_keyword(Parser *parser, const char *keyword);

#endif
