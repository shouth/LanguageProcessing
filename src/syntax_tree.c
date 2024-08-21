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
    printf(" @ %ld..%ld)\n", offset, offset + trivia->children[i]->node.span.text_length);
    offset += trivia->children[i]->node.span.text_length;
  }
}

static void print_node(FILE *file, RawSyntaxNode *node, SyntaxRoot *root, const char *source, unsigned long offset, int depth)
{
  printf("%*.s", (int) depth * 2, "");
  root->interface.print_kind(file, node->kind);
  printf(" @ %ld..%ld", offset, offset + node->span.text_length);

  if (root->interface.is_token(node->kind)) {
    printf(" \"%.*s\"\n", (int) node->span.text_length, source + offset);
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
    /* Spans at depth 0 start with trivia, and alternate between trivia and nodes. *
     * Other spans start with nodes, and alternate between nodes and trivia.       */
    if (i % 2 == (depth != 0)) {
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
static void free_node(RawSyntaxNode *node, SyntaxRoot *root, int depth);
static void free_spans(RawSyntaxSpan **spans, unsigned long span_count, SyntaxRoot *root, int depth);

static void free_trivia(RawSyntaxTrivia *trivia)
{
  unsigned long i;
  for (i = 0; i < trivia->children_count; ++i) {
    free(trivia->children[i]->text);
    free(trivia->children[i]);
  }
  free(trivia->children);
  free(trivia);
}

static void free_node(RawSyntaxNode *node, SyntaxRoot *root, int depth)
{
  if (node->kind != 0) {
    if (root->interface.is_token(node->kind)) {
      RawSyntaxToken *token = (RawSyntaxToken *) node;
      free(token->text);
    } else {
      RawSyntaxTree *tree = (RawSyntaxTree *) node;
      free_spans(tree->children, tree->children_count, root, depth + 1);
    }
  }
  free(node);
}

static void free_spans(RawSyntaxSpan **spans, unsigned long span_count, SyntaxRoot *root, int depth)
{
  unsigned long i;
  for (i = 0; i < span_count; ++i) {
    /* Spans at depth 0 start with trivia, and alternate between trivia and nodes. *
     * Other spans start with nodes, and alternate between nodes and trivia.       */
    if (i % 2 == (depth != 0)) {
      free_trivia((RawSyntaxTrivia *) spans[i]);
    } else {
      free_node((RawSyntaxNode *) spans[i], root, depth);
    }
  }
  free(spans);
}

void syntax_free(SyntaxRoot *syntax)
{
  if (syntax) {
    free_spans(syntax->raw->children, syntax->raw->children_count, syntax, 0);
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
  array_clear(self->trivias);
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

static unsigned long syntax_span_sum(RawSyntaxSpan **span, unsigned long count)
{
  unsigned long i;
  unsigned long text_length = 0;
  for (i = 0; i < count; ++i) {
    text_length += span[i]->text_length;
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

void syntax_builder_empty(SyntaxBuilder *self)
{
  RawSyntaxNode *node    = xmalloc(sizeof(RawSyntaxNode));
  node->span.text_length = 0;
  node->kind             = 0;
  push_node(self, node);
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
  RawSyntaxTree *tree = xmalloc(sizeof(RawSyntaxTree));

  assert(checkpoint % 2 == 0);
  assert(array_count(self->spans) % 2 == 0);

  if (checkpoint == array_count(self->spans)) {
    tree->children_count = 0;
    tree->children       = NULL;
  } else {
    unsigned long start = checkpoint + 1;
    unsigned long end   = array_count(self->spans);

    tree->children_count = end - start;
    tree->children       = memdup(array_at(self->spans, start), sizeof(RawSyntaxSpan *) * tree->children_count);
  }

  tree->node.kind             = kind;
  tree->node.span.text_length = syntax_span_sum(tree->children, tree->children_count);

  array_pop_count(self->spans, tree->children_count);
  array_push(self->spans, &tree);
}

SyntaxRoot *syntax_builder_finish(SyntaxBuilder *self, SyntaxInterface *interface)
{
  SyntaxRoot *root;

  RawSyntaxTrivia *trivia = finish_trivia(self);
  push_span(self, (RawSyntaxSpan *) trivia);

  assert(array_count(self->spans) % 2 == 1);

  root            = xmalloc(sizeof(SyntaxRoot));
  root->interface = *interface;

  root->raw = xmalloc(sizeof(RawSyntaxRoot));
  array_fit(self->spans);
  root->raw->children_count   = array_count(self->spans);
  root->raw->children         = array_steal(self->spans);
  root->raw->span.text_length = syntax_span_sum(root->raw->children, root->raw->children_count);

  self->spans = NULL;
  syntax_builder_free(self);

  return root;
}
