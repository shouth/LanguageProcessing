#ifndef RAW_SYNTAX_TREE_H
#define RAW_SYNTAX_TREE_H

#include <stdio.h>

/* raw syntax tree */

typedef struct RawSyntaxRoot   RawSyntaxRoot;
typedef union RawSyntaxSlot    RawSyntaxSlot;
typedef union RawSyntaxNode    RawSyntaxNode;
typedef struct RawSyntaxTree   RawSyntaxTree;
typedef struct RawSyntaxToken  RawSyntaxToken;
typedef struct RawSyntaxTrivia RawSyntaxTrivia;
typedef unsigned int           RawSyntaxKind;

struct RawSyntaxRoot {
  unsigned long   text_length;
  unsigned long   count;
  RawSyntaxSlot **slots;
};

struct RawSyntaxTree {
  unsigned long   text_length;
  RawSyntaxKind   kind;
  unsigned long   count;
  RawSyntaxSlot **slots;
};

struct RawSyntaxToken {
  unsigned long text_length;
  RawSyntaxKind kind;
};

union RawSyntaxNode {
  struct {
    unsigned long text_length;
    RawSyntaxKind kind;
  } header;
  RawSyntaxTree  tree;
  RawSyntaxToken token;
};

struct RawSyntaxTrivia {
  unsigned long   text_length;
  unsigned long   count;
  RawSyntaxToken *tokens;
};

union RawSyntaxSlot {
  struct {
    unsigned long text_length;
  } header;
  RawSyntaxNode   node;
  RawSyntaxTrivia trivia;
};

void raw_syntax_print(const RawSyntaxRoot *root, FILE *file);
void raw_syntax_free(RawSyntaxRoot *root);

/* raw syntax tree builder */

typedef struct RawSyntaxBuilder RawSyntaxBuilder;
typedef unsigned long           RawSyntaxCheckpoint;

RawSyntaxBuilder   *raw_syntax_builder_new(void);
void                raw_syntax_builder_free(RawSyntaxBuilder *builder);
void                raw_syntax_builder_null(RawSyntaxBuilder *builder);
void                raw_syntax_builder_trivia(RawSyntaxBuilder *builder, RawSyntaxKind kind, unsigned long text_length);
void                raw_syntax_builder_token(RawSyntaxBuilder *builder, RawSyntaxKind kind, unsigned long text_length);
RawSyntaxCheckpoint raw_syntax_builder_open(RawSyntaxBuilder *builder);
void                raw_syntax_builder_close(RawSyntaxBuilder *builder, RawSyntaxKind kind, RawSyntaxCheckpoint checkpoint);
RawSyntaxRoot      *raw_syntax_builder_finish(RawSyntaxBuilder *builder);

#endif /* NEW_SYNTAX_TREE_H */
