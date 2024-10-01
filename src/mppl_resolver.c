/* SPDX-License-Identifier: Apache-2.0 */

#include <assert.h>

#include "diag.h"
#include "mppl_passes.h"
#include "mppl_semantic.h"
#include "mppl_syntax.h"
#include "stdio.h"
#include "syntax_tree.h"
#include "util.h"

typedef struct Binding  Binding;
typedef struct Scope    Scope;
typedef struct Resolver Resolver;

struct Binding {
  const RawSyntaxToken *name;
  unsigned long         depth;
  unsigned long         declared_at;
};

struct Scope {
  Scope        *parent;
  unsigned long depth;
  Vec(Binding) bindings;
  Vec(Binding) shadowed;
};

struct Resolver {
  Scope *scope;
  HashMap(const RawSyntaxToken *, Binding) bindings;
  Vec(MpplSemanticEvent) events;
  Vec(Report *) diags;
};

static Hash name_hash(const void *key)
{
  const RawSyntaxToken *name = *(RawSyntaxToken *const *) key;
  return hash_fnv1a(NULL, name->text, name->node.span.text_length);
}

static int name_eq(const void *a, const void *b)
{
  const RawSyntaxToken *l = *(RawSyntaxToken *const *) a;
  const RawSyntaxToken *r = *(RawSyntaxToken *const *) b;
  return l->node.span.text_length == r->node.span.text_length && memcmp(l->text, r->text, l->node.span.text_length) == 0;
}

static void diag(Resolver *resolver, Report *report)
{
  vec_push(&resolver->diags, &report, 1);
}

static void enter_binding_use(Resolver *resolver, const SyntaxToken *token)
{
  MpplSemanticEvent event;
  HashMapEntry      entry;

  assert(token->raw->node.kind == MPPL_SYNTAX_IDENT_TOKEN);

  if (hashmap_entry(&resolver->bindings, &token->raw, &entry)) {
    event.kind        = MPPL_SEMANTIC_USE;
    event.declared_at = hashmap_value(&resolver->bindings, &entry)->declared_at;
    event.used_at     = token->node.span.offset;
  } else {
    event.kind    = MPPL_SEMANTIC_NOT_FOUND;
    event.used_at = token->node.span.offset;
    diag(resolver, diag_not_found_error(token->node.span.offset, token->raw->node.span.text_length, token->raw->text));
  }
  vec_push(&resolver->events, &event, 1);
}

static void enter_binding_decl(Resolver *resolver, const SyntaxToken *token, MpplSyntaxKind kind)
{
  assert(token->raw->node.kind == MPPL_SYNTAX_IDENT_TOKEN);

  if (kind != MPPL_SYNTAX_PROGRAM) {
    HashMapEntry entry;
    Binding      binding;
    binding.name        = token->raw;
    binding.depth       = resolver->scope->depth;
    binding.declared_at = token->node.span.offset;

    if (hashmap_entry(&resolver->bindings, &binding.name, &entry)) {
      Binding *shadowed = hashmap_value(&resolver->bindings, &entry);
      if (shadowed->depth == binding.depth) {
        diag(resolver,
          diag_multiple_definition_error(token->node.span.offset, token->raw->node.span.text_length, token->raw->text, shadowed->declared_at));
      } else {
        vec_push(&resolver->scope->shadowed, shadowed, 1);
        vec_push(&resolver->scope->bindings, &binding, 1);
        hashmap_occupy(&resolver->bindings, &entry, &binding.name);
        *hashmap_value(&resolver->bindings, &entry) = binding;
      }
    } else {
      vec_push(&resolver->scope->bindings, &binding, 1);
      hashmap_occupy(&resolver->bindings, &entry, &binding.name);
      *hashmap_value(&resolver->bindings, &entry) = binding;
    }
  }

  {
    MpplSemanticEvent event;
    event.kind        = MPPL_SEMANTIC_DEFINE;
    event.declared_at = token->node.span.offset;
    vec_push(&resolver->events, &event, 1);
  }
}

static void enter_ident(Resolver *resolver, const SyntaxToken *token)
{
  const SyntaxTree *parent;

  assert(token->raw->node.kind == MPPL_SYNTAX_IDENT_TOKEN);
  for (parent = token->node.parent; parent; parent = parent->node.parent) {
    switch (parent->raw->node.kind) {
    case MPPL_SYNTAX_ENTIRE_VAR:
    case MPPL_SYNTAX_INDEXED_VAR:
    case MPPL_SYNTAX_CALL_STMT:
      enter_binding_use(resolver, token);
      return;

    case MPPL_SYNTAX_PROGRAM:
    case MPPL_SYNTAX_VAR_DECL:
    case MPPL_SYNTAX_PROC_DECL:
    case MPPL_SYNTAX_FML_PARAM_SEC:
      enter_binding_decl(resolver, token, parent->raw->node.kind);
      return;

    default:
      /* do nothing */
      break;
    }
  }
  unreachable();
}

static void push_scope(Resolver *resolver)
{
  Scope *scope  = malloc(sizeof(Scope));
  scope->parent = resolver->scope;
  scope->depth  = scope->parent ? scope->parent->depth + 1 : 0;
  vec_alloc(&scope->bindings, 0);
  vec_alloc(&scope->shadowed, 0);
  resolver->scope = scope;
}

static void pop_scope(Resolver *resolver)
{
  unsigned long i;
  HashMapEntry  entry;

  Scope *scope = resolver->scope;
  for (i = 0; i < scope->bindings.count; i++) {
    Binding *binding = &scope->bindings.ptr[i];
    hashmap_entry(&resolver->bindings, &binding->name, &entry);
    hashmap_release(&resolver->bindings, &entry);
  }

  for (i = 0; i < scope->shadowed.count; i++) {
    Binding *binding = &scope->shadowed.ptr[i];
    hashmap_entry(&resolver->bindings, &binding->name, &entry);
    hashmap_occupy(&resolver->bindings, &entry, &binding->name);
    *hashmap_value(&resolver->bindings, &entry) = *binding;
  }

  vec_free(&scope->bindings);
  vec_free(&scope->shadowed);
  resolver->scope = scope->parent;
  free(scope);
}

static void do_resolve(Resolver *resolver, const SyntaxTree *syntax)
{
  SyntaxTree   *tree;
  SyntaxToken  *token;
  unsigned long i;

  for (i = 0; i * 2 < syntax->raw->children.count; ++i) {
    if ((tree = syntax_tree_child_tree(syntax, i))) {
      if (tree->raw->node.kind == MPPL_SYNTAX_PROGRAM || tree->raw->node.kind == MPPL_SYNTAX_PROC_BODY) {
        push_scope(resolver);
        do_resolve(resolver, tree);
        pop_scope(resolver);
      } else {
        do_resolve(resolver, tree);
      }
      syntax_tree_free(tree);
    } else if ((token = syntax_tree_child_token(syntax, i))) {
      if (token->raw->node.kind == MPPL_SYNTAX_IDENT_TOKEN) {
        enter_ident(resolver, token);
      }
      syntax_token_free(token);
    }
  }
}

MpplResolveResult mppl_resolve(const SyntaxTree *tree)
{
  MpplResolveResult result;

  Resolver resolver;
  resolver.scope = NULL;
  hashmap_alloc(&resolver.bindings, &name_hash, &name_eq);
  vec_alloc(&resolver.events, 0);
  vec_alloc(&resolver.diags, 0);

  do_resolve(&resolver, tree);

  result.semantics   = mppl_semantics_alloc(tree, resolver.events.ptr, resolver.events.count);
  result.diags.ptr   = resolver.diags.ptr;
  result.diags.count = resolver.diags.count;

  hashmap_free(&resolver.bindings);
  vec_free(&resolver.events);

  return result;
}
