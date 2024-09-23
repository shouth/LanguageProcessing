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

static void free_spans(RawSyntaxSpan **spans, unsigned long span_count);

static void free_trivia(RawSyntaxTrivia *trivia)
{
  if (trivia) {
    free(trivia->text);
    slice_free(&trivia->pieces);
    free(trivia);
  }
}

static void free_node(RawSyntaxNode *node)
{
  switch (node->node_kind) {
  case RAW_SYNTAX_TOKEN: {
    RawSyntaxToken *token = (RawSyntaxToken *) node;
    free(token->text);
    break;
  }
  case RAW_SYNTAX_ROOT:
  case RAW_SYNTAX_TREE: {
    RawSyntaxTree *tree = (RawSyntaxTree *) node;
    free_spans(tree->children.ptr, tree->children.count);
    break;
  }
  case RAW_SYNTAX_EMPTY:
    /* Nothing to do */
    break;
  }
  free(node);
}

static void free_spans(RawSyntaxSpan **spans, unsigned long span_count)
{
  unsigned long i;
  for (i = 0; i < span_count; ++i) {
    if (i % 2 == 1) {
      free_trivia((RawSyntaxTrivia *) spans[i]);
    } else {
      free_node((RawSyntaxNode *) spans[i]);
    }
  }
  free(spans);
}

static void syntax_node_free(SyntaxNode *self)
{
  if (self) {
    if (self->parent) {
      /* remove from parent */
      syntax_node_free((SyntaxNode *) self->parent);
    }
    if (--self->span.ref == 0) {
      /* reference count is zero, free memory */
      if (!self->parent) {
        /* root node, free raw syntax */
        const RawSyntaxTree *root = ((SyntaxTree *) self)->raw;
        free_node((RawSyntaxNode *) root);
      }
      free(self);
    }
  }
}

void syntax_tree_free(SyntaxTree *self)
{
  syntax_node_free((SyntaxNode *) self);
}

void syntax_token_free(SyntaxToken *self)
{
  syntax_node_free((SyntaxNode *) self);
}

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
  print_indent(file, depth);

  switch (node->node_kind) {
  case RAW_SYNTAX_TOKEN: {
    RawSyntaxToken *token = (RawSyntaxToken *) node;

    TermStyle style = term_default_style();
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

    TermStyle style  = term_default_style();
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
  case RAW_SYNTAX_ROOT: {
    RawSyntaxTree *root = (RawSyntaxTree *) node;

    print_spans(file, root->children.ptr, root->children.count, offset, depth, kind_printer);
    break;
  }
  case RAW_SYNTAX_EMPTY: {
    TermStyle style = term_default_style();

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

void syntax_tree_print(const SyntaxTree *syntax, FILE *file, RawSyntaxKindPrinter *kind_printer)
{
  print_node(file, (RawSyntaxNode *) syntax->raw, syntax->node.span.offset, 0, kind_printer);
}

void syntax_token_print(const SyntaxToken *syntax, FILE *file, RawSyntaxKindPrinter *kind_printer)
{
  print_node(file, (RawSyntaxNode *) syntax->raw, syntax->node.span.offset, 0, kind_printer);
}

/* syntax builder */

struct SyntaxBuilder {
  RawSyntaxTrivia *trivia;
  Vec(RawSyntaxSpan *) spans;
};

static void push_span(SyntaxBuilder *self, RawSyntaxSpan *span)
{
  vec_push(&self->spans, &span, 1);
}

static void push_trivia(SyntaxBuilder *self)
{
  if (!self->trivia) {
    syntax_builder_trivia(self, NULL, NULL, 0);
  }
  push_span(self, (RawSyntaxSpan *) self->trivia);
  self->trivia = NULL;
}

static void push_empty(SyntaxBuilder *self)
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

SyntaxBuilder *syntax_builder_new(void)
{
  SyntaxBuilder *builder = xmalloc(sizeof(SyntaxBuilder));
  builder->trivia        = NULL;
  vec_alloc(&builder->spans, 0);
  push_empty(builder);
  return builder;
}

void syntax_builder_free(SyntaxBuilder *self)
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

void syntax_builder_trivia(SyntaxBuilder *self, const char *text, RawSyntaxTriviaPiece *pieces, unsigned long piece_count)
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

void syntax_builder_empty(SyntaxBuilder *self)
{
  push_trivia(self);
  push_empty(self);
}

void syntax_builder_token(SyntaxBuilder *self, RawSyntaxKind kind, const char *text, unsigned long text_length)
{
  RawSyntaxToken *token        = xmalloc(sizeof(RawSyntaxToken));
  token->node.span.text_length = text_length;
  token->node.kind             = kind;
  token->node.node_kind        = RAW_SYNTAX_TOKEN;
  token->text                  = strndup(text, text_length);

  push_trivia(self);
  push_span(self, (RawSyntaxSpan *) token);
}

SyntaxCheckpoint syntax_builder_open(SyntaxBuilder *self)
{
  return self->spans.count;
}

void syntax_builder_close(SyntaxBuilder *self, RawSyntaxKind kind, SyntaxCheckpoint checkpoint)
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

SyntaxTree *syntax_builder_finish(SyntaxBuilder *self)
{
  SyntaxTree    *root = xmalloc(sizeof(SyntaxTree));
  RawSyntaxTree *raw  = xmalloc(sizeof(RawSyntaxTree));

  root->node.parent      = NULL;
  root->node.span.offset = 0;
  root->node.span.ref    = 1;

  assert(self->spans.count % 2 == 1);

  if (self->spans.count > 1) {
    push_trivia(self);
    push_empty(self);
  }

  slice_alloc(&raw->children, self->spans.count);
  memcpy(raw->children.ptr, self->spans.ptr, sizeof(RawSyntaxSpan *) * raw->children.count);
  vec_pop(&self->spans, raw->children.count);

  raw->node.span.text_length = syntax_span_sum(raw->children.ptr, raw->children.count);
  raw->node.kind             = -1;
  raw->node.node_kind        = RAW_SYNTAX_ROOT;

  root->raw = raw;

  syntax_builder_free(self);

  return root;
}
