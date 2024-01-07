#ifndef LEXICAL_TREE_H
#define LEXICAL_TREE_H

#include "syntax_kind.h"

typedef struct TrivialToken TrivialToken;
typedef struct Token        Token;
typedef struct TokenTree    TokenTree;
typedef struct TokenNode    TokenNode;

struct TrivialToken {
  SyntaxKind    kind;
  unsigned long text_length;
  char         *text;
};

struct Token {
  SyntaxKind    kind;
  unsigned long text_length;
  char         *text;
  unsigned long trivia_count;
  TrivialToken *trivia;
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

unsigned long token_node_text_length(const TokenNode *node);
unsigned long token_node_trivia_length(const TokenNode *node);

TokenTree *token_tree_new(SyntaxKind kind, const TokenNode **children, unsigned long children_count);
void       token_tree_free(TokenTree *tree);

Token *token_new(SyntaxKind kind, char *text, unsigned long text_length, TrivialToken *trivia, unsigned long trivia_count);
void   token_free(Token *token);

void token_node_print(TokenNode *node);

#endif
