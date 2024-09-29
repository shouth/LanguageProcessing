#include <assert.h>

#include "diag.h"
#include "mppl_passes.h"
#include "mppl_semantic.h"
#include "mppl_syntax.h"
#include "syntax_tree.h"
#include "util.h"

typedef Slice(char) Name;
typedef struct Binding  Binding;
typedef struct Scope    Scope;
typedef struct Resolver Resolver;

struct Binding {
  Name          name;
  unsigned long depth;
  unsigned long declared_at;
  unsigned long text_length;
};

struct Scope {
  Scope        *parent;
  unsigned long depth;
  Vec(Binding) bindings;
  Vec(Binding) shadowed;
};

struct Resolver {
  Scope *scope;
  HashMap(Name, Binding) bindings;
  Vec(MpplSemanticEvent) events;
  Vec(Report *) diags;
};

Binding binding_alloc(const char *name, unsigned long depth, unsigned long declared_at, unsigned long text_length)
{
  Binding binding;
  binding.depth       = depth;
  binding.declared_at = declared_at;
  binding.text_length = text_length;
  slice_alloc(&binding.name, strlen(name));
  return binding;
}

void binding_free(Binding *binding)
{
  slice_free(&binding->name);
}

void enter_binding_use(Resolver *resolver, const SyntaxToken *token)
{
  MpplSemanticEvent event;
  HashMapEntry      entry;
  Name              name;

  assert(token->raw->node.kind == MPPL_SYNTAX_IDENT_TOKEN);

  name.ptr   = token->raw->text;
  name.count = token->raw->node.span.text_length;

  if (hashmap_entry(&resolver->bindings, &name, &entry)) {
    event.kind        = MPPL_SEMANTIC_USE;
    event.declared_at = hashmap_value(&resolver->bindings, &entry)->declared_at;
    event.used_at     = token->node.span.offset;
  } else {
    event.kind    = MPPL_SEMANTIC_NOT_FOUND;
    event.used_at = token->node.span.offset;
  }
  vec_push(&resolver->events, &event, 1);
}

void enter_binding_decl(Resolver *resolver, const SyntaxToken *token)
{
  MpplSemanticEvent event;
  HashMapEntry      entry;
  Binding           binding;

  assert(token->raw->node.kind == MPPL_SYNTAX_IDENT_TOKEN);

  binding = binding_alloc(
    token->raw->text, resolver->scope->depth, token->node.span.offset, token->raw->node.span.text_length);

  if (hashmap_entry(&resolver->bindings, &binding.name, &entry)) {
    Binding *shadowed = hashmap_value(&resolver->bindings, &entry);
    if (shadowed->depth == binding.depth) {
      /* TODO: handle name conflict */
      binding_free(&binding);
    } else {
      vec_push(&resolver->scope->shadowed, shadowed, 1);
    }
    hashmap_update(&resolver->bindings, &entry, &binding.name, &binding);
  }

  event.kind        = MPPL_SEMANTIC_DEFINE;
  event.declared_at = token->node.span.offset;
  vec_push(&resolver->events, &event, 1);
}

void enter_ident(Resolver *resolver, const SyntaxToken *token)
{
  const SyntaxTree *tree;

  assert(token->raw->node.kind == MPPL_SYNTAX_IDENT_TOKEN);
  for (tree = token->node.parent; tree; tree = tree->node.parent) {
    switch (tree->raw->node.kind) {
    case MPPL_SYNTAX_ENTIRE_VAR:
    case MPPL_SYNTAX_INDEXED_VAR:
    case MPPL_SYNTAX_CALL_STMT:
      enter_binding_use(resolver, token);
      return;

    case MPPL_SYNTAX_PROGRAM:
    case MPPL_SYNTAX_VAR_DECL:
    case MPPL_SYNTAX_PROC_DECL:
    case MPPL_SYNTAX_FML_PARAM_SEC:
      enter_binding_decl(resolver, token);
      return;

    default:
      /* do nothing */
      break;
    }
  }
  unreachable();
}

void push_scope(Resolver *resolver)
{
  Scope *scope  = malloc(sizeof(Scope));
  scope->parent = resolver->scope;
  vec_alloc(&scope->bindings, 0);
  vec_alloc(&scope->shadowed, 0);
  resolver->scope = scope;
}

void pop_scope(Resolver *resolver)
{
  unsigned long i;
  HashMapEntry  entry;

  Scope *scope = resolver->scope;
  for (i = 0; i < scope->bindings.count; i++) {
    Binding *binding = &scope->bindings.ptr[i];
    hashmap_entry(&resolver->bindings, &binding->name, &entry);
    hashmap_erase(&resolver->bindings, &entry);
    binding_free(binding);
  }

  for (i = 0; i < scope->shadowed.count; i++) {
    Binding *binding = &scope->shadowed.ptr[i];
    hashmap_entry(&resolver->bindings, &binding->name, &entry);
    hashmap_update(&resolver->bindings, &entry, &binding->name, binding);
  }

  vec_free(&scope->bindings);
  vec_free(&scope->shadowed);
  resolver->scope = scope->parent;
  free(scope);
}

MpplResolveResult mppl_resolve(const SyntaxTree *tree)
{
}
