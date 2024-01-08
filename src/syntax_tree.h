#ifndef SYNTAX_TREE_H
#define SYNTAX_TREE_H

#include "token_tree.h"

typedef struct SyntaxTree SyntaxTree;
typedef int               SyntaxTreeVisitor(const SyntaxTree *tree, void *data, int enter);

SyntaxTree       *syntax_tree_root(const TokenTree *tree);
const TokenNode  *syntax_tree_raw(const SyntaxTree *tree);
SyntaxKind        syntax_tree_kind(const SyntaxTree *tree);
unsigned long     syntax_tree_offset(const SyntaxTree *tree);
unsigned long     syntax_tree_text_length(const SyntaxTree *tree);
unsigned long     syntax_tree_trivia_length(const SyntaxTree *tree);
const SyntaxTree *syntax_tree_parent(const SyntaxTree *tree);
unsigned long     syntax_tree_child_count(const SyntaxTree *tree);
SyntaxTree       *syntax_tree_child(const SyntaxTree *tree, unsigned long index);
void              syntax_tree_visit(const SyntaxTree *tree, SyntaxTreeVisitor *visitor, void *data);
SyntaxTree       *syntax_tree_free(SyntaxTree *tree);

#endif
