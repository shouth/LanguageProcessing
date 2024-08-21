#ifndef MPPL_TREE_H
#define MPPL_TREE_H

#include <stdio.h>

/* raw syntax tree */

typedef struct RawSyntaxSpan   RawSyntaxSpan;
typedef struct RawSyntaxRoot   RawSyntaxRoot;
typedef struct RawSyntaxNode   RawSyntaxNode;
typedef struct RawSyntaxTree   RawSyntaxTree;
typedef struct RawSyntaxToken  RawSyntaxToken;
typedef struct RawSyntaxTrivia RawSyntaxTrivia;
typedef unsigned int           RawSyntaxKind;

struct RawSyntaxSpan {
  unsigned long text_length;
};

struct RawSyntaxRoot {
  RawSyntaxSpan   span;
  unsigned long   children_count;
  RawSyntaxSpan **children;
};

struct RawSyntaxNode {
  RawSyntaxSpan span;
  RawSyntaxKind kind;
};

struct RawSyntaxTree {
  RawSyntaxNode   node;
  unsigned long   children_count;
  RawSyntaxSpan **children;
};

struct RawSyntaxToken {
  RawSyntaxNode node;
  char         *text;
};

struct RawSyntaxTrivia {
  RawSyntaxSpan    span;
  unsigned long    children_count;
  RawSyntaxToken **children;
};

/* syntax tree */

typedef struct SyntaxRoot      SyntaxRoot;
typedef struct SyntaxInterface SyntaxInterface;

struct SyntaxInterface {
  void (*print_kind)(FILE *file, RawSyntaxKind kind);
  int (*is_token)(RawSyntaxKind kind);
};

struct SyntaxRoot {
  RawSyntaxRoot  *raw;
  SyntaxInterface interface;
};

void syntax_print(SyntaxRoot *syntax, const char *source, FILE *file);
void syntax_free(SyntaxRoot *syntax);

/* raw syntax tree builder */

typedef struct SyntaxBuilder SyntaxBuilder;
typedef unsigned long        SyntaxCheckpoint;

SyntaxBuilder   *syntax_builder_new(void);
void             syntax_builder_free(SyntaxBuilder *self);
void             syntax_builder_empty(SyntaxBuilder *self);
void             syntax_builder_trivia(SyntaxBuilder *self, RawSyntaxKind kind, const char *text, unsigned long text_length);
void             syntax_builder_token(SyntaxBuilder *self, RawSyntaxKind kind, const char *text, unsigned long text_length);
SyntaxCheckpoint syntax_builder_open(SyntaxBuilder *self);
void             syntax_builder_close(SyntaxBuilder *self, RawSyntaxKind kind, SyntaxCheckpoint checkpoint);
SyntaxRoot      *syntax_builder_finish(SyntaxBuilder *self, SyntaxInterface *interface);

#endif /* MPPL_TREE_H */
