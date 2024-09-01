#ifndef MPPL_TREE_H
#define MPPL_TREE_H

#include "utility.h"
#include <stdio.h>

/* raw syntax tree */

typedef enum {
  RAW_SYNTAX_EMPTY,
  RAW_SYNTAX_TOKEN,
  RAW_SYNTAX_TREE
} RawSyntaxNodeKind;

typedef struct RawSyntaxSpan        RawSyntaxSpan;
typedef struct RawSyntaxRoot        RawSyntaxRoot;
typedef struct RawSyntaxNode        RawSyntaxNode;
typedef struct RawSyntaxTree        RawSyntaxTree;
typedef struct RawSyntaxToken       RawSyntaxToken;
typedef struct RawSyntaxTrivia      RawSyntaxTrivia;
typedef struct RawSyntaxTriviaPiece RawSyntaxTriviaPiece;
typedef unsigned int                RawSyntaxKind;

typedef Seq(RawSyntaxSpan *) RawSyntaxChildren;
typedef Seq(RawSyntaxTriviaPiece) RawSyntaxTriviaPieceSeq;

struct RawSyntaxSpan {
  unsigned long text_length;
};

struct RawSyntaxRoot {
  RawSyntaxSpan     span;
  RawSyntaxChildren children;
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
  RawSyntaxSpan           span;
  char                   *text;
  RawSyntaxTriviaPieceSeq pieces;
};

struct RawSyntaxTriviaPiece {
  RawSyntaxSpan span;
  RawSyntaxKind kind;
};

typedef void RawSyntaxKindPrinter(RawSyntaxKind kind, FILE *file);

void                   raw_syntax_root_print(const RawSyntaxRoot *syntax, FILE *file, RawSyntaxKindPrinter *kind_printer);
void                   raw_syntax_root_free(RawSyntaxRoot *syntax);
const RawSyntaxNode   *raw_syntax_root_node(const RawSyntaxRoot *syntax, unsigned long index);
const RawSyntaxTrivia *raw_syntax_root_trivia(const RawSyntaxRoot *syntax, unsigned long index);

const RawSyntaxNode   *raw_syntax_tree_node(const RawSyntaxTree *syntax, unsigned long index);
const RawSyntaxTrivia *raw_syntax_tree_trivia(const RawSyntaxTree *syntax, unsigned long index);

/* raw syntax tree builder */

typedef struct RawSyntaxBuilder RawSyntaxBuilder;
typedef unsigned long           RawSyntaxCheckpoint;

RawSyntaxBuilder   *raw_syntax_builder_new(void);
void                raw_syntax_builder_free(RawSyntaxBuilder *self);
void                raw_syntax_builder_empty(RawSyntaxBuilder *self);
void                raw_syntax_builder_trivia(RawSyntaxBuilder *self, const char *text, RawSyntaxTriviaPiece *pieces, unsigned long piece_count);
void                raw_syntax_builder_token(RawSyntaxBuilder *self, RawSyntaxKind kind, const char *text, unsigned long text_length);
RawSyntaxCheckpoint raw_syntax_builder_open(RawSyntaxBuilder *self);
void                raw_syntax_builder_close(RawSyntaxBuilder *self, RawSyntaxKind kind, RawSyntaxCheckpoint checkpoint);
RawSyntaxRoot      *raw_syntax_builder_finish(RawSyntaxBuilder *self);

#endif /* MPPL_TREE_H */
