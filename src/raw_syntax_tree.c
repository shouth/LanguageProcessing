#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "raw_syntax_tree.h"
#include "syntax_kind.h"
#include "utility.h"

unsigned long raw_syntax_node_text_length(const RawSyntaxNode *node)
{
  return node ? node->text_length : 0;
}

unsigned long raw_syntax_node_trivia_length(const RawSyntaxNode *node)
{
  if (!node) {
    return 0;
  } else if (syntax_kind_is_token(node->kind)) {
    RawSyntaxToken *token = (RawSyntaxToken *) node;
    unsigned long   i;
    unsigned long   result = 0;
    for (i = 0; i < token->trivia_count; ++i) {
      result += token->trivia[i].text_length;
    }
    return result;
  } else {
    RawSyntaxTree *tree = (RawSyntaxTree *) node;
    return tree->children_count ? raw_syntax_node_trivia_length(tree->children[0]) : 0;
  }
}

RawSyntaxTree *raw_syntax_tree_new(SyntaxKind kind, const RawSyntaxNode **children, unsigned long children_count)
{
  RawSyntaxTree *tree = xmalloc(sizeof(RawSyntaxTree));
  unsigned long  i;
  tree->kind = kind;

  tree->text_length = 0;
  for (i = 0; i < children_count; ++i) {
    if (i > 0) {
      tree->text_length += raw_syntax_node_trivia_length(children[i]);
    }
    tree->text_length += raw_syntax_node_text_length(children[i]);
  }

  tree->children_count = children_count;
  if (tree->children_count) {
    tree->children = xmalloc(sizeof(RawSyntaxNode *) * children_count);
    memcpy(tree->children, children, sizeof(RawSyntaxNode *) * children_count);
  } else {
    tree->children = NULL;
  }
  return tree;
}

void raw_syntax_tree_free(RawSyntaxTree *tree)
{
  if (tree) {
    unsigned long i;
    for (i = 0; i < tree->children_count; ++i) {
      if (tree->children[i]) {
        if (syntax_kind_is_token(tree->children[i]->kind)) {
          raw_syntax_token_free((RawSyntaxToken *) tree->children[i]);
        } else {
          raw_syntax_tree_free((RawSyntaxTree *) tree->children[i]);
        }
      }
    }
    free(tree->children);
    free(tree);
  }
}

void token_info_init(RawSyntaxTrivial *info, SyntaxKind kind, const char *token, unsigned long text_length)
{
  info->kind        = kind;
  info->text_length = text_length;
  info->text        = xmalloc(sizeof(char) * (text_length + 1));
  strncpy(info->text, token, text_length);
  info->text[text_length] = '\0';
}

void token_info_deinit(RawSyntaxTrivial *info)
{
  free(info->text);
}

RawSyntaxToken *raw_syntax_token_new(SyntaxKind kind, char *text, unsigned long text_length, RawSyntaxTrivial *trivia, unsigned long trivia_count)
{
  RawSyntaxToken *token = xmalloc(sizeof(RawSyntaxToken));
  token->kind           = kind;
  token->text           = text;
  token->text_length    = text_length;
  token->trivia         = trivia;
  token->trivia_count   = trivia_count;
  return token;
}

void raw_syntax_token_free(RawSyntaxToken *token)
{
  if (token) {
    unsigned long i;
    token_info_deinit((RawSyntaxTrivial *) token);
    for (i = 0; i < token->trivia_count; ++i) {
      token_info_deinit(token->trivia + i);
    }
    free(token->trivia);
    free(token);
  }
}

static void token_node_print_impl(RawSyntaxNode *node, unsigned long depth, unsigned long offset)
{
  printf("%*.s", (int) depth * 2, "");
  if (!node) {
    printf("(NULL)\n");
  } else if (syntax_kind_is_token(node->kind)) {
    RawSyntaxToken *token = (RawSyntaxToken *) node;
    offset += raw_syntax_node_trivia_length(node);
    printf("%s @ %lu..%lu \"%s\"\n", syntax_kind_to_string(token->kind), offset, offset + token->text_length, token->text);
  } else {
    unsigned long  i;
    RawSyntaxTree *tree        = (RawSyntaxTree *) node;
    unsigned long  tree_offset = offset + raw_syntax_node_trivia_length(node);
    printf("%s @ %lu..%lu\n", syntax_kind_to_string(tree->kind), tree_offset, tree_offset + tree->text_length);
    for (i = 0; i < tree->children_count; ++i) {
      token_node_print_impl(tree->children[i], depth + 1, offset);
      offset += raw_syntax_node_trivia_length(tree->children[i]);
      offset += raw_syntax_node_text_length(tree->children[i]);
    }
  }
}

void raw_syntax_node_print(RawSyntaxNode *node)
{
  token_node_print_impl(node, 0, 0);
}

void raw_syntax_node_free(RawSyntaxNode *node)
{
  if (node) {
    if (syntax_kind_is_token(node->kind)) {
      raw_syntax_token_free((RawSyntaxToken *) node);
    } else {
      raw_syntax_tree_free((RawSyntaxTree *) node);
    }
  }
}
