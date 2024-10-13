/* SPDX-License-Identifier: Apache-2.0 */

#include <assert.h>

#include "diag.h"
#include "mppl_passes.h"
#include "mppl_semantic.h"
#include "mppl_syntax.h"
#include "report.h"
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

static void enter_ref_ident(Resolver *resolver, const SyntaxToken *token)
{
  MpplSemanticEvent event;
  HashMapEntry      entry;

  if (hashmap_entry(&resolver->bindings, &token->raw, &entry)) {
    event.kind        = MPPL_SEMANTIC_USE;
    event.declared_at = hashmap_value(&resolver->bindings, &entry)->declared_at;
    event.used_at     = token->node.span.offset;
  } else {
    Report *report = diag_not_defined_error(token->node.span.offset, token->raw->node.span.text_length, token->raw->text);
    vec_push(&resolver->diags, &report, 1);

    event.kind    = MPPL_SEMANTIC_NOT_DEFINED;
    event.used_at = token->node.span.offset;
  }
  vec_push(&resolver->events, &event, 1);
}

static void enter_bind_ident(Resolver *resolver, const SyntaxToken *token)
{
  MpplSyntaxKind    item_kind;
  const SyntaxTree *syntax = token->node.parent;

  while (1) {
    switch (syntax->raw->node.kind) {
    case MPPL_SYNTAX_PROGRAM:
    case MPPL_SYNTAX_PROC_DECL:
    case MPPL_SYNTAX_VAR_DECL:
    case MPPL_SYNTAX_FML_PARAM_SEC:
      item_kind = syntax->raw->node.kind;
      break;

    default:
      syntax = syntax->node.parent;
      continue;
    }
    break;
  }

  if (item_kind != MPPL_SYNTAX_PROGRAM) {
    HashMapEntry entry;
    Binding      binding;
    binding.name        = token->raw;
    binding.depth       = resolver->scope->depth;
    binding.declared_at = token->node.span.offset;

    if (hashmap_entry(&resolver->bindings, &binding.name, &entry)) {
      Binding *shadowed = hashmap_value(&resolver->bindings, &entry);
      if (shadowed->depth == binding.depth) {
        Report *report = diag_multiple_definition_error(
          token->node.span.offset, token->raw->node.span.text_length, token->raw->text, shadowed->declared_at);
        vec_push(&resolver->diags, &report, 1);
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

static void collect_semantic_events(const SyntaxTree *syntax, Resolver *resolver)
{
  SyntaxEvent event = syntax_event_alloc(syntax);
  while (syntax_event_next(&event)) {
    if (event.kind == SYNTAX_EVENT_ENTER) {
      switch (event.syntax->raw->node.kind) {
      case MPPL_SYNTAX_PROGRAM:
      case MPPL_SYNTAX_PROC_BODY:
        push_scope(resolver);
        break;

      case MPPL_SYNTAX_BIND_IDENT: {
        MpplBindIdentFields fields = mppl_bind_ident_fields_alloc((const MpplBindIdent *) event.syntax);
        enter_bind_ident(resolver, fields.ident);
        mppl_bind_ident_fields_free(&fields);
        break;
      }

      case MPPL_SYNTAX_REF_IDENT: {
        MpplRefIdentFields fields = mppl_ref_ident_fields_alloc((const MpplRefIdent *) event.syntax);
        enter_ref_ident(resolver, fields.ident);
        mppl_ref_ident_fields_free(&fields);
        break;
      }

      default:
        /* do nothing */
        break;
      }
    } else {
      switch (event.syntax->raw->node.kind) {
      case MPPL_SYNTAX_PROGRAM:
      case MPPL_SYNTAX_PROC_BODY:
        pop_scope(resolver);
        break;

      default:
        /* do nothing */
        break;
      }
    }
  }
  syntax_event_free(&event);
}

MpplResolveResult mppl_resolve(const MpplRoot *syntax)
{
  MpplResolveResult result;

  Resolver resolver;
  resolver.scope = NULL;
  hashmap_alloc(&resolver.bindings, &name_hash, &name_eq);
  vec_alloc(&resolver.events, 0);
  vec_alloc(&resolver.diags, 0);

  collect_semantic_events(&syntax->syntax, &resolver);

  result.semantics   = mppl_semantics_alloc(&syntax->syntax, resolver.events.ptr, resolver.events.count);
  result.diags.ptr   = resolver.diags.ptr;
  result.diags.count = resolver.diags.count;

  hashmap_free(&resolver.bindings);
  vec_free(&resolver.events);

  return result;
}
