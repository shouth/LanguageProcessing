#include <assert.h>
#include <stddef.h>
#include <string.h>

#include "array.h"
#include "raw_syntax_tree.h"
#include "utility.h"

/* raw syntax tree */

void raw_syntax_print(const RawSyntaxRoot *root, FILE *file);
void raw_syntax_free(RawSyntaxRoot *root);

/* raw syntax tree builder */

struct RawSyntaxBuilder {
  Array *trivias; /* RawSyntaxTrivia */
  Array *slots; /* RawSyntaxSlot */
};

RawSyntaxBuilder *raw_syntax_builder_new(void)
{
  RawSyntaxBuilder *builder = xmalloc(sizeof(RawSyntaxBuilder));
  builder->trivias          = array_new(sizeof(RawSyntaxToken));
  builder->slots            = array_new(sizeof(RawSyntaxSlot *));
  return builder;
}

void raw_syntax_builder_free(RawSyntaxBuilder *builder)
{
  if (builder) {
    array_free(builder->trivias);
    array_free(builder->slots);
    free(builder);
  }
}

void raw_syntax_builder_null(RawSyntaxBuilder *builder)
{
  RawSyntaxSlot *slot = NULL;
  array_push(builder->slots, &slot);
}

void raw_syntax_builder_trivia(RawSyntaxBuilder *builder, RawSyntaxKind kind, unsigned long text_length)
{
  RawSyntaxToken token;
  token.kind        = kind;
  token.text_length = text_length;
  array_push(builder->trivias, &token);
}

static void raw_syntax_builder_finish_trivia(RawSyntaxBuilder *builder)
{
  unsigned long i;

  RawSyntaxTrivia *trivia = xmalloc(sizeof(RawSyntaxTrivia));
  trivia->count           = array_count(builder->trivias);
  trivia->tokens          = dup(array_data(builder->trivias), sizeof(RawSyntaxToken), trivia->count);
  trivia->text_length     = 0;
  for (i = 0; i < trivia->count; ++i) {
    trivia->text_length += trivia->tokens[i].text_length;
  }

  array_clear(builder->trivias);
  array_push(builder->slots, &trivia);
}

static void raw_syntax_builder_node(RawSyntaxBuilder *builder, RawSyntaxNode *node)
{
  RawSyntaxSlot *slot = (RawSyntaxSlot *) node;
  raw_syntax_builder_finish_trivia(builder);
  array_push(builder->slots, &slot);
}

void raw_syntax_builder_token(RawSyntaxBuilder *builder, RawSyntaxKind kind, unsigned long text_length)
{
  RawSyntaxToken *token = xmalloc(sizeof(RawSyntaxToken));
  token->kind           = kind;
  token->text_length    = text_length;
  raw_syntax_builder_node(builder, (RawSyntaxNode *) token);
}

RawSyntaxCheckpoint raw_syntax_builder_open(RawSyntaxBuilder *builder)
{
  return array_count(builder->slots);
}

void raw_syntax_builder_close(RawSyntaxBuilder *builder, RawSyntaxKind kind, RawSyntaxCheckpoint checkpoint)
{
  unsigned long i;

  unsigned long actual_count = array_count(builder->slots) - checkpoint;

  RawSyntaxTree *tree = xmalloc(sizeof(RawSyntaxTree));
  tree->kind          = kind;

  if (checkpoint % 2) {
    tree->count = actual_count + 1;
    tree->slots = xmalloc(sizeof(RawSyntaxSlot *) * tree->count);

    tree->slots[0]                     = xmalloc(sizeof(RawSyntaxTrivia));
    tree->slots[0]->trivia.count       = 0;
    tree->slots[0]->trivia.text_length = 0;
    tree->slots[0]->trivia.tokens      = NULL;

    memcpy(tree->slots + 1, array_at(builder->slots, checkpoint), sizeof(RawSyntaxSlot *) * (tree->count - 1));
  } else {
    tree->count = actual_count;
    tree->slots = dup(array_at(builder->slots, checkpoint), sizeof(RawSyntaxSlot *), tree->count);
  }

  for (i = 0; i < tree->count; ++i) {
    tree->text_length += tree->slots[i]->header.text_length;
  }

  array_pop_count(builder->slots, actual_count);
  raw_syntax_builder_node(builder, (RawSyntaxNode *) tree);
}

RawSyntaxRoot *raw_syntax_builder_finish(RawSyntaxBuilder *builder)
{
  unsigned long i;

  RawSyntaxRoot *root = xmalloc(sizeof(RawSyntaxRoot));
  root->count         = array_count(builder->slots);

  if (root->count % 2 == 0) {
    raw_syntax_builder_finish_trivia(builder);
  }
  root->slots = dup(array_data(builder->slots), sizeof(RawSyntaxSlot *), root->count);

  root->text_length = 0;
  for (i = 0; i < root->count; ++i) {
    root->text_length += root->slots[i]->header.text_length;
  }

  array_clear(builder->slots);
  return root;
}
