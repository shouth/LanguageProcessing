#ifndef PARSER_H
#define PARSER_H

#include "token.h"
#include "token_tree.h"
#include "token_cursor.h"
#include "vector.h"

typedef struct Parser Parser;

struct Parser {
  const char   *_source;
  unsigned long _size;
  unsigned long _offset;

  TokenCursor _cursor;
  Token       _token;

  Vector _parents;
  Vector _children;

  TokenTree *_tree;
  Vector    *_errors;
};

void parser_init(Parser *parser, const char *source, unsigned long size);
void parser_parse(Parser *parser, TokenTree *tree, Vector *errors);

#endif
