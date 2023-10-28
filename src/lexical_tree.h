#ifndef LEXICAL_TREE_H
#define LEXICAL_TREE_H

#include "syntax_kind.h"
#include "vector.h"

typedef struct LexicalTree LexicalTree;

struct LexicalTree {
  SyntaxKind _kind;
  Vector     _children;
};

#endif
