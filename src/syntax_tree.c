#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "array.h"
#include "string.h"
#include "syntax_kind.h"
#include "syntax_tree.h"
#include "utility.h"

struct SyntaxTree {
  const SyntaxTree *parent;
  RawSyntaxNode    *inner;
  unsigned long     offset;
  unsigned long     ref;
};

struct SyntaxBuilder {
  Array *parents;
  Array *children;
  Array *leading_trivia;
  Array *trailing_trivia;
};

unsigned long raw_syntax_node_text_length(const RawSyntaxNode *node)
{
  if (!node) {
    return 0;
  } else if (syntax_kind_is_token(node->kind)) {
    RawSyntaxToken *token = (RawSyntaxToken *) node;
    return string_length(token->string);
  } else {
    RawSyntaxTree *tree = (RawSyntaxTree *) node;
    return tree->text_length;
  }
}

unsigned long raw_syntax_node_trivia_length(const RawSyntaxNode *node)
{
  if (!node) {
    return 0;
  } else if (syntax_kind_is_token(node->kind)) {
    RawSyntaxToken *token = (RawSyntaxToken *) node;
    unsigned long   i;
    unsigned long   result = 0;
    for (i = 0; i < token->leading_trivia_count; ++i) {
      result += string_length(token->leading_trivia[i].string);
    }
    return result;
  } else {
    RawSyntaxTree *tree = (RawSyntaxTree *) node;
    return tree->children_count ? raw_syntax_node_trivia_length(tree->children[0]) : 0;
  }
}

static void raw_syntax_node_print_impl(const RawSyntaxNode *node, unsigned long depth, unsigned long offset)
{
  printf("%*.s", (int) depth * 2, "");
  if (!node) {
    printf("(NULL)\n");
  } else if (syntax_kind_is_token(node->kind)) {
    RawSyntaxToken *token = (RawSyntaxToken *) node;
    offset += raw_syntax_node_trivia_length(node);
    printf("%s @ %lu..%lu \"%s\"\n", syntax_kind_to_string(token->kind), offset, offset + string_length(token->string), string_data(token->string));
  } else {
    unsigned long  i;
    RawSyntaxTree *tree        = (RawSyntaxTree *) node;
    unsigned long  tree_offset = offset + raw_syntax_node_trivia_length(node);
    printf("%s @ %lu..%lu\n", syntax_kind_to_string(tree->kind), tree_offset, tree_offset + tree->text_length);
    for (i = 0; i < tree->children_count; ++i) {
      raw_syntax_node_print_impl(tree->children[i], depth + 1, offset);
      offset += raw_syntax_node_trivia_length(tree->children[i]);
      offset += raw_syntax_node_text_length(tree->children[i]);
    }
  }
}

void raw_syntax_node_print(const RawSyntaxNode *node)
{
  raw_syntax_node_print_impl(node, 0, 0);
}

void raw_syntax_node_free(RawSyntaxNode *node)
{
  if (node) {
    if (syntax_kind_is_token(node->kind)) {
      RawSyntaxToken *token = (RawSyntaxToken *) node;
      free(token->leading_trivia);
      free(token->trailing_trivia);
      free(token);
    } else {
      RawSyntaxTree *tree = (RawSyntaxTree *) node;
      unsigned long  i;
      for (i = 0; i < tree->children_count; ++i) {
        raw_syntax_node_free(tree->children[i]);
      }
      free(tree->children);
      free(tree);
    }
  }
}

static SyntaxTree *syntax_tree_new(const SyntaxTree *parent, RawSyntaxNode *inner, unsigned long offset)
{
  SyntaxTree *tree = xmalloc(sizeof(SyntaxTree));
  tree->parent     = syntax_tree_ref(parent);
  tree->inner      = inner;
  tree->offset     = offset;
  tree->ref        = 1;
  return tree;
}

const SyntaxTree *syntax_tree_ref(const SyntaxTree *tree)
{
  SyntaxTree *mutable_tree = (SyntaxTree *) tree;
  if (tree) {
    ++mutable_tree->ref;
  }
  return tree;
}

void syntax_tree_unref(const SyntaxTree *tree)
{
  SyntaxTree *mutable_tree = (SyntaxTree *) tree;
  if (tree && --mutable_tree->ref == 0) {
    if (tree->parent) {
      syntax_tree_unref(tree->parent);
    } else {
      raw_syntax_node_free(tree->inner);
    }
    free(mutable_tree);
  }
}

const RawSyntaxNode *syntax_tree_raw(const SyntaxTree *tree)
{
  return tree->inner;
}

SyntaxKind syntax_tree_kind(const SyntaxTree *tree)
{
  return syntax_tree_raw(tree)->kind;
}

unsigned long syntax_tree_offset(const SyntaxTree *tree)
{
  return tree->offset;
}

unsigned long syntax_tree_text_length(const SyntaxTree *tree)
{
  return raw_syntax_node_text_length(syntax_tree_raw(tree));
}

unsigned long syntax_tree_trivia_length(const SyntaxTree *tree)
{
  return raw_syntax_node_trivia_length(syntax_tree_raw(tree));
}

const SyntaxTree *syntax_tree_parent(const SyntaxTree *tree)
{
  return tree->parent;
}

unsigned long syntax_tree_child_count(const SyntaxTree *tree)
{
  const RawSyntaxTree *inner = (RawSyntaxTree *) tree->inner;
  if (syntax_kind_is_token(inner->kind)) {
    return 0;
  } else {
    return inner->children_count;
  }
}

SyntaxTree *syntax_tree_child(const SyntaxTree *tree, unsigned long index)
{
  const RawSyntaxTree *inner = (RawSyntaxTree *) tree->inner;
  if (syntax_kind_is_token(inner->kind) || index >= inner->children_count || !inner->children[index]) {
    return NULL;
  } else {
    unsigned long offset = tree->offset;
    unsigned long i;

    for (i = 0; i <= index; ++i) {
      if (i > 0) {
        offset += raw_syntax_node_trivia_length(inner->children[i]);
      }
      if (i < index) {
        offset += raw_syntax_node_text_length(inner->children[i]);
      }
    }
    return syntax_tree_new(tree, inner->children[index], offset);
  }
}

void syntax_tree_visit(const SyntaxTree *tree, SyntaxTreeVisitor *visitor, void *data)
{
  if (tree) {
    const RawSyntaxTree *inner = (const RawSyntaxTree *) tree->inner;
    if (visitor(tree, data, 1)) {
      if (!syntax_kind_is_token(inner->kind)) {
        unsigned long i;
        for (i = 0; i < inner->children_count; ++i) {
          SyntaxTree *child = syntax_tree_child(tree, i);
          syntax_tree_visit(child, visitor, data);
          syntax_tree_unref(child);
        }
      }
      visitor(tree, data, 0);
    }
  }
}

SyntaxBuilder *syntax_builder_new(void)
{
  SyntaxBuilder *builder   = xmalloc(sizeof(SyntaxBuilder));
  builder->parents         = array_new(sizeof(unsigned long));
  builder->children        = array_new(sizeof(RawSyntaxNode *));
  builder->leading_trivia  = array_new(sizeof(RawSyntaxTrivia));
  builder->trailing_trivia = array_new(sizeof(RawSyntaxTrivia));
  return builder;
}

void syntax_builder_free(SyntaxBuilder *builder)
{
  if (builder) {
    array_free(builder->parents);
    array_free(builder->children);
    array_free(builder->leading_trivia);
    array_free(builder->trailing_trivia);
    free(builder);
  }
}

unsigned long syntax_builder_checkpoint(SyntaxBuilder *builder)
{
  return array_count(builder->children);
}

void syntax_builder_start_tree(SyntaxBuilder *builder)
{
  syntax_builder_start_tree_at(builder, syntax_builder_checkpoint(builder));
}

void syntax_builder_start_tree_at(SyntaxBuilder *builder, unsigned long checkpoint)
{
  array_push(builder->parents, &checkpoint);
}

void syntax_builder_end_tree(SyntaxBuilder *builder, SyntaxKind kind)
{
  unsigned long   i;
  unsigned long   checkpoint = *(unsigned long *) array_back(builder->parents);
  RawSyntaxNode **children   = (RawSyntaxNode **) array_at(builder->children, checkpoint);
  unsigned long   count      = array_count(builder->children) - checkpoint;

  RawSyntaxTree *tree = xmalloc(sizeof(RawSyntaxTree));
  tree->kind          = kind;
  tree->text_length   = 0;
  for (i = 0; i < count; ++i) {
    if (i > 0) {
      tree->text_length += raw_syntax_node_trivia_length(children[i]);
    }
    tree->text_length += raw_syntax_node_text_length(children[i]);
  }

  tree->children_count = count;
  tree->children       = dup(children, sizeof(RawSyntaxNode *), count);

  array_pop(builder->parents);
  array_pop_count(builder->children, count);
  array_push(builder->children, &tree);
}

void syntax_builder_null(SyntaxBuilder *builder)
{
  RawSyntaxNode *node = NULL;
  array_push(builder->children, &node);
}

void syntax_builder_trivia(SyntaxBuilder *builder, SyntaxKind kind, const String *text, int leading)
{
  RawSyntaxTrivia trivia;
  trivia.kind   = kind;
  trivia.string = text;

  if (leading) {
    array_push(builder->leading_trivia, &trivia);
  } else {
    array_push(builder->trailing_trivia, &trivia);
  }
}

void syntax_builder_token(SyntaxBuilder *builder, SyntaxKind kind, const String *text)
{
  RawSyntaxToken *token = xmalloc(sizeof(RawSyntaxToken));
  token->kind           = kind;
  token->string         = text;

  token->leading_trivia_count  = array_count(builder->leading_trivia);
  token->leading_trivia        = dup(array_data(builder->leading_trivia), sizeof(RawSyntaxTrivia), token->leading_trivia_count);
  token->trailing_trivia_count = array_count(builder->trailing_trivia);
  token->trailing_trivia       = dup(array_data(builder->trailing_trivia), sizeof(RawSyntaxTrivia), token->trailing_trivia_count);
  array_push(builder->children, &token);

  array_clear(builder->leading_trivia);
  array_clear(builder->trailing_trivia);
}

SyntaxTree *syntax_builder_build(SyntaxBuilder *builder)
{
  RawSyntaxNode **root = (RawSyntaxNode **) array_front(builder->children);
  SyntaxTree     *tree = syntax_tree_new(NULL, *root, raw_syntax_node_trivia_length(*root));
  syntax_builder_free(builder);
  return tree;
}
