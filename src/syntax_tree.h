/* SPDX-License-Identifier: Apache-2.0 */

#ifndef MPPL_TREE_H
#define MPPL_TREE_H

#include <stdio.h>

#include "utility.h"

/* raw syntax tree */

typedef enum {
  RAW_SYNTAX_EMPTY,
  RAW_SYNTAX_TOKEN,
  RAW_SYNTAX_TREE,
  RAW_SYNTAX_ROOT
} RawSyntaxNodeKind;

typedef struct RawSyntaxSpan        RawSyntaxSpan;
typedef struct RawSyntaxNode        RawSyntaxNode;
typedef struct RawSyntaxTree        RawSyntaxTree;
typedef struct RawSyntaxToken       RawSyntaxToken;
typedef struct RawSyntaxTrivia      RawSyntaxTrivia;
typedef struct RawSyntaxTriviaPiece RawSyntaxTriviaPiece;
typedef unsigned int                RawSyntaxKind;

typedef Slice(RawSyntaxSpan *) RawSyntaxChildren;
typedef Slice(RawSyntaxTriviaPiece) RawSyntaxTriviaPieceSlice;

struct RawSyntaxSpan {
  unsigned long text_length;
};

struct RawSyntaxNode {
  RawSyntaxSpan     span;
  RawSyntaxKind     kind;
  RawSyntaxNodeKind node_kind;
};

struct RawSyntaxTree {
  RawSyntaxNode     node;
  RawSyntaxChildren children;
};

struct RawSyntaxToken {
  RawSyntaxNode node;
  char         *text;
};

struct RawSyntaxTrivia {
  RawSyntaxSpan             span;
  char                     *text;
  RawSyntaxTriviaPieceSlice pieces;
};

struct RawSyntaxTriviaPiece {
  RawSyntaxSpan span;
  RawSyntaxKind kind;
};

typedef void RawSyntaxKindPrinter(RawSyntaxKind kind, FILE *file);

/* syntax tree */

typedef struct SyntaxSpan   SyntaxSpan;
typedef struct SyntaxNode   SyntaxNode;
typedef struct SyntaxTree   SyntaxTree;
typedef struct SyntaxToken  SyntaxToken;
typedef struct SyntaxTrivia SyntaxTrivia;

struct SyntaxSpan {
  unsigned long offset;
  unsigned long ref;
};

struct SyntaxNode {
  SyntaxSpan        span;
  const SyntaxNode *parent;
};

struct SyntaxToken {
  SyntaxNode            node;
  const RawSyntaxToken *raw;
};

struct SyntaxTree {
  SyntaxNode           node;
  const RawSyntaxTree *raw;
};

struct SyntaxTrivia {
  SyntaxSpan             span;
  const RawSyntaxTrivia *raw;
};

void syntax_tree_free(SyntaxTree *self);
void syntax_token_free(SyntaxToken *self);

void syntax_tree_print(const SyntaxTree *syntax, FILE *file, RawSyntaxKindPrinter *kind_printer);
void syntax_token_print(const SyntaxToken *syntax, FILE *file, RawSyntaxKindPrinter *kind_printer);

SyntaxTree  *syntax_tree_child_tree(const SyntaxTree *self, unsigned long index);
SyntaxToken *syntax_tree_child_token(const SyntaxTree *self, unsigned long index);

SyntaxTrivia *syntax_node_leading_trivia(const SyntaxNode *self);
SyntaxTrivia *syntax_node_trailing_trivia(const SyntaxNode *self);

/* syntax tree builder */

typedef struct SyntaxBuilder SyntaxBuilder;
typedef unsigned long        SyntaxCheckpoint;

SyntaxBuilder   *syntax_builder_new(void);
void             syntax_builder_free(SyntaxBuilder *self);
void             syntax_builder_empty(SyntaxBuilder *self);
void             syntax_builder_trivia(SyntaxBuilder *self, const char *text, RawSyntaxTriviaPiece *pieces, unsigned long piece_count);
void             syntax_builder_token(SyntaxBuilder *self, RawSyntaxKind kind, const char *text, unsigned long text_length);
SyntaxCheckpoint syntax_builder_open(SyntaxBuilder *self);
void             syntax_builder_close(SyntaxBuilder *self, RawSyntaxKind kind, SyntaxCheckpoint checkpoint);
SyntaxTree      *syntax_builder_finish(SyntaxBuilder *self);

#endif /* MPPL_TREE_H */
