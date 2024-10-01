/* SPDX-License-Identifier: Apache-2.0 */

#include <assert.h>
#include <stddef.h>

#include "mppl_semantic.h"
#include "mppl_syntax.h"
#include "syntax_tree.h"
#include "util.h"

typedef struct MpplSemanticsBuilder MpplSemanticsBuilder;

struct MpplSemanticsBuilder {
  Vec(unsigned long) bindings;
  HashMap(unsigned long, SyntaxToken *) syntax;
  HashMap(unsigned long, Vec(unsigned long)) usage;
  Vec(unsigned long) unresolved;
};

static unsigned long offset_hash(const void *value)
{
  return hash_fnv1a(NULL, value, sizeof(unsigned long));
}

static int offset_equal(const void *a, const void *b)
{
  return *(unsigned long *) a == *(unsigned long *) b;
}

static void handle_event(MpplSemanticsBuilder *builder, const MpplSemanticEvent *events, unsigned long event_count)
{
  unsigned long i;

  for (i = 0; i < event_count; ++i) {
    const MpplSemanticEvent *event = &events[i];
    switch (event->kind) {
    case MPPL_SEMANTIC_DEFINE:
      vec_push(&builder->bindings, &event->declared_at, 1);
      break;

    case MPPL_SEMANTIC_USE: {
      HashMapEntry entry;
      if (!hashmap_entry(&builder->usage, &event->declared_at, &entry)) {
        hashmap_occupy(&builder->usage, &entry, &event->declared_at);
        vec_alloc(hashmap_value(&builder->usage, &entry), 0);
      }
      vec_push(hashmap_value(&builder->usage, &entry), &event->used_at, 1);
      break;
    }

    case MPPL_SEMANTIC_NOT_FOUND:
      vec_push(&builder->unresolved, &event->used_at, 1);
      break;
    }
  }
}

static void handle_binding(MpplSemanticsBuilder *builder, const SyntaxToken *token)
{
  SyntaxTree  *parent;
  HashMapEntry entry;

  assert(token->raw->node.kind == MPPL_SYNTAX_IDENT_TOKEN);
  for (parent = token->node.parent; parent; parent = parent->node.parent) {
    switch (parent->raw->node.kind) {
    case MPPL_SYNTAX_PROGRAM:
    case MPPL_SYNTAX_VAR_DECL:
    case MPPL_SYNTAX_PROC_DECL:
    case MPPL_SYNTAX_FML_PARAM_SEC:
      hashmap_entry(&builder->syntax, &token->node.span.offset, &entry);
      hashmap_occupy(&builder->syntax, &entry, &token->node.span.offset);
      *hashmap_value(&builder->syntax, &entry) = syntax_token_shared(token);
      return;

    case MPPL_SYNTAX_ENTIRE_VAR:
    case MPPL_SYNTAX_INDEXED_VAR:
    case MPPL_SYNTAX_CALL_STMT:
      /* do nothing */
      return;

    default:
      /* do nothing */
      break;
    }
  }
}

static void handle_syntax(MpplSemanticsBuilder *builder, const SyntaxTree *syntax)
{
  unsigned long i;

  for (i = 0; i * 2 < syntax->raw->children.count; ++i) {
    SyntaxTree  *tree;
    SyntaxToken *token;
    if ((tree = syntax_tree_child_tree(syntax, i))) {
      handle_syntax(builder, tree);
      syntax_tree_free(tree);
    } else if ((token = syntax_tree_child_token(syntax, i))) {
      if (token->raw->node.kind == MPPL_SYNTAX_IDENT_TOKEN) {
        handle_binding(builder, token);
      }
      syntax_token_free(token);
    }
  }
}

static MpplSemantics build(MpplSemanticsBuilder *builder)
{
  unsigned long i;
  HashMapEntry  entry;

  MpplSemantics semantics;

  slice_alloc(&semantics.bindings, builder->bindings.count);
  hashmap_alloc(&semantics.ref, &offset_hash, &offset_equal);

  for (i = 0; i < builder->bindings.count; ++i) {
    MpplBinding  *binding = &semantics.bindings.ptr[i];
    unsigned long offset  = builder->bindings.ptr[i];

    hashmap_entry(&builder->syntax, &offset, &entry);
    binding->binding = *hashmap_value(&builder->syntax, &entry);

    if (hashmap_entry(&builder->usage, &offset, &entry)) {
      binding->refs.ptr   = hashmap_value(&builder->usage, &entry)->ptr;
      binding->refs.count = hashmap_value(&builder->usage, &entry)->count;
    } else {
      slice_alloc(&binding->refs, 0);
    }

    hashmap_entry(&semantics.ref, &offset, &entry);
    hashmap_occupy(&semantics.ref, &entry, &offset);
    *hashmap_value(&semantics.ref, &entry) = binding;
  }

  semantics.unresolved.ptr   = builder->unresolved.ptr;
  semantics.unresolved.count = builder->unresolved.count;

  vec_free(&builder->bindings);
  hashmap_free(&builder->syntax);
  hashmap_free(&builder->usage);

  return semantics;
}

MpplSemantics mppl_semantics_alloc(const SyntaxTree *syntax, const MpplSemanticEvent *events, unsigned long event_count)
{
  MpplSemanticsBuilder builder;
  vec_alloc(&builder.bindings, 0);
  hashmap_alloc(&builder.syntax, &offset_hash, &offset_equal);
  hashmap_alloc(&builder.usage, &offset_hash, &offset_equal);
  vec_alloc(&builder.unresolved, 0);

  handle_event(&builder, events, event_count);
  handle_syntax(&builder, syntax);

  return build(&builder);
}

void mppl_semantics_free(MpplSemantics *semantics)
{
  if (semantics) {
    unsigned long i;
    for (i = 0; i < semantics->bindings.count; ++i) {
      MpplBinding *binding = &semantics->bindings.ptr[i];
      syntax_token_free(binding->binding);
      slice_free(&binding->refs);
    }
    slice_free(&semantics->bindings);
    hashmap_free(&semantics->ref);
    slice_free(&semantics->unresolved);
  }
}

void mppl_semantics_print(const MpplSemantics *semantics, const Source *source)
{
  unsigned long i, j;

  for (i = 0; i < semantics->bindings.count; ++i) {
    SyntaxTree    *parent;
    SourceLocation location;

    MpplBinding *binding = &semantics->bindings.ptr[i];

    if (i > 0) {
      printf("\n");
    }

    printf("name: %.*s\n", (int) binding->binding->raw->node.span.text_length, binding->binding->raw->text);

    printf("type: ");
    for (parent = binding->binding->node.parent; parent; parent = parent->node.parent) {
      switch (parent->raw->node.kind) {
      case MPPL_SYNTAX_PROGRAM:
        printf("program");
        break;

      case MPPL_SYNTAX_VAR_DECL:
        printf("variable");
        break;

      case MPPL_SYNTAX_PROC_DECL:
        printf("procedure");
        break;

      case MPPL_SYNTAX_FML_PARAM_SEC:
        printf("formal parameter");
        break;

      case MPPL_SYNTAX_ENTIRE_VAR:
      case MPPL_SYNTAX_INDEXED_VAR:
      case MPPL_SYNTAX_CALL_STMT:
        /* do nothing */
        break;

      default:
        continue;
      }
      break;
    }
    printf("\n");

    source_location(source, binding->binding->node.span.offset, &location);
    printf("definition: %lu:%lu\n", location.line + 1, location.column + 1);

    printf("reference: ");
    for (j = 0; j < binding->refs.count; ++j) {
      source_location(source, binding->refs.ptr[j], &location);
      printf("%lu:%lu ", location.line, location.column);
    }
    printf("\n");
  }
}
