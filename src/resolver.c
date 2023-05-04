#include <stdio.h>
#include <stdlib.h>

#include "ast.h"
#include "context.h"
#include "mpplc.h"
#include "utility.h"

typedef struct scope__s    scope_t;
typedef struct resolver__s resolver_t;

struct scope__s {
  scope_t *outer;
  hash_t  *def_map;
  def_t  **defs;
};

struct resolver__s {
  ast_visitor_t visitor;
  hash_t       *resolution;
  scope_t      *scope;
  def_t        *defs;
};

static def_t *add_def(resolver_t *resolver, const void *ast, const symbol_t *name, def_kind_t kind)
{
  const hash_entry_t *entry = hash_find(resolver->scope->def_map, name);
  if (!entry) {
    def_t *def = xmalloc(sizeof(def_t));
    def->ast   = ast;
    def->name  = name;
    def->kind  = kind;
    def->type  = NULL;
    def->inner = NULL;
    def->next  = NULL;

    *resolver->scope->defs = def;
    resolver->scope->defs  = &def->next;
    hash_insert_unchecked(resolver->scope->def_map, (void *) name, def);
    return def;
  } else {
    /* TODO: emit error message for conflict */
    printf("coflict detected!\n");
    return NULL;
  }
}

static void resolve_def(resolver_t *resolver, const void *ast, const symbol_t *name)
{
  scope_t     *scope = resolver->scope;
  const def_t *def   = NULL;
  for (; scope; scope = scope->outer) {
    const hash_entry_t *entry = hash_find(scope->def_map, name);
    if (entry) {
      def = entry->value;
      break;
    }
  }

  if (def) {
    hash_insert_unchecked(resolver->resolution, (void *) ast, (void *) def);
  } else {
    /* TODO: emit error message for resolution failure */
    printf("resolution failure!\n");
  }
}

static void push_scope(resolver_t *resolver, def_t **defs)
{
  scope_t *scope  = xmalloc(sizeof(scope_t));
  scope->outer    = resolver->scope;
  scope->def_map  = hash_new(&hash_default_comp, &hash_default_hasher);
  scope->defs     = defs;
  resolver->scope = scope;
}

static def_t **pop_scope(resolver_t *resolver)
{
  scope_t *scope  = resolver->scope;
  def_t  **defs   = scope->defs;
  resolver->scope = scope->outer;
  hash_delete(scope->def_map, NULL, NULL);
  free(scope);
  return defs;
}

static void visit_expr(ast_visitor_t *visitor, const ast_expr_t *expr)
{
  switch (expr->kind) {
  case AST_EXPR_KIND_ARRAY_SUBSCRIPT: {
    const ast_expr_array_subscript_t *subscript = (ast_expr_array_subscript_t *) expr;
    resolve_def((resolver_t *) visitor, subscript->decl, subscript->decl->symbol);
    break;
  }
  case AST_EXPR_KIND_DECL_REF: {
    const ast_expr_decl_ref_t *ref = (ast_expr_decl_ref_t *) expr;
    resolve_def((resolver_t *) visitor, ref->decl, ref->decl->symbol);
    break;
  }
  default:
    /* do nothing */
    break;
  }
}

static void visit_stmt(ast_visitor_t *visitor, const ast_stmt_t *stmt)
{
  switch (stmt->kind) {
  case AST_STMT_KIND_CALL: {
    const ast_stmt_call_t *call = (ast_stmt_call_t *) stmt;
    resolve_def((resolver_t *) visitor, call->name, call->name->symbol);
    break;
  }
  default:
    /* do nothing */
    break;
  }
  ast_walk_stmt(visitor, stmt);
}

static void visit_decl_variable(ast_visitor_t *visitor, const ast_decl_variable_t *variable)
{
  ast_ident_t *ident = variable->names;
  for (; ident; ident = ident->next) {
    add_def((resolver_t *) visitor, variable, ident->symbol, DEF_VAR);
  }
}

static void visit_decl_param(ast_visitor_t *visitor, const ast_decl_param_t *param)
{
  ast_ident_t *ident = param->names;
  for (; ident; ident = ident->next) {
    add_def((resolver_t *) visitor, param, ident->symbol, DEF_PARAM);
  }
}

static void visit_decl_part(ast_visitor_t *visitor, const ast_decl_part_t *decl_part)
{
  switch (decl_part->kind) {
  case AST_DECL_PART_PROCEDURE: {
    const ast_decl_part_procedure_t *proc = (ast_decl_part_procedure_t *) decl_part;

    def_t *def = add_def((resolver_t *) visitor, proc, proc->name->symbol, DEF_PROCEDURE);
    if (def) {
      push_scope((resolver_t *) visitor, &def->inner);
      ast_walk_decl_part(visitor, decl_part);
      pop_scope((resolver_t *) visitor);
    }
    break;
  }
  default:
    ast_walk_decl_part(visitor, decl_part);
    break;
  }
}

static void visit_program(ast_visitor_t *visitor, const ast_program_t *program)
{
  def_t *def = add_def((resolver_t *) visitor, program, program->name->symbol, DEF_PROGRAM);
  if (def) {
    push_scope((resolver_t *) visitor, &def->inner);
    ast_walk_program(visitor, program);
    pop_scope((resolver_t *) visitor);
  }
}

void mpplc_resolve(context_t *ctx)
{
  resolver_t     resolver;
  ast_visitor_t *visitor = (ast_visitor_t *) &resolver;
  resolver.resolution    = hash_new(&hash_default_comp, &hash_default_hasher);
  resolver.scope         = NULL;
  resolver.defs          = NULL;

  ast_init_visitor(visitor);
  visitor->visit_expr          = &visit_expr;
  visitor->visit_stmt          = &visit_stmt;
  visitor->visit_decl_variable = &visit_decl_variable;
  visitor->visit_decl_param    = &visit_decl_param;
  visitor->visit_decl_part     = &visit_decl_part;
  visitor->visit_program       = &visit_program;
  push_scope(&resolver, &resolver.defs);
  ast_walk(visitor, ctx->ast);
  pop_scope(&resolver);

  ctx->resolution = resolver.resolution;
  ctx->defs       = resolver.defs;
}
