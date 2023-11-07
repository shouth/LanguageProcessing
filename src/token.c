#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "map.h"
#include "syntax_kind.h"
#include "token.h"
#include "utility.h"
#include "vector.h"

static unsigned long token_hash(unsigned long hash, const Token *token)
{
  return fnv1a(hash, token->string, token->size);
}

static unsigned long token_tree_hash(unsigned long hash, const TokenTree *tree)
{
  return fnv1a(hash, vector_data(&tree->children), sizeof(void *) * vector_count(&tree->children));
}

static unsigned long token_node_hash(unsigned long hash, const TokenNode *node)
{
  hash = fnv1a(hash, &node->kind, sizeof(SyntaxKind));
  if (syntax_kind_is_token(node->kind)) {
    hash = token_hash(hash, (const Token *) node);
  } else {
    hash = token_tree_hash(hash, (const TokenTree *) node);
  }
  return hash;
}

static unsigned long token_node_hasher(const void *value)
{
  return token_node_hash(FNV1A_INIT, value);
}

static int token_node_compare(const TokenNode *left, const TokenNode *right)
{
  if (left->kind == right->kind) {
    if (syntax_kind_is_token(left->kind)) {
      const Token *l = (Token *) left;
      const Token *r = (Token *) right;
      return !strcmp(l->string, r->string);
    } else {
      const TokenTree *l = (TokenTree *) left;
      const TokenTree *r = (TokenTree *) right;
      if (vector_count(&l->children) == vector_count(&r->children)) {
        return !memcmp(vector_data(&l->children), vector_data(&r->children), sizeof(void *) * vector_count(&l->children));
      } else {
        return 0;
      }
    }
  } else {
    return 0;
  }
}

static int token_node_comparator(const void *left, const void *right)
{
  return token_node_compare(left, right);
}

static void token_tree_init(TokenTree *tree, SyntaxKind kind)
{
  tree->node.kind = kind;
  vector_init(&tree->children, sizeof(TokenNode *));
}

static void token_init(Token *token, SyntaxKind kind, const char *string, unsigned long size)
{
  token->node.kind = kind;
  token->size      = size;
  token->string    = xmalloc(sizeof(char) * (size + 1));
  strncpy(token->string, string, size);
  token->string[size] = '\0';
}

static void token_node_deinit(TokenNode *node)
{
  if (syntax_kind_is_token(node->kind)) {
    Token *token = (Token *) node;
    free(token->string);
  } else {
    TokenTree *tree = (TokenTree *) node;
    vector_deinit(&tree->children);
  }
}

void token_context_init(TokenContext *context)
{
  map_init(&context->_cache, &token_node_hasher, &token_node_comparator);
}

static const TokenNode *token_context_intern(TokenContext *context, TokenNode *node)
{
  MapEntry entry;
  if (map_entry(&context->_cache, node, &entry)) {
    token_node_deinit(node);
    free(node);
  } else {
    map_entry_update(&entry, NULL);
  }
  return map_entry_key(&entry);
}

const Token *token_context_token(TokenContext *context, SyntaxKind kind, const char *string, unsigned long size)
{
  Token *token = xmalloc(sizeof(Token));
  token_init(token, kind, string, size);
  return (Token *) token_context_intern(context, (TokenNode *) token);
}

void token_context_deinit(TokenContext *context)
{
  MapIterator iterator;
  map_iterator(&iterator, &context->_cache);
  while (map_iterator_next(&iterator)) {
    token_node_deinit(map_iterator_key(&iterator));
    free(map_iterator_key(&iterator));
  }
  map_deinit(&context->_cache);
}
