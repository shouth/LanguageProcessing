#include <stddef.h>

#include "syntax_kind.h"
#include "syntax_tree.h"
#include "token_tree.h"
#include "utility.h"

struct SyntaxTree {
  const SyntaxTree *parent;
  TokenNode        *inner;
  unsigned long     offset;
  unsigned long     flags;
};

static SyntaxTree *syntax_tree_new(const SyntaxTree *parent, TokenNode *inner, unsigned long offset, unsigned long flags)
{
  SyntaxTree *tree = xmalloc(sizeof(SyntaxTree));
  tree->parent     = parent;
  tree->inner      = inner;
  tree->offset     = offset;
  tree->flags      = flags;
  return tree;
}

SyntaxTree *syntax_tree_root(TokenNode *node)
{
  return syntax_tree_new(NULL, node, token_node_trivia_length(node), 1);
}

const TokenNode *syntax_tree_raw(const SyntaxTree *tree)
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
  return token_node_text_length(syntax_tree_raw(tree));
}

unsigned long syntax_tree_trivia_length(const SyntaxTree *tree)
{
  return token_node_trivia_length(syntax_tree_raw(tree));
}

const SyntaxTree *syntax_tree_parent(const SyntaxTree *tree)
{
  return tree->parent;
}

unsigned long syntax_tree_child_count(const SyntaxTree *tree)
{
  const TokenTree *inner = (TokenTree *) tree->inner;
  if (syntax_kind_is_token(inner->kind)) {
    return 0;
  } else {
    return inner->children_count;
  }
}

SyntaxTree *syntax_tree_child(const SyntaxTree *tree, unsigned long index)
{
  const TokenTree *inner = (TokenTree *) tree->inner;
  if (syntax_kind_is_token(inner->kind) || index >= inner->children_count || !inner->children[index]) {
    return NULL;
  } else {
    unsigned long offset = tree->offset;
    unsigned long i;

    for (i = 0; i <= index; ++i) {
      if (i > 0) {
        offset += token_node_trivia_length(inner->children[i]);
      }
      if (i < index) {
        offset += token_node_text_length(inner->children[i]);
      }
    }
    return syntax_tree_new(tree, inner->children[index], offset, 0);
  }
}

SyntaxTree *syntax_tree_extract(SyntaxTree *tree)
{
  if (tree) {
    tree->parent = NULL;
  }
  return tree;
}

SyntaxTree *syntax_tree_clone(const SyntaxTree *tree)
{
  if (tree) {
    return syntax_tree_new(NULL, tree->inner, tree->offset, 0);
  } else {
    return NULL;
  }
}

void syntax_tree_visit(const SyntaxTree *tree, SyntaxTreeVisitor *visitor, void *data)
{
  if (tree) {
    const TokenTree *inner = (const TokenTree *) tree->inner;
    visitor(tree, data, 1);
    if (!syntax_kind_is_token(inner->kind)) {
      unsigned long i;
      for (i = 0; i < inner->children_count; ++i) {
        SyntaxTree *child = syntax_tree_child(tree, i);
        syntax_tree_visit(child, visitor, data);
        syntax_tree_free(child);
      }
    }
    visitor(tree, data, 0);
  }
}

void syntax_tree_free(SyntaxTree *tree)
{
  if (tree) {
    if (tree->flags & 1) {
      token_node_free(tree->inner);
    }
    free(tree);
  }
}
