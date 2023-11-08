#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "syntax_kind.h"
#include "token.h"
#include "utility.h"

void token_tree_init(TokenTree *tree, SyntaxKind kind, const TokenNode *children, unsigned long length)
{
  tree->kind   = kind;
  tree->length = length;
  if (tree->length) {
    tree->children = xmalloc(sizeof(TokenNode *) * length);
    memcpy(tree->children, children, sizeof(TokenNode *) * length);
  } else {
    tree->children = NULL;
  }
}

void token_tree_deinit(TokenTree *tree)
{
  unsigned long i;
  for (i = 0; i < tree->length; ++i) {
    if (syntax_kind_is_token(tree->children[i]->kind)) {
      token_deinit((Token *) tree->children[i]);
    } else {
      token_tree_deinit((TokenTree *) tree->children[i]);
    }
  }
  free(tree->children);
}

void token_info_init(TokenInfo *info, SyntaxKind kind, const char *token, unsigned long length)
{
  info->kind   = kind;
  info->length = length;
  if (info->length) {
    info->token = xmalloc(sizeof(char) * (length + 1));
    strncpy(info->token, token, length);
    info->token[length] = '\0';
  } else {
    info->token = NULL;
  }
}

void token_info_deinit(TokenInfo *info)
{
  free(info->token);
}

void token_init(Token *token, const TokenInfo *info, const TokenInfo *trivia, unsigned long length)
{
  token->info   = *info;
  token->length = length;
  if (token->length) {
    token->trivia = xmalloc(sizeof(TokenInfo) * length);
    memcpy(token->trivia, trivia, sizeof(TokenInfo) * length);
  } else {
    token->trivia = NULL;
  }
}

void token_deinit(Token *token)
{
  unsigned long i;
  token_info_deinit(&token->info);
  for (i = 0; i < token->length; ++i) {
    token_info_deinit(token->trivia + i);
  }
  free(token->trivia);
}
