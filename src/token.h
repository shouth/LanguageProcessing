#ifndef LEXICAL_TREE_H
#define LEXICAL_TREE_H

#include "syntax_kind.h"

typedef struct TokenInfo     TokenInfo;
typedef struct Token         Token;
typedef struct TokenTree     TokenTree;
typedef struct TokenNodeBase TokenNodeBase;
typedef union TokenNode      TokenNode;

struct TokenInfo {
  SyntaxKind    kind;
  unsigned long text_length;
  char         *text;
};

struct Token {
  SyntaxKind    kind;
  unsigned long text_length;
  char         *text;
  unsigned long trivia_length;
  TokenInfo    *trivia;
};

struct TokenTree {
  SyntaxKind    kind;
  unsigned long text_length;
  unsigned long children_length;
  TokenNode    *children;
};

struct TokenNodeBase {
  SyntaxKind    kind;
  unsigned long text_length;
};

union TokenNode {
  TokenNodeBase common;
  Token         token;
  TokenTree     tree;
};

void token_tree_init(TokenTree *tree, SyntaxKind kind, const TokenNode *children, unsigned long children_length);
void token_tree_deinit(TokenTree *tree);

void token_info_init(TokenInfo *info, SyntaxKind kind, const char *token, unsigned long text_length);
void token_info_deinit(TokenInfo *info);

void token_init(Token *token, const TokenInfo *info, const TokenInfo *trivia, unsigned long trivia_length);
void token_deinit(Token *token);

void token_node_print(TokenNode *node);

#endif
