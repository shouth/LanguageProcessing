#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "ast.h"
#include "lexer.h"

typedef struct {
  color_t foreground;
  color_t program;
  color_t keyword;
  color_t operator;
  color_t procedure;
  color_t argument;
  color_t string;
  color_t literal;
} color_scheme_t;

const color_scheme_t monokai = {
  0xc9d1d9, 0x66d9ef, 0xf92672, 0xf92672, 0xa6e22e, 0xfd971f, 0xe6db74, 0xae81ff
};

typedef struct {
  size_t                indent;
  const color_scheme_t *color_scheme;
} printer_t;

void pp_symbol(printer_t *printer, const symbol_t *symbol)
{
  assert(printer);
  printf("%.*s", (int) symbol->len, symbol->ptr);
}

void pp_colored_program(printer_t *printer, const ast_ident_t *ident)
{
  assert(printer && ident);
  console_24bit(printer->color_scheme->program);
  pp_symbol(printer, ident->symbol);
  console_reset();
}

void pp_colored_keyword(printer_t *printer, token_kind_t type)
{
  assert(printer);
  console_24bit(printer->color_scheme->keyword);
  printf("%s", token_to_str(type));
  console_reset();
}

void pp_colored_operator(printer_t *printer, token_kind_t type)
{
  assert(printer);
  console_24bit(printer->color_scheme->operator);
  printf("%s", token_to_str(type));
  console_reset();
}

void pp_colored_procedure(printer_t *printer, const ast_ident_t *ident)
{
  assert(printer && ident);
  console_24bit(printer->color_scheme->procedure);
  pp_symbol(printer, ident->symbol);
  console_reset();
}

void pp_colored_reserved_function(printer_t *printer, token_kind_t type)
{
  assert(printer);
  console_24bit(printer->color_scheme->procedure);
  printf("%s", token_to_str(type));
  console_reset();
}

void pp_colored_parameter(printer_t *printer, const ast_ident_t *ident)
{
  assert(printer && ident);
  console_24bit(printer->color_scheme->argument);
  pp_symbol(printer, ident->symbol);
  console_reset();
}

void pp_colored_string(printer_t *printer, const ast_lit_string_t *lit)
{
  assert(printer && lit);
  console_24bit(printer->color_scheme->string);
  printf("\'");
  pp_symbol(printer, lit->symbol);
  printf("\'");
  console_reset();
}

void pp_colored_number(printer_t *printer, const ast_lit_number_t *lit)
{
  assert(printer && lit);
  console_24bit(printer->color_scheme->literal);
  pp_symbol(printer, lit->symbol);
  console_reset();
}

void pp_colored_reserved_lit(printer_t *printer, token_kind_t type)
{
  assert(printer);
  console_24bit(printer->color_scheme->literal);
  printf("%s", token_to_str(type));
  console_reset();
}

void pp_type(printer_t *printer, const ast_type_t *type)
{
  assert(printer && type);
  switch (type->kind) {
  case AST_TYPE_KIND_INTEGER:
    pp_colored_keyword(printer, TOKEN_INTEGER);
    break;
  case AST_TYPE_KIND_BOOLEAN:
    pp_colored_keyword(printer, TOKEN_BOOLEAN);
    break;
  case AST_TYPE_KIND_CHAR:
    pp_colored_keyword(printer, TOKEN_CHAR);
    break;
  case AST_TYPE_KIND_ARRAY:
    pp_colored_keyword(printer, TOKEN_ARRAY);
    printf("[");
    pp_colored_number(printer, &type->type.array.size->lit.number);
    printf("] ");
    pp_colored_keyword(printer, TOKEN_OF);
    printf(" ");
    pp_type(printer, type->type.array.base);
    break;
  }
}

void pp_indent(printer_t *printer)
{
  size_t i;
  assert(printer);
  for (i = 0; i < printer->indent; i++) {
    printf("    ");
  }
}

void pp_ident(printer_t *printer, const ast_ident_t *ident)
{
  assert(printer && ident);

  pp_symbol(printer, ident->symbol);
  ident = ident->next;
  while (ident) {
    printf(", ");
    pp_symbol(printer, ident->symbol);
    ident = ident->next;
  }
}

void pp_lit(printer_t *printer, const ast_lit_t *lit)
{
  assert(printer && lit);
  switch (lit->kind) {
  case AST_LIT_KIND_NUMBER:
    pp_colored_number(printer, &lit->lit.number);
    break;
  case AST_LIT_KIND_BOOLEAN:
    pp_colored_reserved_lit(printer, lit->lit.boolean.value ? TOKEN_TRUE : TOKEN_FALSE);
    break;
  case AST_LIT_KIND_STRING:
    pp_colored_string(printer, &lit->lit.string);
    break;
  }
}

void pp_expr(printer_t *printer, const ast_expr_t *expr);

void pp_binary_op_expr(printer_t *printer, const ast_expr_binary_t *expr)
{
  assert(printer && expr);
  if (expr->lhs->kind != AST_EXPR_KIND_EMPTY) {
    pp_expr(printer, expr->lhs);
    printf(" ");
  }
  switch (expr->kind) {
  case AST_EXPR_BINARY_KIND_STAR:
    pp_colored_operator(printer, TOKEN_STAR);
    break;
  case AST_EXPR_BINARY_KIND_DIV:
    pp_colored_operator(printer, TOKEN_DIV);
    break;
  case AST_EXPR_BINARY_KIND_AND:
    pp_colored_operator(printer, TOKEN_AND);
    break;
  case AST_EXPR_BINARY_KIND_PLUS:
    pp_colored_operator(printer, TOKEN_PLUS);
    break;
  case AST_EXPR_BINARY_KIND_MINUS:
    pp_colored_operator(printer, TOKEN_MINUS);
    break;
  case AST_EXPR_BINARY_KIND_OR:
    pp_colored_operator(printer, TOKEN_OR);
    break;
  case AST_EXPR_BINARY_KIND_EQUAL:
    pp_colored_operator(printer, TOKEN_EQUAL);
    break;
  case AST_EXPR_BINARY_KIND_NOTEQ:
    pp_colored_operator(printer, TOKEN_NOTEQ);
    break;
  case AST_EXPR_BINARY_KIND_LE:
    pp_colored_operator(printer, TOKEN_LE);
    break;
  case AST_EXPR_BINARY_KIND_LEEQ:
    pp_colored_operator(printer, TOKEN_LEEQ);
    break;
  case AST_EXPR_BINARY_KIND_GR:
    pp_colored_operator(printer, TOKEN_GR);
    break;
  case AST_EXPR_BINARY_KIND_GREQ:
    pp_colored_operator(printer, TOKEN_GREQ);
    break;
  }
  if (expr->lhs->kind != AST_EXPR_KIND_EMPTY) {
    printf(" ");
  }
  pp_expr(printer, expr->rhs);
}

void pp_unary_op_expr(printer_t *printer, const ast_expr_unary_t *expr)
{
  switch (expr->kind) {
  case AST_EXPR_UNARY_KIND_NOT:
    pp_colored_operator(printer, TOKEN_NOT);
    printf(" ");
    pp_expr(printer, expr->expr);
    break;
  }
}

void pp_paren_expr(printer_t *printer, const ast_expr_paren_t *expr)
{
  assert(printer && expr);
  printf("(");
  pp_expr(printer, expr->inner);
  printf(")");
}

void pp_cast_expr(printer_t *printer, const ast_expr_cast_t *expr)
{
  assert(printer && expr);
  pp_type(printer, expr->type);
  printf("(");
  pp_expr(printer, expr->cast);
  printf(")");
}

void pp_decl_ref_expr(printer_t *printer, const ast_expr_decl_ref_t *expr)
{
  assert(printer && expr);
  pp_ident(printer, expr->decl);
}

void pp_array_subscript_expr(printer_t *printer, const ast_expr_array_subscript_t *expr)
{
  assert(printer && expr);
  pp_ident(printer, expr->decl);
  printf("[");
  pp_expr(printer, expr->subscript);
  printf("]");
}

void pp_constant_expr(printer_t *printer, const ast_expr_constant_t *expr)
{
  assert(printer && expr);
  pp_lit(printer, expr->lit);
}

void pp_expr(printer_t *printer, const ast_expr_t *expr)
{
  const ast_expr_t *cur;
  assert(printer && expr);
  cur = expr;
  while (cur) {
    switch (cur->kind) {
    case AST_EXPR_KIND_BINARY:
      pp_binary_op_expr(printer, (ast_expr_binary_t *) cur);
      break;
    case AST_EXPR_KIND_UNARY:
      pp_unary_op_expr(printer, (ast_expr_unary_t *) cur);
      break;
    case AST_EXPR_KIND_PAREN:
      pp_paren_expr(printer, (ast_expr_paren_t *) cur);
      break;
    case AST_EXPR_KIND_CAST:
      pp_cast_expr(printer, (ast_expr_cast_t *) cur);
      break;
    case AST_EXPR_KIND_DECL_REF:
      pp_decl_ref_expr(printer, (ast_expr_decl_ref_t *) cur);
      break;
    case AST_EXPR_KIND_ARRAY_SUBSCRIPT:
      pp_array_subscript_expr(printer, (ast_expr_array_subscript_t *) cur);
      break;
    case AST_EXPR_KIND_CONSTANT:
      pp_constant_expr(printer, (ast_expr_constant_t *) cur);
      break;
    case AST_EXPR_KIND_EMPTY:
      break;
    }
    if ((cur = cur->next)) {
      printf(", ");
    }
  }
}

void pp_stmt(printer_t *printer, const ast_stmt_t *stmt);

void pp_stmt_assign(printer_t *printer, const ast_stmt_assign_t *stmt)
{
  pp_expr(printer, stmt->lhs);
  printf(" ");
  pp_colored_operator(printer, TOKEN_ASSIGN);
  printf(" ");
  pp_expr(printer, stmt->rhs);
}

void pp_structured_stmt(printer_t *printer, const ast_stmt_t *stmt)
{
  if (stmt->kind != AST_STMT_KIND_EMPTY) {
    printf("\n");
  }
  if (stmt->kind != AST_STMT_KIND_COMPOUND) {
    printer->indent++;
    pp_indent(printer);
    pp_stmt(printer, stmt);
    printer->indent--;
  } else {
    pp_indent(printer);
    pp_stmt(printer, stmt);
  }
}

void pp_stmt_if(printer_t *printer, const ast_stmt_if_t *stmt)
{
  assert(printer && stmt);
  pp_colored_keyword(printer, TOKEN_IF);
  printf(" ");
  pp_expr(printer, stmt->cond);
  printf(" ");
  pp_colored_keyword(printer, TOKEN_THEN);
  pp_structured_stmt(printer, stmt->then_stmt);
  if (stmt->else_stmt) {
    printf("\n");
    pp_indent(printer);
    pp_colored_keyword(printer, TOKEN_ELSE);
    if (stmt->else_stmt->kind == AST_STMT_KIND_IF) {
      printf(" ");
      pp_stmt(printer, stmt->else_stmt);
    } else {
      pp_structured_stmt(printer, stmt->else_stmt);
    }
  }
}

void pp_stmt_while(printer_t *printer, const ast_stmt_while_t *stmt)
{
  assert(printer && stmt);
  pp_colored_keyword(printer, TOKEN_WHILE);
  printf(" ");
  pp_expr(printer, stmt->cond);
  printf(" ");
  pp_colored_keyword(printer, TOKEN_DO);
  pp_structured_stmt(printer, stmt->do_stmt);
}

void pp_stmt_call(printer_t *printer, const ast_stmt_call_t *stmt)
{
  assert(printer && stmt);
  pp_colored_keyword(printer, TOKEN_CALL);
  printf(" ");
  pp_colored_procedure(printer, stmt->name);
  if (stmt->args) {
    printf("(");
    pp_expr(printer, stmt->args);
    printf(")");
  }
}

void pp_stmt_read(printer_t *printer, const ast_stmt_read_t *stmt)
{
  assert(printer && stmt);
  pp_colored_reserved_function(printer, stmt->newline ? TOKEN_READLN : TOKEN_READ);
  if (stmt->args) {
    printf("(");
    pp_expr(printer, stmt->args);
    printf(")");
  }
}

void pp_stmt_write(printer_t *printer, const ast_stmt_write_t *stmt)
{
  assert(printer && stmt);
  pp_colored_reserved_function(printer, stmt->newline ? TOKEN_WRITELN : TOKEN_WRITE);
  if (stmt->formats) {
    ast_output_format_t *cur = stmt->formats;
    printf("(");
    while (cur) {
      if (cur != stmt->formats) {
        printf(", ");
      }
      pp_expr(printer, cur->expr);
      if (cur->len) {
        printf(" : ");
        pp_colored_number(printer, (ast_lit_number_t *) cur->len);
      }
      cur = cur->next;
    }
    printf(")");
  }
}

void pp_stmt_compound(printer_t *printer, const ast_stmt_compound_t *stmt)
{
  ast_stmt_t *cur;
  assert(printer && stmt);
  pp_colored_keyword(printer, TOKEN_BEGIN);
  printer->indent++;
  cur = stmt->stmts;
  if (cur->next || cur->kind != AST_STMT_KIND_EMPTY) {
    printf("\n");
  }
  while (cur) {
    pp_indent(printer);
    pp_stmt(printer, cur);
    if ((cur = cur->next)) {
      printf(";");
      if (!cur->next && cur->kind == AST_STMT_KIND_EMPTY) {
        break;
      }
      printf("\n");
    }
  }
  printer->indent--;
  printf("\n");
  pp_indent(printer);
  pp_colored_keyword(printer, TOKEN_END);
}

void pp_stmt(printer_t *printer, const ast_stmt_t *stmt)
{
  assert(printer && stmt);
  switch (stmt->kind) {
  case AST_STMT_KIND_ASSIGN:
    pp_stmt_assign(printer, (ast_stmt_assign_t *) stmt);
    break;
  case AST_STMT_KIND_IF:
    pp_stmt_if(printer, (ast_stmt_if_t *) stmt);
    break;
  case AST_STMT_KIND_WHILE:
    pp_stmt_while(printer, (ast_stmt_while_t *) stmt);
    break;
  case AST_STMT_KIND_BREAK:
    pp_colored_keyword(printer, TOKEN_BREAK);
    break;
  case AST_STMT_KIND_CALL:
    pp_stmt_call(printer, (ast_stmt_call_t *) stmt);
    break;
  case AST_STMT_KIND_RETURN:
    pp_colored_keyword(printer, TOKEN_RETURN);
    break;
  case AST_STMT_KIND_READ:
    pp_stmt_read(printer, (ast_stmt_read_t *) stmt);
    break;
  case AST_STMT_KIND_WRITE:
    pp_stmt_write(printer, (ast_stmt_write_t *) stmt);
    break;
  case AST_STMT_KIND_COMPOUND:
    pp_stmt_compound(printer, (ast_stmt_compound_t *) stmt);
    break;
  case AST_STMT_KIND_EMPTY:
    break;
  }
}

void pp_decl_part(printer_t *printer, const ast_decl_part_t *decl_part);

void pp_variable_decl_part(printer_t *printer, const ast_decl_part_variable_t *decl_part)
{
  const ast_decl_variable_t *cur;
  assert(printer && decl_part);

  pp_colored_keyword(printer, TOKEN_VAR);
  printf("\n");
  cur = decl_part->decls;
  printer->indent++;
  while (cur) {
    pp_indent(printer);
    pp_ident(printer, cur->names);
    printf(": ");
    pp_type(printer, cur->type);
    printf(";\n");
    cur = cur->next;
  }
  printer->indent--;
}

void pp_procedure_decl_part(printer_t *printer, const ast_decl_part_procedure_t *decl_part)
{
  assert(printer && decl_part);
  pp_colored_keyword(printer, TOKEN_PROCEDURE);
  printf(" ");
  pp_colored_procedure(printer, decl_part->name);
  if (decl_part->params) {
    const ast_decl_param_t *cur = decl_part->params;
    printf("(");
    while (cur) {
      const ast_ident_t *ident = cur->names;
      if (cur != decl_part->params) {
        printf("; ");
      }
      while (ident) {
        pp_colored_parameter(printer, ident);
        if ((ident = ident->next)) {
          printf(", ");
        }
      }
      printf(": ");
      pp_type(printer, cur->type);
      cur = cur->next;
    }
    printf(")");
  }
  printf(";\n");
  if (decl_part->variables) {
    pp_decl_part(printer, decl_part->variables);
  }
  pp_indent(printer);
  pp_stmt(printer, decl_part->stmt);
  printf(";\n");
}

void pp_decl_part(printer_t *printer, const ast_decl_part_t *decl_part)
{
  const ast_decl_part_t *cur;
  assert(printer && decl_part);

  cur = decl_part;
  while (cur) {
    pp_indent(printer);
    switch (cur->kind) {
    case AST_DECL_PART_VARIABLE:
      pp_variable_decl_part(printer, (ast_decl_part_variable_t *) cur);
      break;
    case AST_DECL_PART_PROCEDURE:
      pp_procedure_decl_part(printer, (ast_decl_part_procedure_t *) cur);
      break;
    }
    if ((cur = cur->next)) {
      printf("\n");
    }
  }
}

void pp_program(printer_t *printer, const ast_program_t *program)
{
  assert(printer && program);
  pp_colored_keyword(printer, TOKEN_PROGRAM);
  printf(" ");
  pp_colored_program(printer, program->name);
  printf(";\n");
  if (program->decl_part) {
    printer->indent++;
    pp_decl_part(printer, program->decl_part);
    printer->indent--;
    printf("\n");
  }
  pp_stmt(printer, program->stmt);
  printf(".\n");
}

void pretty_print(const ast_t *ast)
{
  printer_t printer;
  assert(ast);
  printer.indent = 0;

  printer.color_scheme = &monokai;
  console_reset();
  pp_program(&printer, ast->program);
  console_reset();
}
