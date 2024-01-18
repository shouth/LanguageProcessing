#ifndef LEXICAL_TREE_H
#define LEXICAL_TREE_H

#include "syntax_kind.h"

typedef struct RawSyntaxTrivial RawSyntaxTrivial;
typedef struct RawSyntaxToken   RawSyntaxToken;
typedef struct RawSyntaxTree    RawSyntaxTree;
typedef struct RawSyntaxNode    RawSyntaxNode;

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

#endif
