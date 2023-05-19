#include <stdio.h>
#include <stdlib.h>

#include "ast.h"
#include "context.h"
#include "message.h"
#include "mpplc.h"
#include "source.h"
#include "types.h"
#include "utility.h"

typedef struct scope__s    scope_t;
typedef struct resolver__s resolver_t;

struct scope__s {
  scope_t    *outer;
  hash_map_t *def_map;
  def_t     **defs;
};

struct resolver__s {
  ast_visitor_t   visitor;
  const source_t *src;
  hash_map_t     *resolution;
  scope_t        *scope;
  def_t          *defs;
};

void error_conflict(resolver_t *resolver, def_t *def, region_t region)
{
  msg_t *msg = msg_new(resolver->src, region, MSG_ERROR, "conflicting names");
  msg_add_inline(msg, def->region, "first used here");
  msg_add_inline(msg, region, "second used here");
  msg_emit(msg);
}

void error_undeclared(resolver_t *resolver, const symbol_t *name, region_t region)
{
  msg_t *msg = msg_new(resolver->src, region, MSG_ERROR, "%s is not declared", name->ptr);
  msg_emit(msg);
}

static def_t *add_def(resolver_t *resolver, def_kind_t kind, const void *ast, const symbol_t *name, region_t region)
{
  const hash_map_entry_t *entry = hash_map_find(resolver->scope->def_map, name);
  if (!entry) {
    def_t *def  = xmalloc(sizeof(def_t));
    def->ast    = ast;
    def->name   = name;
    def->region = region;
    def->kind   = kind;
    def->inner  = NULL;
    def->next   = NULL;

    *resolver->scope->defs = def;
    resolver->scope->defs  = &def->next;
    hash_map_update(resolver->scope->def_map, (void *) name, def);
    return def;
  } else {
    error_conflict(resolver, entry->value, region);
    return NULL;
  }
}

static void resolve_def(resolver_t *resolver, const void *ast, const symbol_t *name, region_t region)
{
  scope_t     *scope = resolver->scope;
  const def_t *def   = NULL;
  for (; scope; scope = scope->outer) {
    const hash_map_entry_t *entry = hash_map_find(scope->def_map, name);
    if (entry) {
      def = entry->value;
      break;
    }
  }

  if (def) {
    hash_map_update(resolver->resolution, (void *) ast, (void *) def);
  } else {
    error_undeclared(resolver, name, region);
  }
}

static void push_scope(resolver_t *resolver, def_t **defs)
{
  scope_t *scope  = xmalloc(sizeof(scope_t));
  scope->outer    = resolver->scope;
  scope->def_map  = hash_map_new(NULL, NULL);
  scope->defs     = defs;
  resolver->scope = scope;
}

static def_t **pop_scope(resolver_t *resolver)
{
  scope_t *scope  = resolver->scope;
  def_t  **defs   = scope->defs;
  resolver->scope = scope->outer;
  hash_map_delete(scope->def_map);
  free(scope);
  return defs;
}

static void visit_expr(ast_visitor_t *visitor, const ast_expr_t *expr)
{
  switch (expr->kind) {
  case AST_EXPR_KIND_ARRAY_SUBSCRIPT: {
    const ast_expr_array_subscript_t *subscript = (ast_expr_array_subscript_t *) expr;
    resolve_def((resolver_t *) visitor, subscript->decl, subscript->decl->symbol, subscript->decl->region);
    break;
  }
  case AST_EXPR_KIND_DECL_REF: {
    const ast_expr_decl_ref_t *ref = (ast_expr_decl_ref_t *) expr;
    resolve_def((resolver_t *) visitor, ref->decl, ref->decl->symbol, ref->decl->region);
    break;
  }
  default:
    /* do nothing */
    break;
  }
  ast_walk_expr(visitor, expr);
}

static void visit_stmt(ast_visitor_t *visitor, const ast_stmt_t *stmt)
{
  switch (stmt->kind) {
  case AST_STMT_KIND_CALL: {
    const ast_stmt_call_t *call = (ast_stmt_call_t *) stmt;
    resolve_def((resolver_t *) visitor, call->name, call->name->symbol, call->name->region);
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
    add_def((resolver_t *) visitor, DEF_VAR, variable, ident->symbol, ident->region);
  }
}

static void visit_decl_param(ast_visitor_t *visitor, const ast_decl_param_t *param)
{
  ast_ident_t *ident = param->names;
  for (; ident; ident = ident->next) {
    add_def((resolver_t *) visitor, DEF_PARAM, param, ident->symbol, ident->region);
  }
}

static void visit_decl_part(ast_visitor_t *visitor, const ast_decl_part_t *decl_part)
{
  switch (decl_part->kind) {
  case AST_DECL_PART_PROCEDURE: {
    const ast_decl_part_procedure_t *proc = (ast_decl_part_procedure_t *) decl_part;

    def_t *def = add_def((resolver_t *) visitor, DEF_PROCEDURE, proc, proc->name->symbol, proc->name->region);
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
  def_t *def = add_def((resolver_t *) visitor, DEF_PROGRAM, program, program->name->symbol, program->name->region);
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
  resolver.src           = ctx->src;
  resolver.resolution    = hash_map_new(NULL, NULL);
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
