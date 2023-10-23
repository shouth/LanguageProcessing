#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include "lexical_tree.h"
#include "vector.h"

typedef struct Parser Parser;

struct Parser {
  const char   *_source;
  unsigned long _size;
  unsigned long _offset;
  Lexer         _lexer;
  LexerToken    _token;

  LexicalTree *_tree;
  Vector      *_errors;
};

void parser_init(Parser *parser, const char *source, unsigned long size);
void parser_parse(Parser *parser, LexicalTree *tree, Vector *errors);

#endif
