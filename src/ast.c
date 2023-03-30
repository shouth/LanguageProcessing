#include <assert.h>
#include <stdlib.h>

#include "ast.h"

const char *ast_binop_str(ast_expr_binary_kind_t kind)
{
  switch (kind) {
  case AST_EXPR_BINARY_KIND_STAR:
    return "*";
  case AST_EXPR_BINARY_KIND_DIV:
    return "div";
  case AST_EXPR_BINARY_KIND_AND:
    return "and";
  case AST_EXPR_BINARY_KIND_PLUS:
    return "+";
  case AST_EXPR_BINARY_KIND_MINUS:
    return "-";
  case AST_EXPR_BINARY_KIND_OR:
    return "or";
  case AST_EXPR_BINARY_KIND_EQUAL:
    return "=";
  case AST_EXPR_BINARY_KIND_NOTEQ:
    return "<>";
  case AST_EXPR_BINARY_KIND_LE:
    return "<";
  case AST_EXPR_BINARY_KIND_LEEQ:
    return "<=";
  case AST_EXPR_BINARY_KIND_GR:
    return ">";
  case AST_EXPR_BINARY_KIND_GREQ:
    return ">=";
  default:
    unreachable();
  }
}

static void delete_ast_lit(ast_lit_t *lit)
{
  free(lit);
}

static void delete_ast_type(ast_type_t *type)
{
  if (type) {
    if (type->kind == AST_TYPE_KIND_ARRAY) {
      delete_ast_type(type->type.array.base);
      delete_ast_lit(type->type.array.size);
    }
  }
  free(type);
}

static void delete_ast_ident(ast_ident_t *ident)
{
  if (ident) {
    delete_ast_ident(ident->next);
  }
  free(ident);
}

static void delete_ast_expr(ast_expr_t *expr)
{
  if (expr) {
    switch (expr->kind) {
    case AST_EXPR_KIND_BINARY:
      delete_ast_expr(expr->expr.binary.lhs);
      delete_ast_expr(expr->expr.binary.rhs);
      break;
    case AST_EXPR_KIND_UNARY:
      delete_ast_expr(expr->expr.not .expr);
      break;
    case AST_EXPR_KIND_PAREN:
      delete_ast_expr(expr->expr.paren.inner);
      break;
    case AST_EXPR_KIND_CAST:
      delete_ast_type(expr->expr.cast.type);
      delete_ast_expr(expr->expr.cast.cast);
      break;
    case AST_EXPR_KIND_DECL_REF:
      delete_ast_ident(expr->expr.decl_ref.decl);
      break;
    case AST_EXPR_KIND_ARRAY_SUBSCRIPT:
      delete_ast_ident(expr->expr.array_subscript.decl);
      delete_ast_expr(expr->expr.array_subscript.subscript);
      break;
    case AST_EXPR_KIND_CONSTANT:
      delete_ast_lit(expr->expr.constant.lit);
      break;
    case AST_EXPR_KIND_EMPTY:
      /* do nothing */
      break;
    }
    delete_ast_expr(expr->next);
  }
  free(expr);
}

static void delete_ast_output_format(ast_output_format_t *format)
{
  if (format) {
    delete_ast_expr(format->expr);
    delete_ast_lit(format->len);
    delete_ast_output_format(format->next);
  }
  free(format);
}

static void delete_ast_stmt(ast_stmt_t *stmt)
{
  if (stmt) {
    switch (stmt->kind) {
    case AST_STMT_KIND_ASSIGN:
      delete_ast_expr(stmt->stmt.assign.lhs);
      delete_ast_expr(stmt->stmt.assign.rhs);
      break;
    case AST_STMT_KIND_IF:
      delete_ast_expr(stmt->stmt.if_.cond);
      delete_ast_stmt(stmt->stmt.if_.then_stmt);
      delete_ast_stmt(stmt->stmt.if_.else_stmt);
      break;
    case AST_STMT_KIND_WHILE:
      delete_ast_expr(stmt->stmt.while_.cond);
      delete_ast_stmt(stmt->stmt.while_.do_stmt);
      break;
    case AST_STMT_KIND_CALL:
      delete_ast_ident(stmt->stmt.call.name);
      delete_ast_expr(stmt->stmt.call.args);
      break;
    case AST_STMT_KIND_READ:
      delete_ast_expr(stmt->stmt.read.args);
      break;
    case AST_STMT_KIND_WRITE:
      delete_ast_output_format(stmt->stmt.write.formats);
      break;
    case AST_STMT_KIND_COMPOUND:
      delete_ast_stmt(stmt->stmt.compound.stmts);
      break;
    case AST_STMT_KIND_BREAK:
    case AST_STMT_KIND_RETURN:
    case AST_STMT_KIND_EMPTY:
      /* do nothing */
      break;
    }
    delete_ast_stmt(stmt->next);
  }
  free(stmt);
}

static void delete_ast_param_decl(ast_decl_param_t *params)
{
  if (params) {
    delete_ast_ident(params->names);
    delete_ast_type(params->type);
    delete_ast_param_decl(params->next);
  }
  free(params);
}

static void delete_ast_variable_decl(ast_decl_variable_t *decl)
{
  if (decl) {
    delete_ast_ident(decl->names);
    delete_ast_type(decl->type);
    delete_ast_variable_decl(decl->next);
  }
  free(decl);
}

static void delete_decl_part(ast_decl_part_t *decl)
{
  if (decl) {
    switch (decl->kind) {
    case AST_DECL_PART_VARIABLE:
      delete_ast_variable_decl(decl->decl_part.variable.decls);
      break;
    case AST_DECL_PART_PROCEDURE:
      delete_ast_ident(decl->decl_part.procedure.name);
      delete_ast_param_decl(decl->decl_part.procedure.params);
      delete_ast_stmt(decl->decl_part.procedure.stmt);
      delete_decl_part(decl->decl_part.procedure.variables);
      break;
    }
    delete_decl_part(decl->next);
  }
  free(decl);
}

static void delete_program(ast_program_t *program)
{
  if (program) {
    delete_ast_ident(program->name);
    delete_decl_part(program->decl_part);
    delete_ast_stmt(program->stmt);
  }
  free(program);
}

void delete_ast(ast_t *ast)
{
  if (ast) {
    delete_program(ast->program);
    symbol_context_delete(ast->symbols);
  }
  free(ast);
}
