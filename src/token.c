#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "syntax_kind.h"
#include "token.h"
#include "utility.h"

void token_tree_init(TokenTree *tree, SyntaxKind kind, const TokenNode *children, unsigned long children_length)
{
  unsigned long i;
  tree->kind        = kind;
  tree->text_length = 0;
  for (i = 0; i < children_length; ++i) {
    tree->text_length += children->common.text_length;
  }
  tree->children_length = children_length;
  if (tree->children_length) {
    tree->children = xmalloc(sizeof(TokenNode) * children_length);
    memcpy(tree->children, children, sizeof(TokenNode) * children_length);
  } else {
    tree->children = NULL;
  }
}

void token_tree_deinit(TokenTree *tree)
{
  unsigned long i;
  for (i = 0; i < tree->children_length; ++i) {
    if (syntax_kind_is_token(tree->children[i].common.kind)) {
      token_deinit((Token *) &tree->children[i].token);
    } else {
      token_tree_deinit((TokenTree *) &tree->children[i].tree);
    }
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

void token_init(Token *token, const TokenInfo *info, const TokenInfo *trivia, unsigned long trivia_length)
{
  memcpy(token, info, sizeof(TokenInfo));
  token->trivia_length = trivia_length;
  if (token->trivia_length) {
    token->trivia = xmalloc(sizeof(TokenInfo) * trivia_length);
    memcpy(token->trivia, trivia, sizeof(TokenInfo) * trivia_length);
  } else {
    token->trivia = NULL;
  }
}

void token_deinit(Token *token)
{
  unsigned long i;
  token_info_deinit((TokenInfo *) token);
  for (i = 0; i < token->trivia_length; ++i) {
    token_info_deinit(token->trivia + i);
  }
  free(token->trivia);
}
