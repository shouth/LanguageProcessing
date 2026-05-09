/*
 * syn.c -- syntax
 *
 * SPDX-FileCopyrightText: 2026 Shota Minami
 * SPDX-License-Identifier: Apache-2.0
 */

#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "ds.h"
#include "fmt.h"
#include "syn.h"

char const *syn_kind_to_lexeme(enum syn_kind kind)
{
  switch (kind) {
#define TOK(KIND, LEXEME) \
  case KIND: return LEXEME;

#define SEQ(KIND, TYPE, FIELDS)
#define ALT(KIND, TYPE, OPTIONS)
#define REP(KIND, TYPE, ITEM)

  DEF_SYN(TOK, SEQ, ALT, REP)

#undef TOK
#undef SEQ
#undef ALT
#undef REP

  default: return NULL;
  }
}

char const *syn_kind_to_string(enum syn_kind kind)
{
  switch (kind) {
#define TOK(KIND, LEXEME) \
  case KIND: return #KIND;

#define SEQ(KIND, TYPE, FIELDS) \
  case KIND: return #KIND;

#define ALT(KIND, TYPE, OPTIONS)

#define REP(KIND, TYPE, ITEM) \
  case KIND: return #KIND;

  DEF_SYN(TOK, SEQ, ALT, REP)

#undef TOK
#undef SEQ
#undef ALT
#undef REP

  default: return NULL;
  }
}

#define TOK(KIND, LEXEME)
#define SEQ(KIND, TYPE, FIELDS)

#define OPT(KIND, TYPE, ENUM) \
  case KIND: return ENUM;

#define ALT(KIND, TYPE, OPTIONS) \
  enum TYPE ## _kind TYPE ## _kind(struct TYPE const *n) { \
    switch (((struct syn_node const *) n)->kind) { \
      OPTIONS(OPT) \
      default: return SIZE_ ## KIND; \
    } \
  }

#define REP(KIND, TYPE, ITEM)

DEF_SYN(TOK, SEQ, ALT, REP)

#undef TOK
#undef SEQ
#undef OPT
#undef ALT
#undef REP

void syn_free(struct syn_node *node)
{
  if (!node) {
    return;
  }

  switch (node->kind) {
#define TOK(KIND, LEXEME) \
  case KIND: { \
    struct syn_tok *tok = (struct syn_tok *) node; \
    if (tok->triv) { \
      free(tok->triv->pieces); \
      free(tok->triv->offsets); \
      free(tok->triv); \
    } \
    free(tok); \
    break; \
  }

#define FLD(KIND, TYPE, NAME) \
  syn_free((struct syn_node *) n->NAME);

#define SEQ(KIND, TYPE, FIELDS) \
  case KIND: { \
    struct TYPE *n = (struct TYPE *) node; \
    FIELDS(FLD) \
    free(n->syn.offsets); \
    free(n); \
    break; \
  }

#define ALT(KIND, TYPE, OPTIONS)

#define REP(KIND, TYPE, ITEM) \
  case KIND: { \
    size_t i; \
    struct TYPE *n = (struct TYPE *) node; \
    for (i = 0; i < n->count; ++i) { \
      syn_free((struct syn_node *) n->children[i]); \
    } \
    free(n->children); \
    free(n->syn.offsets); \
    free(n); \
    break; \
  }

  DEF_SYN(TOK, SEQ, ALT, REP)

#undef TOK
#undef FLD
#undef SEQ
#undef ALT
#undef REP

  default: break;
  }
}

size_t syn_text_off(struct syn_node const *node)
{
  return node ? syn_text_off(&node->parent->node) + fw_query(node->parent->offsets, node->index) : 0;
}

size_t syn_text_len(struct syn_node const *node)
{
  if (node->kind >= TOK_BEGIN && node->kind <= TOK_END) {
    struct syn_tok const *tok = (struct syn_tok const *) node;
    return tok->text->len + (tok->triv ? fw_query(tok->triv->offsets, tok->triv->count) : 0);
  } else {
    struct syn_tree const *tree = (struct syn_tree const *) node;
    return fw_query(tree->offsets, syn_child_count(node));
  }
}

struct syn_node *syn_child_at(struct syn_node const *node, size_t index)
{
  switch (node->kind) {
#define TOK(KIND, LEXEME) \
  case KIND: return NULL;

#define FLD_IDX(KIND, TYPE, NAME) \
  index_ ## NAME,

#define FLD_AT(KIND, TYPE, NAME) \
  case index_ ## NAME: return (struct syn_node *) n->NAME;

#define SEQ(KIND, TYPE, FIELDS) \
  case KIND: { \
    enum { FIELDS(FLD_IDX) count }; \
    struct TYPE const *n = (struct TYPE const *) node; \
    (void) n; \
    switch (index) { \
      FIELDS(FLD_AT) \
      default: return NULL; \
    } \
  }

#define ALT(KIND, TYPE, OPTIONS)

#define REP(KIND, TYPE, ITEM) \
  case KIND: { \
    struct TYPE const *n = (struct TYPE const *) node; \
    return index < n->count ? (struct syn_node *) n->children[index] : NULL; \
  }

  DEF_SYN(TOK, SEQ, ALT, REP)

#undef TOK
#undef FLD_IDX
#undef FLD_AT
#undef SEQ
#undef ALT
#undef REP

  default: return NULL;
  }
}

size_t syn_child_count(struct syn_node const *node)
{
  switch (node->kind) {
#define TOK(KIND, LEXEME) \
  case KIND: return 0;

#define FLD(KIND, TYPE, NAME) \
  + 1

#define SEQ(KIND, TYPE, FIELDS) \
  case KIND: return 0 FIELDS(FLD);

#define ALT(KIND, TYPE, OPTIONS)

#define REP(KIND, TYPE, ITEM) \
  case KIND: { \
    struct TYPE const *n = (struct TYPE const *) node; \
    return n->count; \
  }

  DEF_SYN(TOK, SEQ, ALT, REP)

#undef TOK
#undef FLD
#undef SEQ
#undef ALT
#undef REP

  default: return 0;
  }
}

struct syn_triv *syn_triv(struct syn_node const *node)
{
  if (node->kind >= TOK_BEGIN && node->kind <= TOK_END) {
    struct syn_tok const *tok = (struct syn_tok const *) node;
    return tok->triv;
  } else {
    return syn_triv(syn_child_at(node, 0));
  }
}

size_t syn_triv_text_len(struct syn_node const *node)
{
  struct syn_triv const *triv = syn_triv(node);
  return fw_query(triv->offsets, triv->count);
}

static void print_line(enum syn_kind kind, size_t start, size_t end, char const *text, int indent, FILE *out)
{
  struct fmt_style s = { 0 };

  s.color = 0;
  fmt_print(out, &s, "%*s", indent, "");

  s.color = TRIV_BEGIN <= kind && kind <= TRIV_END ? FMT_BRIGHT_BLACK : FMT_BRIGHT_GREEN;
  fmt_print(out, &s, "%s", syn_kind_to_string(kind));

  s.color = 0;
  fmt_print(out, &s, " @ ");

  s.color = FMT_BRIGHT_BLUE;
  fmt_print(out, &s, "%lu", start);

  s.color = 0;
  fmt_print(out, &s, "..");

  s.color = FMT_BRIGHT_BLUE;
  fmt_print(out, &s, "%lu", end);

  if (text) {
    s.color = FMT_BRIGHT_YELLOW;
    fmt_print(out, &s, " \"%s\"", text);
  }

  s.color = 0;
  fmt_print(out, &s, "\n");
}

static void print_tree(struct syn_node const *node, FILE *out, size_t offset, size_t indent)
{
  if (!node) {
    struct fmt_style s = { 0 };

    s.color = 0;
    fmt_print(out, &s, "%*s", (int) indent, "");

    s.color = FMT_BRIGHT_CYAN;
    fmt_print(out, &s, "NULL");

    s.color = 0;
    fmt_print(out, &s, "\n");
  } else if (node->kind >= TOK_BEGIN && node->kind <= TOK_END) {
    struct syn_tok const *n = (struct syn_tok const *) node;
    size_t triv = n->triv ? fw_query(n->triv->offsets, n->triv->count) : 0;

    print_line(node->kind, triv + offset, triv + offset + n->text->len, n->text->str, (int) indent, out);
    if (n->triv) {
      size_t i;
      for (i = 0; i < n->triv->count; ++i) {
        struct syn_triv_piece const *piece = &n->triv->pieces[i];
        size_t start = fw_query(n->triv->offsets, i);
        size_t end = fw_query(n->triv->offsets, i + 1);
        print_line(piece->kind, start + offset, end + offset, NULL, (int) indent + 2, out);
      }
    }
  } else {
    size_t i;
    struct syn_tree const *n = (struct syn_tree const *) node;

    print_line(node->kind, offset, offset + fw_query(n->offsets, syn_child_count(node)), NULL, (int) indent, out);
    for (i = 0; i < syn_child_count(node); ++i) {
      print_tree(syn_child_at(node, i), out, offset + fw_query(n->offsets, i), indent + 2);
    }
  }
}

void syn_print(struct syn_node const *node, FILE *out)
{
  print_tree(node, out, 0, 0);
}

void syn_bldr_init(struct syn_bldr *b, struct sym_ctxt *ctxt)
{
  b->ctxt = ctxt;
  vec_init(&b->trivs);
  vec_init(&b->triv_lens);
  vec_init(&b->stack);
}

void syn_bldr_deinit(struct syn_bldr *b)
{
  vec_deinit(&b->trivs);
  vec_deinit(&b->triv_lens);
  vec_deinit(&b->stack);
}

void syn_bldr_triv(struct syn_bldr *b, enum syn_kind kind, char const *text, size_t len)
{
  struct syn_triv_piece piece;
  piece.kind = kind;
  piece.text = NULL;

  if (text) {
    piece.text = sym_intern(b->ctxt, text, len);
  }

  vec_push(&b->trivs, &piece);
  vec_push(&b->triv_lens, &len);
}

void syn_bldr_tok(struct syn_bldr *b, enum syn_kind kind, char const *text, size_t len)
{
  struct syn_node *syn;
  struct syn_tok *tok = malloc(sizeof(struct syn_tok));
  tok->node.index = 0;
  tok->node.kind = kind;
  tok->node.parent = NULL;
  tok->text = NULL;
  tok->triv = NULL;

  if (text) {
    tok->text = sym_intern(b->ctxt, text, len);
  }

  if (b->trivs.count > 0) {
    tok->triv = malloc(sizeof(struct syn_triv));
    tok->triv->pieces = malloc(sizeof(struct syn_triv_piece) * b->trivs.count);
    tok->triv->offsets = malloc(sizeof(size_t) * b->trivs.count);
    memcpy(tok->triv->pieces, b->trivs.data, sizeof(struct syn_triv_piece) * b->trivs.count);
    memcpy(tok->triv->offsets, b->triv_lens.data, sizeof(size_t) * b->trivs.count);
    tok->triv->count = b->trivs.count;
    fw_build(tok->triv->offsets, tok->triv->count);
    vec_clear(&b->trivs);
    vec_clear(&b->triv_lens);
  }

  syn = &tok->node;
  vec_push(&b->stack, &syn);
}

void syn_bldr_empty(struct syn_bldr *b)
{
  struct syn_node *node = NULL;
  vec_push(&b->stack, &node);
}

syn_ckpt_t syn_bldr_open(struct syn_bldr *b)
{
  return b->stack.count;
}

void syn_bldr_close(struct syn_bldr *b, enum syn_kind kind, syn_ckpt_t ckpt)
{
  switch (kind) {
#define TOK(KIND, LEXEME) \
  case KIND: \
    assert(!"tried to close with token kind " #KIND); \
    break;

#define FLD_IDX(KIND, TYPE, NAME) \
  index_ ## NAME,

#define FLD_SET(KIND, TYPE, NAME) \
  { \
    struct syn_node *child = *vec_at(&b->stack, ckpt + index_ ## NAME); \
    if (child) { \
      child->index = index_ ## NAME; \
      child->parent = &node->syn; \
    } \
    node->NAME = (struct TYPE *) child; \
    node->syn.offsets[index_ ## NAME] = child ? syn_text_len(child) : 0; \
  }

#define SEQ(KIND, TYPE, FIELDS) \
  case KIND: { \
    enum { FIELDS(FLD_IDX) count }; \
    struct syn_node *syn; \
    struct TYPE *node = malloc(sizeof(struct TYPE)); \
    node->syn.node.kind = kind; \
    node->syn.node.index = 0; \
    node->syn.node.parent = NULL; \
    node->syn.offsets = malloc(sizeof(size_t) * count); \
    FIELDS(FLD_SET) \
    fw_build(node->syn.offsets, count); \
    while (b->stack.count > ckpt) { \
      vec_pop(&b->stack); \
    } \
    syn = &node->syn.node; \
    vec_push(&b->stack, &syn); \
    break; \
  }

#define ALT(KIND, TYPE, OPTIONS)

#define REP(KIND, TYPE, ITEM) \
  case KIND: { \
    size_t i; \
    struct syn_node *syn; \
    struct TYPE *node = malloc(sizeof(struct TYPE)); \
    node->syn.node.kind = kind; \
    node->syn.node.index = 0; \
    node->syn.node.parent = NULL; \
    node->count = b->stack.count - ckpt; \
    node->syn.offsets = malloc(sizeof(size_t) * node->count); \
    node->children = malloc(sizeof(struct ITEM *) * node->count); \
    for (i = 0; i < node->count; ++i) { \
      struct syn_node *child = *vec_at(&b->stack, ckpt + i); \
      if (child) { \
        child->index = i; \
        child->parent = &node->syn; \
      } \
      node->children[i] = (struct ITEM *) child; \
      node->syn.offsets[i] = child ? syn_text_len(child) : 0; \
    } \
    fw_build(node->syn.offsets, node->count); \
    while (b->stack.count > ckpt) { \
      vec_pop(&b->stack); \
    } \
    syn = &node->syn.node; \
    vec_push(&b->stack, &syn); \
    break; \
  }

  DEF_SYN(TOK, SEQ, ALT, REP)

#undef TOK
#undef FLD_IDX
#undef FLD_SET
#undef SEQ
#undef ALT
#undef REP

  default: break;
  }
}

struct syn_node *syn_bldr_finish(struct syn_bldr *b)
{
  struct syn_node *result;

  assert(b->stack.count == 1);
  assert(b->trivs.count == 0);
  result = *vec_front(&b->stack);
  vec_clear(&b->stack);
  return result;
}
