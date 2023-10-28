#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include "lexical_tree.h"
#include "symbol.h"
#include "token_cursor.h"
#include "vector.h"

typedef struct Parser Parser;

struct Parser {
  const char   *_source;
  unsigned long _size;
  unsigned long _offset;
  TokenCursor   _cursor;

  Token  _token;
  Symbol _symbol;

  LexicalTree *_tree;
  Vector      *_errors;
};

void parser_init(Parser *parser, const char *source, unsigned long size);
void parser_parse(Parser *parser, LexicalTree *tree, Vector *errors);

#endif
