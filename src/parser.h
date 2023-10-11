#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include "lexical_tree.h"

struct Parser {
  Lexer lexer;
};

int parse(const char *source, long length, LexicalTree *tree);

#endif
