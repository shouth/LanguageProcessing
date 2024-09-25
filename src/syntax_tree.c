/* SPDX-License-Identifier: Apache-2.0 */

#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "syntax_tree.h"
#include "term.h"
#include "util.h"

/* syntax tree */

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
  if (node) {
    if (node->kind >> 8 == SYNTAX_TOKEN) {
      RawSyntaxToken *token = (RawSyntaxToken *) node;
      free(token->text);
    } else {
      RawSyntaxTree *tree = (RawSyntaxTree *) node;
      unsigned long  i;

      for (i = 0; i < tree->children.count; ++i) {
        if (i % 2 == 1) {
          free_trivia((RawSyntaxTrivia *) tree->children.ptr[i].node);
        } else {
          free_node((RawSyntaxNode *) tree->children.ptr[i].node);
        }
      }
      slice_free(&tree->children);
    }
  }

  free(node);
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

static void print_slots(FILE *file, RawSyntaxSlot *slots, unsigned long slot_count, unsigned long offset, int depth, RawSyntaxKindPrinter *kind_printer);

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

  if (!node) {
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
  } else if (node->kind >> 8 == SYNTAX_ROOT) {
    RawSyntaxTree *root = (RawSyntaxTree *) node;

    print_slots(file, root->children.ptr, root->children.count, offset, depth, kind_printer);
  } else if (node->kind >> 8 == SYNTAX_TREE) {
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
    print_slots(file, tree->children.ptr, tree->children.count, offset, depth + 1, kind_printer);
  } else {
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
  }
}

static void print_slots(FILE *file, RawSyntaxSlot *slots, unsigned long slot_count, unsigned long offset, int depth, RawSyntaxKindPrinter *kind_printer)
{
  unsigned long i;

  unsigned long begin = 0;
  unsigned long end   = slot_count;

  if (depth == 0) {
    ++begin;
    --end;
  }

  for (i = begin; i < end; ++i) {
    if (i % 2 == 1) {
      print_trivia(file, (RawSyntaxTrivia *) slots[i].node, offset + slots[i].offset, depth, kind_printer);
    } else {
      print_node(file, (RawSyntaxNode *) slots[i].node, offset + slots[i].offset, depth, kind_printer);
    }
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

SyntaxTree *syntax_tree_child_tree(const SyntaxTree *self, unsigned long index)
{
  RawSyntaxNode *child = self->raw->children.ptr[index * 2].node;

  if (child && child->kind >> 8 == SYNTAX_TREE) {
    SyntaxTree *tree       = xmalloc(sizeof(SyntaxTree));
    tree->node.parent      = self;
    tree->node.span.offset = self->node.span.offset + self->raw->children.ptr[index * 2].offset;
    tree->node.span.ref    = 1;
    tree->raw              = (RawSyntaxTree *) child;
    return tree;
  } else {
    return NULL;
  }
}

SyntaxToken *syntax_tree_child_token(const SyntaxTree *self, unsigned long index)
{
  RawSyntaxNode *child = self->raw->children.ptr[index * 2].node;

  if (child && child->kind >> 8 == SYNTAX_TOKEN) {
    SyntaxToken *token      = xmalloc(sizeof(SyntaxToken));
    token->node.parent      = self;
    token->node.span.offset = self->node.span.offset + self->raw->children.ptr[index * 2].offset;
    token->node.span.ref    = 1;
    token->raw              = (RawSyntaxToken *) child;
    return token;
  } else {
    return NULL;
  }
}

static SyntaxTrivia *syntax_node_adjacent_trivia(const SyntaxNode *self, const RawSyntaxNode *raw, int leading)
{
  const RawSyntaxSlot *slot;
  unsigned long        i;

  for (slot = NULL; !self && !slot; self = (const SyntaxNode *) self->parent) {
    for (i = 0; i < self->parent->raw->children.count; ++i) {
      if (self->parent->raw->children.ptr[i].node == raw) {
        break;
      }
    }

    assert(i < self->parent->raw->children.count);

    if (leading) {
      if (i != 0) {
        slot = &self->parent->raw->children.ptr[i - 1];
      }
    } else {
      if (i != self->parent->raw->children.count - 1) {
        slot = &self->parent->raw->children.ptr[i + 1];
      }
    }
  }

  if (slot) {
    SyntaxTrivia *adjacent = xmalloc(sizeof(SyntaxTrivia));
    adjacent->span.offset  = self->parent->node.span.offset + slot->offset;
    adjacent->span.ref     = 1;
    adjacent->adjacent     = self;
    adjacent->raw          = (const RawSyntaxTrivia *) slot->node;
    return adjacent;
  } else {
    return NULL;
  }
}

SyntaxTrivia *syntax_tree_leading_trivia(const SyntaxTree *self)
{
  return syntax_node_adjacent_trivia((const SyntaxNode *) self, (const RawSyntaxNode *) self->raw, 1);
}

SyntaxTrivia *syntax_tree_trailing_trivia(const SyntaxTree *self)
{
  return syntax_node_adjacent_trivia((const SyntaxNode *) self, (const RawSyntaxNode *) self->raw, 0);
}

SyntaxTrivia *syntax_token_leading_trivia(const SyntaxToken *self)
{
  return syntax_node_adjacent_trivia((const SyntaxNode *) self, (const RawSyntaxNode *) self->raw, 1);
}

SyntaxTrivia *syntax_token_trailing_trivia(const SyntaxToken *self)
{
  return syntax_node_adjacent_trivia((const SyntaxNode *) self, (const RawSyntaxNode *) self->raw, 0);
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
  RawSyntaxNode *node = NULL;
  push_span(self, (RawSyntaxSpan *) node);
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

  if (text) {
    self->trivia->text = xmalloc(self->trivia->span.text_length + 1);
    memcpy(self->trivia->text, text, self->trivia->span.text_length);
    self->trivia->text[self->trivia->span.text_length] = '\0';
  } else {
    self->trivia->text = NULL;
  }
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

  if (text) {
    token->text = xmalloc(text_length + 1);
    memcpy(token->text, text, text_length);
    token->text[text_length] = '\0';
  } else {
    token->text = NULL;
  }

  push_trivia(self);
  push_span(self, (RawSyntaxSpan *) token);
}

SyntaxCheckpoint syntax_builder_open(SyntaxBuilder *self)
{
  return self->spans.count + 1;
}

void syntax_builder_close(SyntaxBuilder *self, RawSyntaxKind kind, SyntaxCheckpoint checkpoint)
{
  RawSyntaxTree *tree = xmalloc(sizeof(RawSyntaxTree));

  assert(checkpoint % 2 == 0);
  assert(self->spans.count % 2 == 1);

  tree->node.kind = kind;

  if (checkpoint > self->spans.count) {
    tree->node.span.text_length = 0;
    slice_alloc(&tree->children, 0);
    push_trivia(self);
  } else {
    unsigned long i;
    unsigned long offset = 0;
    unsigned long count  = self->spans.count - checkpoint;

    slice_alloc(&tree->children, count);
    for (i = 0; i < count; ++i) {
      RawSyntaxSpan *span          = self->spans.ptr[checkpoint + i];
      tree->children.ptr[i].node   = (RawSyntaxNode *) span;
      tree->children.ptr[i].offset = offset;
      if (span) {
        offset += span->text_length;
      }
    }
    tree->node.span.text_length = offset;
    vec_pop(&self->spans, count);
  }
  push_span(self, (RawSyntaxSpan *) tree);
}

SyntaxTree *syntax_builder_finish(SyntaxBuilder *self)
{
  SyntaxTree *root = xmalloc(sizeof(SyntaxTree));

  root->node.parent      = NULL;
  root->node.span.offset = 0;
  root->node.span.ref    = 1;

  assert(self->spans.count % 2 == 1);

  if (self->spans.count > 1) {
    push_trivia(self);
    push_empty(self);
  }
  syntax_builder_close(self, -1, 0);

  {
    RawSyntaxTree *raw = (RawSyntaxTree *) self->spans.ptr[0];
    raw->node.kind     = SYNTAX_ROOT << 8;
    root->raw          = raw;
  }

  syntax_builder_free(self);

  return root;
}
