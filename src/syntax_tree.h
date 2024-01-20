#ifndef SYNTAX_TREE_H
#define SYNTAX_TREE_H

#include "syntax_kind.h"
typedef struct RawSyntaxTrivial RawSyntaxTrivial;
typedef struct RawSyntaxToken   RawSyntaxToken;
typedef struct RawSyntaxTree    RawSyntaxTree;
typedef struct RawSyntaxNode    RawSyntaxNode;

typedef struct SyntaxTree SyntaxTree;
typedef int               SyntaxTreeVisitor(const SyntaxTree *tree, void *data, int enter);

struct RawSyntaxTrivial {
  SyntaxKind    kind;
  unsigned long text_length;
  char         *text;
};

struct RawSyntaxToken {
  SyntaxKind        kind;
  unsigned long     text_length;
  char             *text;
  unsigned long     trivia_count;
  RawSyntaxTrivial *trivia;
};

struct RawSyntaxTree {
  SyntaxKind      kind;
  unsigned long   text_length;
  unsigned long   children_count;
  RawSyntaxNode **children;
};

struct RawSyntaxNode {
  SyntaxKind    kind;
  unsigned long text_length;
};

unsigned long raw_syntax_node_text_length(const RawSyntaxNode *node);
unsigned long raw_syntax_node_trivia_length(const RawSyntaxNode *node);

RawSyntaxTree *raw_syntax_tree_new(SyntaxKind kind, const RawSyntaxNode **children, unsigned long children_count);
void           raw_syntax_tree_free(RawSyntaxTree *tree);

RawSyntaxToken *raw_syntax_token_new(SyntaxKind kind, char *text, unsigned long text_length, RawSyntaxTrivial *trivia, unsigned long trivia_count);
void            raw_syntax_token_free(RawSyntaxToken *token);

void raw_syntax_node_print(RawSyntaxNode *node);
void raw_syntax_node_free(RawSyntaxNode *node);

SyntaxTree          *syntax_tree_root(RawSyntaxNode *tree);
const RawSyntaxNode *syntax_tree_raw(const SyntaxTree *tree);
SyntaxKind           syntax_tree_kind(const SyntaxTree *tree);
unsigned long        syntax_tree_offset(const SyntaxTree *tree);
unsigned long        syntax_tree_text_length(const SyntaxTree *tree);
unsigned long        syntax_tree_trivia_length(const SyntaxTree *tree);
const SyntaxTree    *syntax_tree_parent(const SyntaxTree *tree);
unsigned long        syntax_tree_child_count(const SyntaxTree *tree);
SyntaxTree          *syntax_tree_child(const SyntaxTree *tree, unsigned long index);
SyntaxTree          *syntax_tree_subtree(const SyntaxTree *tree);
void                 syntax_tree_visit(const SyntaxTree *tree, SyntaxTreeVisitor *visitor, void *data);
void                 syntax_tree_free(SyntaxTree *tree);

#endif
