/* SPDX-License-Identifier: Apache-2.0 */

#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "syntax_tree.h"
#include "term.h"
#include "utility.h"

/* syntax tree */

static void print_trivia(FILE *file, RawSyntaxTrivia *trivia, unsigned long offset, unsigned long depth, RawSyntaxKindPrinter *kind_printer);
static void print_node(FILE *file, RawSyntaxNode *node, unsigned long offset, unsigned long depth, RawSyntaxKindPrinter *kind_printer);
static void print_spans(FILE *file, RawSyntaxSpan **spans, unsigned long span_count, unsigned long offset, int depth, RawSyntaxKindPrinter *kind_printer);

static void print_indent(FILE *file, unsigned long depth)
{
  unsigned long i;
  TermStyle     style;

  style            = term_default_style();
  style.foreground = TERM_COLOR_256 | 0x666666;
  style.intensity  = TERM_INTENSITY_FAINT;
  term_style(file, &style);
  for (i = 0; i < depth; ++i) {
    fprintf(file, "│ ");
  }
  term_style(file, NULL);
}

static void print_trivia(FILE *file, RawSyntaxTrivia *trivia, unsigned long offset, unsigned long depth, RawSyntaxKindPrinter *kind_printer)
{
  unsigned long i;
  TermStyle     style;

  for (i = 0; i < trivia->pieces.count; ++i) {
    print_indent(file, depth);

    style           = term_default_style();
    style.intensity = TERM_INTENSITY_FAINT;
    term_print(file, &style, "(");

    style.foreground = TERM_COLOR_256 | MONOKAI_GREEN;
    if (kind_printer) {
      term_style(file, &style);
      kind_printer(trivia->pieces.ptr[i].kind, file);
      term_style(file, NULL);
    } else {
      term_print(file, &style, "TRIVIA(%d)", trivia->pieces.ptr[i].kind);
    }

    style.foreground = TERM_COLOR_NONE;
    term_print(file, &style, ")");

    style.foreground = TERM_COLOR_NONE;
    term_print(file, &style, " @ ");

    style.foreground = TERM_COLOR_256 | MONOKAI_PURPLE;
    term_print(file, &style, "%ld", offset);

    style.foreground = TERM_COLOR_NONE;
    term_print(file, &style, "..");

    style.foreground = TERM_COLOR_256 | MONOKAI_PURPLE;
    term_print(file, &style, "%ld", offset + trivia->pieces.ptr[i].span.text_length);

    fprintf(file, "\n");
    offset += trivia->pieces.ptr[i].span.text_length;
  }
}

static void print_node(FILE *file, RawSyntaxNode *node, unsigned long offset, unsigned long depth, RawSyntaxKindPrinter *kind_printer)
{
  TermStyle style;

  print_indent(file, depth);

  switch (node->node_kind) {
  case RAW_SYNTAX_TOKEN: {
    RawSyntaxToken *token = (RawSyntaxToken *) node;

    style = term_default_style();
    if (token->node.kind) {
      /* normal token */
      style.foreground = TERM_COLOR_256 | MONOKAI_GREEN;
    } else {
      /* ERROR token */
      style.foreground = TERM_COLOR_256 | MONOKAI_RED;
    }
    if (kind_printer) {
      term_style(file, &style);
      kind_printer(token->node.kind, file);
      term_style(file, NULL);
    } else {
      term_print(file, &style, "TOKEN(%d)", token->node.kind);
    }

    style.foreground = TERM_COLOR_NONE;
    term_print(file, &style, " @ ");

    style.foreground = TERM_COLOR_256 | MONOKAI_PURPLE;
    term_print(file, &style, "%ld", offset);

    style.foreground = TERM_COLOR_NONE;
    term_print(file, &style, "..");

    style.foreground = TERM_COLOR_256 | MONOKAI_PURPLE;
    term_print(file, &style, "%ld", offset + token->node.span.text_length);

    if (token->text) {
      style.foreground = TERM_COLOR_256 | MONOKAI_YELLOW;
      term_print(file, &style, " \"%s\"", token->text);
    }

    fprintf(file, "\n");
    break;
  }
  case RAW_SYNTAX_TREE: {
    RawSyntaxTree *tree = (RawSyntaxTree *) node;

    style            = term_default_style();
    style.foreground = TERM_COLOR_256 | MONOKAI_GREEN;
    if (kind_printer) {
      term_style(file, &style);
      kind_printer(tree->node.kind, file);
      term_style(file, NULL);
    } else {
      term_print(file, &style, "TREE(%d)", tree->node.kind);
    }

    style.foreground = TERM_COLOR_NONE;
    term_print(file, &style, " @ ");

    style.foreground = TERM_COLOR_256 | MONOKAI_PURPLE;
    term_print(file, &style, "%ld", offset);

    style.foreground = TERM_COLOR_NONE;
    term_print(file, &style, "..");

    style.foreground = TERM_COLOR_256 | MONOKAI_PURPLE;
    term_print(file, &style, "%ld", offset + tree->node.span.text_length);

    fprintf(file, "\n");
    print_spans(file, tree->children.ptr, tree->children.count, offset, depth + 1, kind_printer);
    break;
  }
  case RAW_SYNTAX_EMPTY: {
    style = term_default_style();

    style.foreground = TERM_COLOR_NONE;
    term_print(file, &style, "[");

    style.foreground = TERM_COLOR_256 | MONOKAI_BLUE;
    term_print(file, &style, "EMPTY");

    style.foreground = TERM_COLOR_NONE;
    term_print(file, &style, "]");

    style.foreground = TERM_COLOR_NONE;
    term_print(file, &style, " @ ");

    style.foreground = TERM_COLOR_256 | MONOKAI_PURPLE;
    term_print(file, &style, "%ld", offset);

    style.foreground = TERM_COLOR_NONE;
    term_print(file, &style, "..");

    style.foreground = TERM_COLOR_256 | MONOKAI_PURPLE;
    term_print(file, &style, "%ld", offset);

    fprintf(file, "\n");
    break;
  }
  }
}

static void print_spans(FILE *file, RawSyntaxSpan **spans, unsigned long span_count, unsigned long offset, int depth, RawSyntaxKindPrinter *kind_printer)
{
  unsigned long i;

  unsigned long begin = 0;
  unsigned long end   = span_count;

  if (depth == 0) {
    ++begin;
    --end;
  }

  for (i = begin; i < end; ++i) {
    if (i % 2 == 1) {
      print_trivia(file, (RawSyntaxTrivia *) spans[i], offset, depth, kind_printer);
    } else {
      print_node(file, (RawSyntaxNode *) spans[i], offset, depth, kind_printer);
    }
    offset += spans[i]->text_length;
  }
}

void raw_syntax_root_print(const RawSyntaxRoot *syntax, FILE *file, RawSyntaxKindPrinter *kind_printer)
{
  print_spans(file, syntax->children.ptr, syntax->children.count, 0, 0, kind_printer);
}

static void free_spans(RawSyntaxSpan **spans, unsigned long span_count, int depth);

static void free_trivia(RawSyntaxTrivia *trivia)
{
  if (trivia) {
    free(trivia->text);
    slice_free(&trivia->pieces);
    free(trivia);
  }
}

static void free_node(RawSyntaxNode *node, int depth)
{
  switch (node->node_kind) {
  case RAW_SYNTAX_TOKEN: {
    RawSyntaxToken *token = (RawSyntaxToken *) node;
    free(token->text);
    break;
  }
  case RAW_SYNTAX_TREE: {
    RawSyntaxTree *tree = (RawSyntaxTree *) node;
    free_spans(tree->children.ptr, tree->children.count, depth + 1);
    break;
  }
  case RAW_SYNTAX_EMPTY:
    /* Nothing to do */
    break;
  }
  free(node);
}

static void free_spans(RawSyntaxSpan **spans, unsigned long span_count, int depth)
{
  unsigned long i;
  for (i = 0; i < span_count; ++i) {
    if (i % 2 == 1) {
      free_trivia((RawSyntaxTrivia *) spans[i]);
    } else {
      free_node((RawSyntaxNode *) spans[i], depth);
    }
  }
  free(spans);
}

void raw_syntax_root_free(RawSyntaxRoot *syntax)
{
  if (syntax) {
    free_spans(syntax->children.ptr, syntax->children.count, 0);
    free(syntax);
  }
}

const RawSyntaxNode *raw_syntax_root_node(const RawSyntaxRoot *syntax, unsigned long index)
{
  return (const RawSyntaxNode *) syntax->children.ptr[(index << 1) | 1];
}

const RawSyntaxTrivia *raw_syntax_root_trivia(const RawSyntaxRoot *syntax, unsigned long index)
{
  return (const RawSyntaxTrivia *) syntax->children.ptr[index << 1];
}

const RawSyntaxNode *raw_syntax_tree_node(const RawSyntaxTree *syntax, unsigned long index)
{
  return (const RawSyntaxNode *) syntax->children.ptr[index << 1];
}

const RawSyntaxTrivia *raw_syntax_tree_trivia(const RawSyntaxTree *syntax, unsigned long index)
{
  return (const RawSyntaxTrivia *) syntax->children.ptr[(index << 1) | 1];
}

/* syntax builder */

struct RawSyntaxBuilder {
  RawSyntaxTrivia *trivia;
  Vec(RawSyntaxSpan *) spans;
};

static void push_span(RawSyntaxBuilder *self, RawSyntaxSpan *span)
{
  vec_push(&self->spans, &span, 1);
}

static void push_trivia(RawSyntaxBuilder *self)
{
  if (!self->trivia) {
    raw_syntax_builder_trivia(self, NULL, NULL, 0);
  }
  push_span(self, (RawSyntaxSpan *) self->trivia);
  self->trivia = NULL;
}

static void push_empty(RawSyntaxBuilder *self)
{
  RawSyntaxNode *node    = xmalloc(sizeof(RawSyntaxNode));
  node->span.text_length = 0;
  node->node_kind        = RAW_SYNTAX_EMPTY;
  node->kind             = -1;

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

RawSyntaxBuilder *raw_syntax_builder_new(void)
{
  RawSyntaxBuilder *builder = xmalloc(sizeof(RawSyntaxBuilder));
  builder->trivia           = NULL;
  vec_alloc(&builder->spans, 0);
  push_empty(builder);
  return builder;
}

void raw_syntax_builder_free(RawSyntaxBuilder *self)
{
  /* TODO: update functions to free memories even if building is not finished */
  if (self) {
    if (self->trivia) {
      free_trivia(self->trivia);
    }
    vec_free(&self->spans);
    free(self);
  }
}

void raw_syntax_builder_trivia(RawSyntaxBuilder *self, const char *text, RawSyntaxTriviaPiece *pieces, unsigned long piece_count)
{
  unsigned long i;

  if (self->trivia) {
    free_trivia(self->trivia);
  }
  self->trivia = xmalloc(sizeof(RawSyntaxTrivia));

  slice_alloc(&self->trivia->pieces, piece_count);
  if (piece_count > 0) {
    memcpy(self->trivia->pieces.ptr, pieces, sizeof(RawSyntaxTriviaPiece) * piece_count);
  }

  self->trivia->span.text_length = 0;
  for (i = 0; i < self->trivia->pieces.count; ++i) {
    self->trivia->span.text_length += self->trivia->pieces.ptr[i].span.text_length;
  }
  self->trivia->text = strndup(text, self->trivia->span.text_length);
}

void raw_syntax_builder_empty(RawSyntaxBuilder *self)
{
  push_trivia(self);
  push_empty(self);
}

void raw_syntax_builder_token(RawSyntaxBuilder *self, RawSyntaxKind kind, const char *text, unsigned long text_length)
{
  RawSyntaxToken *token        = xmalloc(sizeof(RawSyntaxToken));
  token->node.span.text_length = text_length;
  token->node.kind             = kind;
  token->node.node_kind        = RAW_SYNTAX_TOKEN;
  token->text                  = strndup(text, text_length);

  push_trivia(self);
  push_span(self, (RawSyntaxSpan *) token);
}

RawSyntaxCheckpoint raw_syntax_builder_open(RawSyntaxBuilder *self)
{
  return self->spans.count;
}

void raw_syntax_builder_close(RawSyntaxBuilder *self, RawSyntaxKind kind, RawSyntaxCheckpoint checkpoint)
{
  RawSyntaxTree *tree = xmalloc(sizeof(RawSyntaxTree));

  assert(checkpoint % 2 == 1);
  assert(self->spans.count % 2 == 1);

  if (checkpoint == self->spans.count) {
    slice_alloc(&tree->children, 0);
    push_trivia(self);
  } else {
    unsigned long start = checkpoint + 1;
    unsigned long end   = self->spans.count;

    slice_alloc(&tree->children, end - start);
    memcpy(tree->children.ptr, self->spans.ptr + start, sizeof(RawSyntaxSpan *) * tree->children.count);

    vec_pop(&self->spans, tree->children.count);
  }

  tree->node.kind             = kind;
  tree->node.node_kind        = RAW_SYNTAX_TREE;
  tree->node.span.text_length = syntax_span_sum(tree->children.ptr, tree->children.count);
  push_span(self, (RawSyntaxSpan *) tree);
}

RawSyntaxRoot *raw_syntax_builder_finish(RawSyntaxBuilder *self)
{
  RawSyntaxRoot *root = xmalloc(sizeof(RawSyntaxRoot));

  assert(self->spans.count % 2 == 1);

  if (self->spans.count > 1) {
    push_trivia(self);
    push_empty(self);
  }

  slice_alloc(&root->children, self->spans.count);
  memcpy(root->children.ptr, self->spans.ptr, sizeof(RawSyntaxSpan *) * root->children.count);
  vec_pop(&self->spans, root->children.count);
  root->span.text_length = syntax_span_sum(root->children.ptr, root->children.count);

  raw_syntax_builder_free(self);

  return root;
}
