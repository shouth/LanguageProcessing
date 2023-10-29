#ifndef LEXICAL_TREE_H
#define LEXICAL_TREE_H

#include "map.h"
#include "syntax_kind.h"
#include "vector.h"

typedef struct TokenContext     TokenContext;
typedef struct TokenNode        TokenNode;
typedef struct TokenTreeBuilder TokenTreeBuilder;
typedef struct TokenTree        TokenTree;
typedef struct Token            Token;

struct TokenContext {
  Map _cache;
};

struct TokenNode {
  SyntaxKind kind;
};

struct TokenTreeBuilder {
  Vector _parents;
  Vector _children;
  Vector _errors;
};

struct TokenTree {
  TokenNode node;
  Vector    children;
};

struct Token {
  TokenNode     node;
  char         *string;
  unsigned long size;
};

void             token_context_init(TokenContext *context);
const Token     *token_context_token(TokenContext *context, SyntaxKind kind, const char *string, unsigned long size);
void             token_context_deinit(TokenContext *context);
void             token_context_tree_builder(TokenContext *context, TokenTreeBuilder *builder);
const TokenTree *token_context_tree(TokenContext *context, TokenTreeBuilder *builder);

void          token_tree_builder_token(TokenTreeBuilder *builder, const Token *token);
unsigned long token_tree_builder_checkpoint(TokenTreeBuilder *builder);
void          token_tree_builder_node_start(TokenTreeBuilder *builder, SyntaxKind kind);
void          token_tree_builder_node_start_at(TokenTreeBuilder *builder, SyntaxKind kind, unsigned long checkpoint);
void          token_tree_builder_node_end(TokenTreeBuilder *builder);

#endif
