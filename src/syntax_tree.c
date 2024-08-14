#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "array.h"
#include "syntax_tree.h"
#include "utility.h"

/* syntax tree */

static void print_trivia(FILE *file, RawSyntaxTrivia *trivia, SyntaxRoot *root, unsigned long offset, int depth);
static void print_node(FILE *file, RawSyntaxNode *node, SyntaxRoot *root, const char *source, unsigned long offset, int depth);
static void print_spans(FILE *file, RawSyntaxSpan **spans, unsigned long span_count, SyntaxRoot *root, const char *source, unsigned long offset, int depth);

static void print_trivia(FILE *file, RawSyntaxTrivia *trivia, SyntaxRoot *root, unsigned long offset, int depth)
{
  unsigned long i;
  for (i = 0; i < trivia->children_count; ++i) {
    printf("%*.s(", (int) depth * 2, "");
    root->interface.print_kind(file, trivia->children[i]->node.kind);
    printf(" @ %ld..%ld)", offset, offset + trivia->children[i]->node.span.text_length);
    offset += trivia->children[i]->node.span.text_length;
  }
}

static void print_node(FILE *file, RawSyntaxNode *node, SyntaxRoot *root, const char *source, unsigned long offset, int depth)
{
  printf("%.*s", (int) depth * 2, "");
  root->interface.print_kind(file, node->kind);
  printf(" @ %ld..%ld", offset, offset + node->span.text_length);

  if (root->interface.is_token(node->kind)) {
    printf(" \"%*.s\"\n", (int) node->span.text_length, source + offset);
  } else {
    RawSyntaxTree *tree = (RawSyntaxTree *) node;
    printf("\n");
    print_spans(file, tree->children, tree->children_count, root, source, offset, depth + 1);
  }
}

static void print_spans(FILE *file, RawSyntaxSpan **spans, unsigned long span_count, SyntaxRoot *root, const char *source, unsigned long offset, int depth)
{
  unsigned long i;
  for (i = 0; i < span_count; ++i) {
    if (i % 2 == 0) {
      print_trivia(file, (RawSyntaxTrivia *) spans[i], root, offset, depth);
    } else {
      print_node(file, (RawSyntaxNode *) spans[i], root, source, offset, depth);
    }
    offset += spans[i]->text_length;
  }
}

void syntax_print(SyntaxRoot *syntax, const char *source, FILE *file)
{
  print_spans(file, syntax->raw->children, syntax->raw->children_count, syntax, source, 0, 0);
}

static void free_trivia(RawSyntaxTrivia *trivia);
static void free_node(RawSyntaxNode *node, SyntaxRoot *root);
static void free_spans(RawSyntaxSpan **spans, unsigned long span_count, SyntaxRoot *root);

static void free_trivia(RawSyntaxTrivia *trivia)
{
  free(trivia->children);
  free(trivia);
}

static void free_node(RawSyntaxNode *node, SyntaxRoot *root)
{
  if (!root->interface.is_token(node->kind)) {
    RawSyntaxTree *tree = (RawSyntaxTree *) node;
    free_spans(tree->children, tree->children_count, root);
  }
  free(node);
}

static void free_spans(RawSyntaxSpan **spans, unsigned long span_count, SyntaxRoot *root)
{
  unsigned long i;
  for (i = 0; i < span_count; ++i) {
    if (i % 2 == 0) {
      free_trivia((RawSyntaxTrivia *) spans[i]);
    } else {
      free_node((RawSyntaxNode *) spans[i], root);
    }
  }
  free(spans);
}

void syntax_free(SyntaxRoot *syntax)
{
  if (syntax) {
    free_spans(syntax->raw->children, syntax->raw->children_count, syntax);
    free(syntax->raw);
    free(syntax);
  }
}

/* syntax builder */

struct SyntaxBuilder {
  Array *trivias; /* RawSyntaxToken */
  Array *spans; /* RawSyntaxSpan */
};

static RawSyntaxTrivia *finish_trivia(SyntaxBuilder *self)
{
  unsigned long    i;
  RawSyntaxTrivia *trivia  = xmalloc(sizeof(RawSyntaxTrivia));
  trivia->children_count   = array_count(self->trivias);
  trivia->children         = memdup(array_data(self->trivias), sizeof(RawSyntaxToken *) * trivia->children_count);
  trivia->span.text_length = 0;
  for (i = 0; i < trivia->children_count; ++i) {
    trivia->span.text_length += trivia->children[i]->node.span.text_length;
  }
  return trivia;
}

static void push_span(SyntaxBuilder *self, RawSyntaxSpan *span)
{
  array_push(self->spans, &span);
}

static void push_node(SyntaxBuilder *self, RawSyntaxNode *node)
{
  RawSyntaxTrivia *trivia = finish_trivia(self);
  push_span(self, (RawSyntaxSpan *) trivia);
  push_span(self, (RawSyntaxSpan *) node);
}

static RawSyntaxSpan **finish_span(
  SyntaxBuilder *self, SyntaxCheckpoint checkpoint, unsigned long *span_count)
{
  RawSyntaxSpan **spans;
  unsigned long   start, end;

  start = checkpoint - checkpoint % 2;
  if (array_count(self->spans) % 2 == 0) {
    RawSyntaxTrivia *trivia = finish_trivia(self);
    push_span(self, (RawSyntaxSpan *) trivia);
  }
  end = array_count(self->spans);

  *span_count = end - start;

  spans = memdup(array_at(self->spans, start), sizeof(RawSyntaxSpan *) * *span_count);
  if (start != checkpoint) {
    RawSyntaxTrivia *trivia  = xmalloc(sizeof(RawSyntaxTrivia));
    trivia->span.text_length = 0;
    trivia->children_count   = 0;
    trivia->children         = NULL;

    spans[0] = (RawSyntaxSpan *) trivia;
  }
  return spans;
}

static unsigned long total_text_length(RawSyntaxSpan **span, unsigned long count)
{
  unsigned long i;
  unsigned long text_length = 0;
  for (i = 0; i < count; ++i) {
    text_length = span[i]->text_length;
  }
  return text_length;
}

SyntaxBuilder *syntax_builder_new(void)
{
  SyntaxBuilder *builder = xmalloc(sizeof(SyntaxBuilder));
  builder->trivias       = array_new(sizeof(RawSyntaxToken *));
  builder->spans         = array_new(sizeof(RawSyntaxSpan *));
  return builder;
}

void syntax_builder_free(SyntaxBuilder *self)
{
  if (self) {
    array_free(self->trivias);
    array_free(self->spans);
    free(self);
  }
}

void syntax_builder_trivia(SyntaxBuilder *self, RawSyntaxKind kind, const char *text, unsigned long text_length)
{
  RawSyntaxToken *token        = xmalloc(sizeof(RawSyntaxToken));
  token->node.span.text_length = text_length;
  token->node.kind             = kind;
  token->text                  = strndup(text, text_length);
  array_push(self->trivias, &token);
}

void syntax_builder_null(SyntaxBuilder *self)
{
  push_node(self, NULL);
}

void syntax_builder_token(SyntaxBuilder *self, RawSyntaxKind kind, const char *text, unsigned long text_length)
{
  RawSyntaxToken *token        = xmalloc(sizeof(RawSyntaxToken));
  token->node.span.text_length = text_length;
  token->node.kind             = kind;
  token->text                  = strndup(text, text_length);
  push_node(self, (RawSyntaxNode *) token);
}

SyntaxCheckpoint syntax_builder_open(SyntaxBuilder *self)
{
  return array_count(self->spans);
}

void syntax_builder_close(SyntaxBuilder *self, RawSyntaxKind kind, SyntaxCheckpoint checkpoint)
{
  RawSyntaxTree *tree         = xmalloc(sizeof(RawSyntaxTree));
  tree->node.kind             = kind;
  tree->children              = finish_span(self, checkpoint, &tree->children_count);
  tree->node.span.text_length = total_text_length(tree->children, tree->children_count);
  push_node(self, (RawSyntaxNode *) tree);
}

SyntaxRoot *syntax_builder_finish(SyntaxBuilder *self, SyntaxInterface *interface)
{
  SyntaxRoot *root            = xmalloc(sizeof(SyntaxRoot));
  root->raw                   = xmalloc(sizeof(RawSyntaxRoot));
  root->raw->children         = finish_span(self, 0, &root->raw->children_count);
  root->raw->span.text_length = total_text_length(root->raw->children, root->raw->children_count);
  root->interface             = *interface;
  return root;
}
