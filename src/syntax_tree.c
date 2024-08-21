#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "array.h"
#include "syntax_tree.h"
#include "utility.h"

/* syntax tree */

static void print_trivia(FILE *file, RawSyntaxTrivia *trivia, unsigned long offset, int depth, RawSyntaxKindPrinter *kind_printer);
static void print_node(FILE *file, RawSyntaxNode *node, unsigned long offset, int depth, RawSyntaxKindPrinter *kind_printer);
static void print_spans(FILE *file, RawSyntaxSpan **spans, unsigned long span_count, unsigned long offset, int depth, RawSyntaxKindPrinter *kind_printer);

static void print_trivia(FILE *file, RawSyntaxTrivia *trivia, unsigned long offset, int depth, RawSyntaxKindPrinter *kind_printer)
{
  unsigned long i;
  for (i = 0; i < trivia->children_count; ++i) {
    fprintf(file, "%*.s(", (int) depth * 2, "");
    if (kind_printer) {
      kind_printer(trivia->children[i]->node.kind, file);
    } else {
      fprintf(file, "TRIVIA(%d)", trivia->children[i]->node.kind);
    }
    fprintf(file, " @ %ld..%ld)\n", offset, offset + trivia->children[i]->node.span.text_length);
    offset += trivia->children[i]->node.span.text_length;
  }
}

static void print_node(FILE *file, RawSyntaxNode *node, unsigned long offset, int depth, RawSyntaxKindPrinter *kind_printer)
{
  fprintf(file, "%*.s", (int) depth * 2, "");

  if (node->node_kind == RAW_SYNTAX_TOKEN) {
    RawSyntaxToken *token = (RawSyntaxToken *) node;
    if (kind_printer) {
      kind_printer(token->node.kind, file);
    } else {
      fprintf(file, "TOKEN(%d)", token->node.kind);
    }
    fprintf(file, " @ %ld..%ld", offset, offset + token->node.span.text_length);
    fprintf(file, " \"%s\"\n", token->text);
  } else if (node->node_kind == RAW_SYNTAX_TREE) {
    RawSyntaxTree *tree = (RawSyntaxTree *) node;
    if (kind_printer) {
      kind_printer(tree->node.kind, file);
    } else {
      fprintf(file, "TREE(%d)", tree->node.kind);
    }
    fprintf(file, " @ %ld..%ld\n", offset, offset + tree->node.span.text_length);
    print_spans(file, tree->children, tree->children_count, offset, depth + 1, kind_printer);
  } else {
    fprintf(file, "[EMPTY]");
    fprintf(file, " @ %ld..%ld\n", offset, offset);
  }
}

static void print_spans(FILE *file, RawSyntaxSpan **spans, unsigned long span_count, unsigned long offset, int depth, RawSyntaxKindPrinter *kind_printer)
{
  unsigned long i;
  for (i = 0; i < span_count; ++i) {
    /* Spans at depth 0 start with trivia, and alternate between trivia and nodes. *
     * Other spans start with nodes, and alternate between nodes and trivia.       */
    if (i % 2 == (depth != 0)) {
      print_trivia(file, (RawSyntaxTrivia *) spans[i], offset, depth, kind_printer);
    } else {
      print_node(file, (RawSyntaxNode *) spans[i], offset, depth, kind_printer);
    }
    offset += spans[i]->text_length;
  }
}

void raw_syntax_print(RawSyntaxRoot *syntax, FILE *file, RawSyntaxKindPrinter *kind_printer)
{
  print_spans(file, syntax->children, syntax->children_count, 0, 0, kind_printer);
}

static void free_trivia(RawSyntaxTrivia *trivia);
static void free_node(RawSyntaxNode *node, int depth);
static void free_spans(RawSyntaxSpan **spans, unsigned long span_count, int depth);

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

static void free_node(RawSyntaxNode *node, int depth)
{
  if (node->node_kind == RAW_SYNTAX_TOKEN) {
    RawSyntaxToken *token = (RawSyntaxToken *) node;
    free(token->text);
  } else if (node->node_kind == RAW_SYNTAX_TREE) {
    RawSyntaxTree *tree = (RawSyntaxTree *) node;
    free_spans(tree->children, tree->children_count, depth + 1);
  } else {
    /* do nothing for empty nodes */
  }
  free(node);
}

static void free_spans(RawSyntaxSpan **spans, unsigned long span_count, int depth)
{
  unsigned long i;
  for (i = 0; i < span_count; ++i) {
    /* Spans at depth 0 start with trivia, and alternate between trivia and nodes. *
     * Other spans start with nodes, and alternate between nodes and trivia.       */
    if (i % 2 == (depth != 0)) {
      free_trivia((RawSyntaxTrivia *) spans[i]);
    } else {
      free_node((RawSyntaxNode *) spans[i], depth);
    }
  }
  free(spans);
}

void raw_syntax_free(RawSyntaxRoot *syntax)
{
  if (syntax) {
    free_spans(syntax->children, syntax->children_count, 0);
    free(syntax);
  }
}

/* syntax builder */

struct RawSyntaxBuilder {
  Array *trivias; /* RawSyntaxToken */
  Array *spans; /* RawSyntaxSpan */
};

static RawSyntaxTrivia *finish_trivia(RawSyntaxBuilder *self)
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

static void push_span(RawSyntaxBuilder *self, RawSyntaxSpan *span)
{
  array_push(self->spans, &span);
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

RawSyntaxBuilder *raw_syntax_builder_new(void)
{
  RawSyntaxBuilder *builder = xmalloc(sizeof(RawSyntaxBuilder));
  builder->trivias          = array_new(sizeof(RawSyntaxToken *));
  builder->spans            = array_new(sizeof(RawSyntaxSpan *));
  return builder;
}

void raw_syntax_builder_free(RawSyntaxBuilder *self)
{
  if (self) {
    array_free(self->trivias);
    array_free(self->spans);
    free(self);
  }
}

void raw_syntax_builder_trivia(RawSyntaxBuilder *self, RawSyntaxKind kind, const char *text, unsigned long text_length)
{
  RawSyntaxToken *token        = xmalloc(sizeof(RawSyntaxToken));
  token->node.span.text_length = text_length;
  token->node.kind             = kind;
  token->text                  = strndup(text, text_length);
  array_push(self->trivias, &token);
}

void raw_syntax_builder_empty(RawSyntaxBuilder *self)
{
  RawSyntaxNode *node    = xmalloc(sizeof(RawSyntaxNode));
  node->span.text_length = 0;
  node->node_kind        = RAW_SYNTAX_EMPTY;
  node->kind             = -1;

  push_span(self, (RawSyntaxSpan *) finish_trivia(self));
  push_span(self, (RawSyntaxSpan *) node);
}

void raw_syntax_builder_token(RawSyntaxBuilder *self, RawSyntaxKind kind, const char *text, unsigned long text_length)
{
  RawSyntaxToken *token        = xmalloc(sizeof(RawSyntaxToken));
  token->node.span.text_length = text_length;
  token->node.kind             = kind;
  token->node.node_kind        = RAW_SYNTAX_TOKEN;
  token->text                  = strndup(text, text_length);

  push_span(self, (RawSyntaxSpan *) finish_trivia(self));
  push_span(self, (RawSyntaxSpan *) token);
}

RawSyntaxCheckpoint raw_syntax_builder_open(RawSyntaxBuilder *self)
{
  return array_count(self->spans);
}

void raw_syntax_builder_close(RawSyntaxBuilder *self, RawSyntaxKind kind, RawSyntaxCheckpoint checkpoint)
{
  RawSyntaxTree *tree = xmalloc(sizeof(RawSyntaxTree));

  assert(checkpoint % 2 == 0);
  assert(array_count(self->spans) % 2 == 0);

  if (checkpoint == array_count(self->spans)) {
    tree->children_count = 0;
    tree->children       = NULL;
    push_span(self, (RawSyntaxSpan *) finish_trivia(self));
  } else {
    unsigned long start = checkpoint + 1;
    unsigned long end   = array_count(self->spans);

    tree->children_count = end - start;
    tree->children       = memdup(array_at(self->spans, start), sizeof(RawSyntaxSpan *) * tree->children_count);
    array_pop_count(self->spans, tree->children_count);
  }

  tree->node.kind             = kind;
  tree->node.node_kind        = RAW_SYNTAX_TREE;
  tree->node.span.text_length = syntax_span_sum(tree->children, tree->children_count);
  push_span(self, (RawSyntaxSpan *) tree);
}

RawSyntaxRoot *raw_syntax_builder_finish(RawSyntaxBuilder *self)
{
  RawSyntaxRoot *root = xmalloc(sizeof(RawSyntaxRoot));

  assert(array_count(self->spans) % 2 == 0);
  push_span(self, (RawSyntaxSpan *) finish_trivia(self));

  array_fit(self->spans);
  root->children_count   = array_count(self->spans);
  root->children         = array_steal(self->spans);
  root->span.text_length = syntax_span_sum(root->children, root->children_count);

  self->spans = NULL;
  raw_syntax_builder_free(self);

  return root;
}
