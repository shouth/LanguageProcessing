#include <stddef.h>

#include "syntax_kind.h"
#include "syntax_tree.h"
#include "token_tree.h"
#include "utility.h"

struct SyntaxTree {
  const SyntaxTree *parent;
  const TokenNode  *inner;
  unsigned long     offset;
};

SyntaxTree *syntax_tree_root(const TokenTree *tree)
{
  SyntaxTree *root = xmalloc(sizeof(SyntaxTree));
  root->parent     = NULL;
  root->inner      = (TokenNode *) tree;
  root->offset     = token_node_trivia_length((const TokenNode *) tree);
  return root;
}

const TokenNode *syntax_tree_raw(const SyntaxTree *tree)
{
  return tree->inner;
}

SyntaxKind syntax_tree_kind(const SyntaxTree *tree)
{
  const TokenTree *inner = (TokenTree *) tree->inner;
  return inner->kind;
}

unsigned long syntax_tree_offset(const SyntaxTree *tree)
{
  return tree->offset;
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
    unsigned long i;
    SyntaxTree   *child = xmalloc(sizeof(SyntaxTree));
    child->parent       = tree;
    child->inner        = inner->children[index];
    child->offset       = tree->offset;
    for (i = 0; i <= index; ++i) {
      child->offset += token_node_trivia_length(inner->children[i]);
      if (i < index) {
        child->offset += token_node_text_length(inner->children[i]);
      }
    }
    return child;
  }
}

void syntax_tree_visit(const SyntaxTree *tree, SyntaxTreeVisitor *visitor, void *data)
{
  if (tree) {
    const TokenTree *inner = (TokenTree *) tree->inner;
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

SyntaxTree *syntax_tree_free(SyntaxTree *tree)
{
  free(tree);
  return NULL;
}
