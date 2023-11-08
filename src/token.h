#ifndef LEXICAL_TREE_H
#define LEXICAL_TREE_H

#include "syntax_kind.h"

typedef struct TokenNode TokenNode;
typedef struct TokenTree TokenTree;
typedef struct TokenInfo TokenInfo;
typedef struct Token     Token;

struct TokenNode {
  SyntaxKind kind;
};

struct TokenTree {
  SyntaxKind    kind;
  unsigned long length;
  TokenNode   **children;
};

struct TokenInfo {
  SyntaxKind    kind;
  unsigned long length;
  char         *token;
};

struct Token {
  TokenInfo     info;
  unsigned long length;
  TokenInfo    *trivia;
};

void token_tree_init(TokenTree *tree, SyntaxKind kind, const TokenNode *children, unsigned long length);
void token_tree_deinit(TokenTree *tree);

void token_info_init(TokenInfo *info, SyntaxKind kind, const char *token, unsigned long length);
void token_info_deinit(TokenInfo *info);

void token_init(Token *token, const TokenInfo *info, const TokenInfo *trivia, unsigned long length);
void token_deinit(Token *token);

#endif
