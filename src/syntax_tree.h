/* SPDX-License-Identifier: Apache-2.0 */

#ifndef SYNTAX_TREE_H
#define SYNTAX_TREE_H

#include <stdio.h>

#include "util.h"

#define SYNTAX_TOKEN 0u
#define SYNTAX_TREE  1u
#define SYNTAX_ROOT  2u

/* raw syntax tree */

typedef struct RawSyntaxSpan        RawSyntaxSpan;
typedef struct RawSyntaxSlot        RawSyntaxSlot;
typedef struct RawSyntaxNode        RawSyntaxNode;
typedef struct RawSyntaxTree        RawSyntaxTree;
typedef struct RawSyntaxToken       RawSyntaxToken;
typedef struct RawSyntaxTrivia      RawSyntaxTrivia;
typedef struct RawSyntaxTriviaPiece RawSyntaxTriviaPiece;
typedef unsigned int                RawSyntaxKind;

struct RawSyntaxSpan {
  unsigned long text_length;
};

struct RawSyntaxSlot {
  RawSyntaxNode *node;
  unsigned long  offset;
};

struct RawSyntaxNode {
  RawSyntaxSpan span;
  RawSyntaxKind kind;
};

struct RawSyntaxTree {
  RawSyntaxNode node;
  Slice(RawSyntaxSlot) children;
};

struct RawSyntaxToken {
  RawSyntaxNode node;
  char         *text;
};

struct RawSyntaxTrivia {
  RawSyntaxSpan span;
  char         *text;
  Slice(RawSyntaxTriviaPiece) pieces;
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
  const SyntaxTree *parent;
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
  const SyntaxNode      *adjacent;
  const RawSyntaxTrivia *raw;
};

void syntax_tree_free(SyntaxTree *self);
void syntax_token_free(SyntaxToken *self);

void syntax_tree_print(const SyntaxTree *syntax, FILE *file, RawSyntaxKindPrinter *kind_printer);
void syntax_token_print(const SyntaxToken *syntax, FILE *file, RawSyntaxKindPrinter *kind_printer);

SyntaxTree   *syntax_tree_child_tree(const SyntaxTree *self, unsigned long index);
SyntaxToken  *syntax_tree_child_token(const SyntaxTree *self, unsigned long index);
SyntaxTrivia *syntax_tree_leading_trivia(const SyntaxTree *self);
SyntaxTrivia *syntax_tree_trailing_trivia(const SyntaxTree *self);

SyntaxTrivia *syntax_token_leading_trivia(const SyntaxToken *self);
SyntaxTrivia *syntax_token_trailing_trivia(const SyntaxToken *self);

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

#endif /* SYNTAX_TREE_H */
