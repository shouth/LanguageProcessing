#include <stddef.h>

#include "syntax_kind.h"
#include "syntax_tree.h"
#include "token_tree.h"
#include "utility.h"

struct SyntaxTree {
  const SyntaxTree *parent;
  const TokenNode  *inner;
};

SyntaxTree *syntax_tree_root(const TokenTree *tree)
{
  SyntaxTree *root = xmalloc(sizeof(SyntaxTree));
  root->parent     = NULL;
  root->inner      = (TokenNode *) tree;
  return root;
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
  if (syntax_kind_is_token(inner->kind) || index >= inner->children_count) {
    return NULL;
  } else {
    SyntaxTree *child = xmalloc(sizeof(SyntaxTree));
    child->parent     = tree;
    child->inner      = inner->children[index];
    return child;
  }
}

SyntaxTree *syntax_tree_free(SyntaxTree *tree)
{
  free(tree);
  return NULL;
}
