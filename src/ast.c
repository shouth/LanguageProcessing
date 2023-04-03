#include <assert.h>
#include <stdlib.h>

#include "ast.h"

void ast_walk_lit(ast_visitor_t *visitor, ast_lit_t *lit)
{
  /* do nothing */
  (void) visitor;
  (void) lit;
}

void ast_walk_ident(ast_visitor_t *visitor, ast_ident_t *ident)
{
  /* do nothing */
  (void) visitor;
  (void) ident;
}

void ast_walk_type(ast_visitor_t *visitor, ast_type_t *type)
{
  if (type) {
    switch (type->kind) {
    case AST_TYPE_KIND_ARRAY: {
      ast_type_array_t *array = (ast_type_array_t *) type;
      visitor->visit_lit(visitor, array->size);
      visitor->visit_type(visitor, array->base);
      break;
    }
    case AST_TYPE_KIND_BOOLEAN:
    case AST_TYPE_KIND_CHAR:
    case AST_TYPE_KIND_INTEGER:
      /* do nothing */
      break;
    }
  }
}

void ast_walk_expr(ast_visitor_t *visitor, ast_expr_t *expr)
{
  if (expr) {
    switch (expr->kind) {
    case AST_EXPR_KIND_DECL_REF: {
      ast_expr_decl_ref_t *decl_ref = (ast_expr_decl_ref_t *) expr;
      ast_list_walk(visitor, visit_ident, ast_ident_t, decl_ref->decl, next);
      break;
    }
    case AST_EXPR_KIND_ARRAY_SUBSCRIPT: {
      ast_expr_array_subscript_t *array_subscript = (ast_expr_array_subscript_t *) expr;
      ast_list_walk(visitor, visit_ident, ast_ident_t, array_subscript->decl, next);
      ast_list_walk(visitor, visit_expr, ast_expr_t, array_subscript->subscript, next);
      break;
    }
    case AST_EXPR_KIND_BINARY: {
      ast_expr_binary_t *binary = (ast_expr_binary_t *) expr;
      ast_list_walk(visitor, visit_expr, ast_expr_t, binary->lhs, next);
      ast_list_walk(visitor, visit_expr, ast_expr_t, binary->rhs, next);
      break;
    }
    case AST_EXPR_KIND_NOT: {
      ast_expr_not_t *not_ = (ast_expr_not_t *) expr;
      ast_list_walk(visitor, visit_expr, ast_expr_t, not_->expr, next);
      break;
    }
    case AST_EXPR_KIND_PAREN: {
      ast_expr_paren_t *paren = (ast_expr_paren_t *) expr;
      ast_list_walk(visitor, visit_expr, ast_expr_t, paren->inner, next);
      break;
    }
    case AST_EXPR_KIND_CAST: {
      ast_expr_cast_t *cast = (ast_expr_cast_t *) expr;
      visitor->visit_type(visitor, cast->type);
      ast_list_walk(visitor, visit_expr, ast_expr_t, cast->cast, next);
      break;
    }
    case AST_EXPR_KIND_CONSTANT: {
      ast_expr_constant_t *constant = (ast_expr_constant_t *) expr;
      visitor->visit_lit(visitor, constant->lit);
      break;
    }
    case AST_EXPR_KIND_EMPTY:
      /* do nothing */
      break;
    }
  }
}

void ast_walk_out_fmt(ast_visitor_t *visitor, ast_out_fmt_t *out_fmt)
{
  if (out_fmt) {
    ast_list_walk(visitor, visit_expr, ast_expr_t, out_fmt->expr, next);
    visitor->visit_lit(visitor, out_fmt->len);
  }
}

void ast_walk_stmt(ast_visitor_t *visitor, ast_stmt_t *stmt)
{
  if (stmt) {
    switch (stmt->kind) {
    case AST_STMT_KIND_ASSIGN: {
      ast_stmt_assign_t *assign = (ast_stmt_assign_t *) stmt;
      ast_list_walk(visitor, visit_expr, ast_expr_t, assign->lhs, next);
      ast_list_walk(visitor, visit_expr, ast_expr_t, assign->rhs, next);
      break;
    }
    case AST_STMT_KIND_IF: {
      ast_stmt_if_t *if_ = (ast_stmt_if_t *) stmt;
      ast_list_walk(visitor, visit_expr, ast_expr_t, if_->cond, next);
      ast_list_walk(visitor, visit_stmt, ast_stmt_t, if_->then_stmt, next);
      ast_list_walk(visitor, visit_stmt, ast_stmt_t, if_->else_stmt, next);
      break;
    }
    case AST_STMT_KIND_WHILE: {
      ast_stmt_while_t *while_ = (ast_stmt_while_t *) stmt;
      ast_list_walk(visitor, visit_expr, ast_expr_t, while_->cond, next);
      ast_list_walk(visitor, visit_stmt, ast_stmt_t, while_->do_stmt, next);
      break;
    }
    case AST_STMT_KIND_CALL: {
      ast_stmt_call_t *call = (ast_stmt_call_t *) stmt;
      ast_list_walk(visitor, visit_ident, ast_ident_t, call->name, next);
      ast_list_walk(visitor, visit_expr, ast_expr_t, call->args, next);
      break;
    }
    case AST_STMT_KIND_READ: {
      ast_stmt_read_t *read = (ast_stmt_read_t *) stmt;
      ast_list_walk(visitor, visit_expr, ast_expr_t, read->args, next);
      break;
    }
    case AST_STMT_KIND_WRITE: {
      ast_stmt_write_t *write = (ast_stmt_write_t *) stmt;
      ast_list_walk(visitor, visit_out_fmt, ast_out_fmt_t, write->formats, next);
      break;
    }
    case AST_STMT_KIND_COMPOUND: {
      ast_stmt_compound_t *compound = (ast_stmt_compound_t *) stmt;
      ast_list_walk(visitor, visit_stmt, ast_stmt_t, compound->stmts, next);
      break;
    }

    case AST_STMT_KIND_RETURN:
    case AST_STMT_KIND_BREAK:
    case AST_STMT_KIND_EMPTY:
      /* do nothing */
      break;
    }
  }
}

void ast_walk_decl_variable(ast_visitor_t *visitor, ast_decl_variable_t *variable)
{
  if (variable) {
    ast_list_walk(visitor, visit_ident, ast_ident_t, variable->names, next);
    visitor->visit_type(visitor, variable->type);
  }
}

void ast_walk_decl_param(ast_visitor_t *visitor, ast_decl_param_t *param)
{
  if (param) {
    ast_list_walk(visitor, visit_ident, ast_ident_t, param->names, next);
    visitor->visit_type(visitor, param->type);
  }
}

void ast_walk_decl_part(ast_visitor_t *visitor, ast_decl_part_t *decl_part)
{
  if (decl_part) {
    switch (decl_part->kind) {
    case AST_DECL_PART_PROCEDURE: {
      ast_decl_part_procedure_t *procedure = (ast_decl_part_procedure_t *) decl_part;
      ast_list_walk(visitor, visit_ident, ast_ident_t, procedure->name, next);
      ast_list_walk(visitor, visit_decl_param, ast_decl_param_t, procedure->params, next);
      ast_list_walk(visitor, visit_decl_part, ast_decl_part_t, procedure->variables, next);
      ast_list_walk(visitor, visit_stmt, ast_stmt_t, procedure->stmt, next);
      break;
    }
    case AST_DECL_PART_VARIABLE: {
      ast_decl_part_variable_t *variable = (ast_decl_part_variable_t *) decl_part;
      ast_list_walk(visitor, visit_decl_variable, ast_decl_variable_t, variable->decls, next);
      break;
    }
    }
  }
}

void ast_walk_program(ast_visitor_t *visitor, ast_program_t *program)
{
  if (program) {
    ast_list_walk(visitor, visit_ident, ast_ident_t, program->name, next);
    ast_list_walk(visitor, visit_decl_part, ast_decl_part_t, program->decl_part, next);
    ast_list_walk(visitor, visit_stmt, ast_stmt_t, program->stmt, next);
  }
}

void ast_walk(ast_visitor_t *visitor, ast_t *ast)
{
  if (ast) {
    visitor->visit_program(visitor, ast->program);
  }
}

void ast_init_visitor(ast_visitor_t *visitor)
{
  if (visitor) {
    visitor->visit_lit           = &ast_walk_lit;
    visitor->visit_ident         = &ast_walk_ident;
    visitor->visit_type          = &ast_walk_type;
    visitor->visit_expr          = &ast_walk_expr;
    visitor->visit_out_fmt       = &ast_walk_out_fmt;
    visitor->visit_stmt          = &ast_walk_stmt;
    visitor->visit_decl_variable = &ast_walk_decl_variable;
    visitor->visit_decl_param    = &ast_walk_decl_param;
    visitor->visit_decl_part     = &ast_walk_decl_part;
    visitor->visit_program       = &ast_walk_program;
  }
}

/* clang-format off */

#define define_deleter(name, type, walker)         \
  void name(ast_visitor_t *visitor, type *pointer) \
  {                                                \
    walker(visitor, pointer);                      \
    free(pointer);                                 \
  }

define_deleter(delete_lit, ast_lit_t, ast_walk_lit)
define_deleter(delete_ident, ast_ident_t, ast_walk_ident)
define_deleter(delete_type, ast_type_t, ast_walk_type)
define_deleter(delete_expr, ast_expr_t, ast_walk_expr)
define_deleter(delete_out_fmt, ast_out_fmt_t, ast_walk_out_fmt)
define_deleter(delete_stmt, ast_stmt_t, ast_walk_stmt)
define_deleter(delete_decl_variable, ast_decl_variable_t, ast_walk_decl_variable)
define_deleter(delete_decl_param, ast_decl_param_t, ast_walk_decl_param)
define_deleter(delete_decl_part, ast_decl_part_t, ast_walk_decl_part)
define_deleter(delete_program, ast_program_t, ast_walk_program)

#undef define_deleter

void ast_delete(ast_t *ast)
{
  if (ast) {
    ast_visitor_t visitor;
    visitor.visit_lit           = &delete_lit;
    visitor.visit_ident         = &delete_ident;
    visitor.visit_type          = &delete_type;
    visitor.visit_expr          = &delete_expr;
    visitor.visit_out_fmt       = &delete_out_fmt;
    visitor.visit_stmt          = &delete_stmt;
    visitor.visit_decl_variable = &delete_decl_variable;
    visitor.visit_decl_param    = &delete_decl_param;
    visitor.visit_decl_part     = &delete_decl_part;
    visitor.visit_program       = &delete_program;
    ast_walk(&visitor, ast);
    symbol_context_delete(ast->symbols);
  }
  free(ast);
}

/* clang-format on */
