#ifndef SYNTAX_TREE_H
#define SYNTAX_TREE_H

#include "string.h"
#include "syntax_kind.h"
typedef struct RawSyntaxTrivia RawSyntaxTrivia;
typedef struct RawSyntaxToken  RawSyntaxToken;
typedef struct RawSyntaxTree   RawSyntaxTree;
typedef struct RawSyntaxNode   RawSyntaxNode;

typedef struct SyntaxTree SyntaxTree;
typedef int               SyntaxTreeVisitor(const SyntaxTree *tree, void *data, int enter);

typedef struct SyntaxBuilder SyntaxBuilder;

struct RawSyntaxTrivia {
  SyntaxKind    kind;
  unsigned long text_length;
  char         *text;

  const String *string;
};

struct RawSyntaxToken {
  SyntaxKind    kind;
  unsigned long text_length;
  char         *text;

  const String    *string;
  unsigned long    leading_trivia_count;
  RawSyntaxTrivia *leading_trivia;
  unsigned long    trailing_trivia_count;
  RawSyntaxTrivia *trailing_trivia;
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

void raw_syntax_node_print(const RawSyntaxNode *node);
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

SyntaxBuilder *syntax_builder_new(void);
void           syntax_builder_free(SyntaxBuilder *builder);
unsigned long  syntax_builder_checkpoint(SyntaxBuilder *builder);
void           syntax_builder_start_tree(SyntaxBuilder *builder);
void           syntax_builder_start_tree_at(SyntaxBuilder *builder, unsigned long checkpoint);
void           syntax_builder_end_tree(SyntaxBuilder *builder, SyntaxKind kind);
void           syntax_builder_null(SyntaxBuilder *builder);
void           syntax_builder_trivia(SyntaxBuilder *builder, SyntaxKind kind, const String *text, int leading);
void           syntax_builder_token(SyntaxBuilder *builder, SyntaxKind kind, const String *text);
SyntaxTree    *syntax_builder_build(SyntaxBuilder *builder);

#endif
