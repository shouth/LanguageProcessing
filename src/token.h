#ifndef LEXICAL_TREE_H
#define LEXICAL_TREE_H

#include "syntax_kind.h"

typedef struct TokenInfo TokenInfo;
typedef struct Token     Token;
typedef struct TokenTree TokenTree;
typedef struct TokenNode TokenNode;

struct TokenInfo {
  SyntaxKind    kind;
  unsigned long text_length;
  char         *text;
};

struct Token {
  SyntaxKind    kind;
  unsigned long text_length;
  char         *text;
  unsigned long leading_trivia_count;
  TokenInfo    *leading_trivia;
  unsigned long trailing_trivia_count;
  TokenInfo    *trailing_trivia;
};

struct TokenTree {
  SyntaxKind    kind;
  unsigned long text_length;
  unsigned long children_count;
  TokenNode   **children;
};

struct TokenNode {
  SyntaxKind    kind;
  unsigned long text_length;
};

void token_tree_init(TokenTree *tree, SyntaxKind kind, const TokenNode **children, unsigned long children_count);
void token_tree_deinit(TokenTree *tree);

void token_info_init(TokenInfo *info, SyntaxKind kind, const char *token, unsigned long text_length);
void token_info_deinit(TokenInfo *info);

void token_init(Token *token, const TokenInfo *info,
  const TokenInfo *leading_trivia, unsigned long leading_trivia_count,
  const TokenInfo *trailing_trivia, unsigned long trailing_trivia_count);
void token_deinit(Token *token);

void token_node_print(TokenNode *node);

#endif
