#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "syntax_kind.h"
#include "token.h"
#include "utility.h"

static unsigned long trivia_length(TokenInfo *trivia, unsigned long count)
{
  unsigned long i;
  unsigned long result = 0;
  for (i = 0; i < count; ++i) {
    result += trivia[i].text_length;
  }
  return result;
}

static unsigned long token_node_leading_trivia_length(const TokenNode *node)
{
  if (syntax_kind_is_token(node->kind)) {
    Token *token = (Token *) node;
    return trivia_length(token->leading_trivia, token->leading_trivia_count);
  } else {
    TokenTree *tree = (TokenTree *) node;
    return tree->children_count ? token_node_leading_trivia_length(tree->children[0]) : 0;
  }
}

static unsigned long token_node_trailing_trivia_length(const TokenNode *node)
{
  if (syntax_kind_is_token(node->kind)) {
    Token *token = (Token *) node;
    return trivia_length(token->trailing_trivia, token->trailing_trivia_count);
  } else {
    TokenTree *tree = (TokenTree *) node;
    return tree->children_count ? token_node_trailing_trivia_length(tree->children[tree->children_count - 1]) : 0;
  }
}

void token_tree_init(TokenTree *tree, SyntaxKind kind, const TokenNode **children, unsigned long children_count)
{
  unsigned long i;
  tree->kind = kind;

  tree->text_length = 0;
  for (i = 0; i < children_count; ++i) {
    if (children[i]) {
      if (i > 0) {
        tree->text_length += token_node_leading_trivia_length(children[i]);
      }
      tree->text_length += children[i]->text_length;
      if (i + 1 < children_count) {
        tree->text_length += token_node_trailing_trivia_length(children[i]);
      }
    }
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
    if (syntax_kind_is_token(tree->children[i]->kind)) {
      token_deinit((Token *) tree->children[i]);
    } else {
      token_tree_deinit((TokenTree *) tree->children[i]);
    }
    free(tree->children[i]);
  }
  free(tree->children);
}

void token_info_init(TokenInfo *info, SyntaxKind kind, const char *token, unsigned long text_length)
{
  info->kind        = kind;
  info->text_length = text_length;
  if (info->text_length) {
    info->text = xmalloc(sizeof(char) * (text_length + 1));
    strncpy(info->text, token, text_length);
    info->text[text_length] = '\0';
  } else {
    info->text = NULL;
  }
}

void token_info_deinit(TokenInfo *info)
{
  free(info->text);
}

void token_init(Token *token, const TokenInfo *info,
  const TokenInfo *leading_trivia, unsigned long leading_trivia_count,
  const TokenInfo *trailing_trivia, unsigned long trailing_trivia_count)
{
  memcpy(token, info, sizeof(TokenInfo));
  if ((token->leading_trivia_count = leading_trivia_count)) {
    token->leading_trivia = xmalloc(sizeof(TokenInfo) * leading_trivia_count);
    memcpy(token->leading_trivia, leading_trivia, sizeof(TokenInfo) * leading_trivia_count);
  } else {
    token->leading_trivia = NULL;
  }

  if ((token->trailing_trivia_count = trailing_trivia_count)) {
    token->trailing_trivia = xmalloc(sizeof(TokenInfo) * trailing_trivia_count);
    memcpy(token->trailing_trivia, trailing_trivia, sizeof(TokenInfo) * trailing_trivia_count);
  } else {
    token->trailing_trivia = NULL;
  }
}

void token_deinit(Token *token)
{
  unsigned long i;
  token_info_deinit((TokenInfo *) token);
  for (i = 0; i < token->leading_trivia_count; ++i) {
    token_info_deinit(token->leading_trivia + i);
  }
  free(token->leading_trivia);
  for (i = 0; i < token->trailing_trivia_count; ++i) {
    token_info_deinit(token->trailing_trivia + i);
  }
  free(token->trailing_trivia);
}

static void token_node_print_impl(TokenNode *node, unsigned long depth, unsigned long offset)
{
  printf("%*.s", (int) depth * 2, "");
  if (node) {
    printf("(NULL)\n");
  } else if (syntax_kind_is_token(node->kind)) {
    Token *token = (Token *) node;
    printf("%s @ %lu..%lu \"%s\"\n", syntax_kind_to_string(token->kind), offset, offset + token->text_length, token->text);
  } else {
    unsigned long i;
    TokenTree    *tree = (TokenTree *) node;
    printf("%s @ %lu..%lu\n", syntax_kind_to_string(tree->kind), offset, offset + tree->text_length);
    for (i = 0; i < tree->children_count; ++i) {
      offset += token_node_leading_trivia_length(tree->children[i]);
      token_node_print_impl(tree->children[i], depth + 1, offset);
      offset += tree->children[i]->text_length;
      offset += token_node_trailing_trivia_length(tree->children[i]);
    }
  }
}

void token_node_print(TokenNode *node)
{
  token_node_print_impl(node, 0, 0);
}
