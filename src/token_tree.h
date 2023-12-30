#ifndef LEXICAL_TREE_H
#define LEXICAL_TREE_H

#include "syntax_kind.h"

typedef struct TokenInfo TokenInfo;
typedef struct Token     Token;
typedef struct TokenTree TokenTree;
typedef struct TokenNode TokenNode;

typedef enum {
  TOKEN_OK,
  TOKEN_EOF,
  TOKEN_ERROR_STRAY_CHAR,
  TOKEN_ERROR_NONGRAPHIC_CHAR,
  TOKEN_ERROR_UNTERMINATED_STRING,
  TOKEN_ERROR_UNTERMINATED_COMMENT
} TokenStatus;

struct TokenInfo {
  SyntaxKind    kind;
  unsigned long text_length;
  char         *text;
};

struct Token {
  SyntaxKind    kind;
  unsigned long text_length;
  char         *text;
  unsigned long trivia_count;
  TokenInfo    *trivia;
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

TokenTree *token_tree_new(SyntaxKind kind, const TokenNode **children, unsigned long children_count);
void       token_tree_free(TokenTree *tree);

void token_info_init(TokenInfo *info, SyntaxKind kind, const char *token, unsigned long text_length);
void token_info_deinit(TokenInfo *info);

Token *token_new(const TokenInfo *info, const TokenInfo *trivia, unsigned long trivia_count);
void   token_free(Token *token);

void token_node_print(TokenNode *node);

#endif
