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

Hash name_hash(const void *key)
{
  const Name *name = key;
  return hash_fnv1a(NULL, name->ptr, name->count);
}

int name_eq(const void *a, const void *b)
{
  const Name *l = a;
  const Name *r = b;
  return l->count == r->count && memcmp(l->ptr, r->ptr, l->count) == 0;
}

void diag(Resolver *resolver, Report *report)
{
  vec_push(&resolver->diags, &report, 1);
}

Binding binding_alloc(const char *name, unsigned long depth, unsigned long declared_at, unsigned long text_length)
{
  Binding binding;
  binding.depth       = depth;
  binding.declared_at = declared_at;
  binding.text_length = text_length;
  slice_alloc(&binding.name, text_length + 1);
  memcpy(binding.name.ptr, name, text_length);
  binding.name.ptr[text_length] = '\0';
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
    diag(resolver, diag_not_found_error(token->node.span.offset, token->raw->node.span.text_length, token->raw->text));
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
      diag(resolver,
        diag_multiple_definition_error(token->node.span.offset, token->raw->node.span.text_length, token->raw->text, shadowed->declared_at));
      binding_free(&binding);
    } else {
      vec_push(&resolver->scope->shadowed, shadowed, 1);
      hashmap_update(&resolver->bindings, &entry, &binding.name, &binding);
    }
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

    case MPPL_SYNTAX_VAR_DECL:
    case MPPL_SYNTAX_PROC_DECL:
    case MPPL_SYNTAX_FML_PARAM_SEC:
      enter_binding_decl(resolver, token);
      return;

    case MPPL_SYNTAX_PROGRAM:
      /* do nothing */
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

void do_resolve(Resolver *resolver, const SyntaxTree *syntax)
{
  SyntaxTree   *tree;
  SyntaxToken  *token;
  unsigned long i;

  for (i = 0; i < syntax->raw->children.count; i++) {
    if ((tree = syntax_tree_child_tree(syntax, i))) {
      if (tree->raw->node.kind == MPPL_SYNTAX_PROGRAM || tree->raw->node.kind == MPPL_SYNTAX_PROC_DECL) {
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

  result.diags.ptr   = resolver.diags.ptr;
  result.diags.count = resolver.diags.count;

  return result;
}
