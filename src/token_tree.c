#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "syntax_kind.h"
#include "token_tree.h"
#include "utility.h"

static unsigned long token_node_text_length(const TokenNode *node)
{
  return node ? node->text_length : 0;
}

static unsigned long token_node_trivia_length(const TokenNode *node)
{
  if (!node) {
    return 0;
  } else if (syntax_kind_is_token(node->kind)) {
    Token        *token = (Token *) node;
    unsigned long i;
    unsigned long result = 0;
    for (i = 0; i < token->trivia_count; ++i) {
      result += token->trivia[i].text_length;
    }
    return result;
  } else {
    TokenTree *tree = (TokenTree *) node;
    return tree->children_count ? token_node_trivia_length(tree->children[0]) : 0;
  }
}

void token_tree_init(TokenTree *tree, SyntaxKind kind, const TokenNode **children, unsigned long children_count)
{
  unsigned long i;
  tree->kind = kind;

  tree->text_length = 0;
  for (i = 0; i < children_count; ++i) {
    if (i > 0) {
      tree->text_length += token_node_trivia_length(children[i]);
    }
    tree->text_length += token_node_text_length(children[i]);
  }

  tree->children_count = children_count;
  if (tree->children_count) {
    tree->children = xmalloc(sizeof(TokenNode *) * children_count);
    memcpy(tree->children, children, sizeof(TokenNode *) * children_count);
  } else {
    tree->children = NULL;
  }
}

void token_tree_deinit(TokenTree *tree)
{
  unsigned long i;
  for (i = 0; i < tree->children_count; ++i) {
    if (tree->children[i]) {
      if (syntax_kind_is_token(tree->children[i]->kind)) {
        token_deinit((Token *) tree->children[i]);
      } else {
        token_tree_deinit((TokenTree *) tree->children[i]);
      }
      free(tree->children[i]);
    }
  }
  free(tree->children);
}

void token_info_init(TokenInfo *info, SyntaxKind kind, const char *token, unsigned long text_length)
{
  info->kind        = kind;
  info->text_length = text_length;
  info->text        = xmalloc(sizeof(char) * (text_length + 1));
  strncpy(info->text, token, text_length);
  info->text[text_length] = '\0';
}

void token_info_deinit(TokenInfo *info)
{
  free(info->text);
}

void token_init(Token *token, const TokenInfo *info, const TokenInfo *trivia, unsigned long trivia_count)
{
  memcpy(token, info, sizeof(TokenInfo));
  if ((token->trivia_count = trivia_count)) {
    token->trivia = xmalloc(sizeof(TokenInfo) * trivia_count);
    memcpy(token->trivia, trivia, sizeof(TokenInfo) * trivia_count);
  } else {
    token->trivia = NULL;
  }
}

void token_deinit(Token *token)
{
  unsigned long i;
  token_info_deinit((TokenInfo *) token);
  for (i = 0; i < token->trivia_count; ++i) {
    token_info_deinit(token->trivia + i);
  }
  free(token->trivia);
}

static void token_node_print_impl(TokenNode *node, unsigned long depth, unsigned long offset)
{
  printf("%*.s", (int) depth * 2, "");
  if (!node) {
    printf("(NULL)\n");
  } else if (syntax_kind_is_token(node->kind)) {
    Token *token = (Token *) node;
    offset += token_node_trivia_length(node);
    printf("%s @ %lu..%lu \"%s\"\n", syntax_kind_to_string(token->kind), offset, offset + token->text_length, token->text);
  } else {
    unsigned long i;
    TokenTree    *tree        = (TokenTree *) node;
    unsigned long tree_offset = offset + token_node_trivia_length(node);
    printf("%s @ %lu..%lu\n", syntax_kind_to_string(tree->kind), tree_offset, tree_offset + tree->text_length);
    for (i = 0; i < tree->children_count; ++i) {
      token_node_print_impl(tree->children[i], depth + 1, offset);
      offset += token_node_trivia_length(tree->children[i]);
      offset += token_node_text_length(tree->children[i]);
    }
  }
}

void token_node_print(TokenNode *node)
{
  token_node_print_impl(node, 0, 0);
}
