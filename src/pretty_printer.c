#include <assert.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "ast.h"
#include "lexer.h"

typedef struct {
  unsigned long foreground;
  unsigned long program;
  unsigned long keyword;
  unsigned long operator;
  unsigned long procedure;
  unsigned long argument;
  unsigned long string;
  unsigned long literal;
} color_scheme_t;

const color_scheme_t monokai = {
  0xc9d1d9, 0x66d9ef, 0xf92672, 0xf92672, 0xa6e22e, 0xfd971f, 0xe6db74, 0xae81ff
};

typedef struct {
  long                  indent;
  const color_scheme_t *colors;
} printer_t;

const char *pp_binary_operator_str(ast_expr_binary_kind_t kind)
{
  /* clang-format off */
  switch (kind) {
  case AST_EXPR_BINARY_KIND_STAR:  return "*";
  case AST_EXPR_BINARY_KIND_DIV:   return "div";
  case AST_EXPR_BINARY_KIND_AND:   return "and";
  case AST_EXPR_BINARY_KIND_PLUS:  return "+";
  case AST_EXPR_BINARY_KIND_MINUS: return "-";
  case AST_EXPR_BINARY_KIND_OR:    return "or";
  case AST_EXPR_BINARY_KIND_EQUAL: return "=";
  case AST_EXPR_BINARY_KIND_NOTEQ: return "<>";
  case AST_EXPR_BINARY_KIND_LE:    return "<";
  case AST_EXPR_BINARY_KIND_LEEQ:  return "<=";
  case AST_EXPR_BINARY_KIND_GR:    return ">";
  case AST_EXPR_BINARY_KIND_GREQ:  return ">=";

  default:
    unreachable();
  }
  /* clang-format on */
}

void pp_indent(printer_t *printer)
{
  long i;
  for (i = 0; i < printer->indent; ++i) {
    printf("    ");
  }
}

void pp_colored(unsigned long color, const char *format, va_list args)
{
  term_set(color);
  vprintf(format, args);
  term_set(SGR_RESET);
}

void pp_program_ident(printer_t *printer, const char *format, ...)
{
  va_list args;
  va_start(args, format);
  pp_colored(printer->colors->program, format, args);
  va_end(args);
}

void pp_keyword(printer_t *printer, const char *format, ...)
{
  va_list args;
  va_start(args, format);
  pp_colored(printer->colors->keyword, format, args);
  va_end(args);
}

void pp_operator(printer_t *printer, const char *format, ...)
{
  va_list args;
  va_start(args, format);
  pp_colored(printer->colors->operator, format, args);
  va_end(args);
}

void pp_procedure_ident(printer_t *printer, const char *format, ...)
{
  va_list args;
  va_start(args, format);
  pp_colored(printer->colors->procedure, format, args);
  va_end(args);
}

void pp_param_ident(printer_t *printer, const char *format, ...)
{
  va_list args;
  va_start(args, format);
  pp_colored(printer->colors->argument, format, args);
  va_end(args);
}

void pp_ident(printer_t *printer, const ast_ident_t *ident)
{
  printf("%.*s", (int) ident->symbol->len, ident->symbol->ptr);
  while ((ident = ident->next)) {
    printf(", %.*s", (int) ident->symbol->len, ident->symbol->ptr);
  }
}

void pp_lit(printer_t *printer, const ast_lit_t *lit)
{
  switch (lit->kind) {
  case AST_LIT_KIND_NUMBER: {
    ast_lit_number_t *number = (ast_lit_number_t *) lit;
    term_set(printer->colors->literal);
    printf("%.*s", (int) number->symbol->len, number->symbol->ptr);
    term_set(SGR_RESET);
    break;
  }
  case AST_LIT_KIND_BOOLEAN: {
    ast_lit_boolean_t *boolean = (ast_lit_boolean_t *) lit;
    term_set(printer->colors->literal);
    printf("%s", boolean->value ? "true" : "false");
    term_set(SGR_RESET);
    break;
  }
  case AST_LIT_KIND_STRING: {
    ast_lit_string_t *string = (ast_lit_string_t *) lit;
    term_set(printer->colors->string);
    printf("'");
    printf("%.*s", (int) string->symbol->len, string->symbol->ptr);
    printf("'");
    term_set(SGR_RESET);
    break;
  }
  }
}

void pp_type(printer_t *printer, const ast_type_t *type)
{
  switch (type->kind) {
  case AST_TYPE_KIND_INTEGER:
    pp_keyword(printer, "integer");
    break;
  case AST_TYPE_KIND_BOOLEAN:
    pp_keyword(printer, "boolean");
    break;
  case AST_TYPE_KIND_CHAR:
    pp_keyword(printer, "char");
    break;
  case AST_TYPE_KIND_ARRAY: {
    ast_type_array_t *array = (ast_type_array_t *) type;
    pp_keyword(printer, "array");
    printf("[");
    pp_lit(printer, array->size);
    printf("] ");
    pp_keyword(printer, "of");
    printf(" ");
    pp_type(printer, array->base);
    break;
  }
  }
}

void pp_expr(printer_t *printer, const ast_expr_t *expr)
{
  while (expr) {
    switch (expr->kind) {
    case AST_EXPR_KIND_BINARY: {
      ast_expr_binary_t *binary = (ast_expr_binary_t *) expr;
      pp_expr(printer, binary->lhs);
      if (binary->lhs->kind != AST_EXPR_KIND_EMPTY) {
        printf(" ");
      }
      pp_operator(printer, pp_binary_operator_str(binary->kind));
      if (binary->lhs->kind != AST_EXPR_KIND_EMPTY) {
        printf(" ");
      }
      pp_expr(printer, binary->rhs);
      break;
    }
    case AST_EXPR_KIND_NOT: {
      ast_expr_not_t *not = (ast_expr_not_t *) expr;
      pp_operator(printer, "not");
      printf(" ");
      pp_expr(printer, not ->expr);
      break;
    }
    case AST_EXPR_KIND_PAREN: {
      ast_expr_paren_t *paren = (ast_expr_paren_t *) expr;
      printf("(");
      pp_expr(printer, paren->inner);
      printf(")");
      break;
    }
    case AST_EXPR_KIND_CAST: {
      ast_expr_cast_t *cast = (ast_expr_cast_t *) expr;
      pp_type(printer, cast->type);
      printf("(");
      pp_expr(printer, cast->cast);
      printf(")");
      break;
    }
    case AST_EXPR_KIND_DECL_REF: {
      ast_expr_decl_ref_t *decl_ref = (ast_expr_decl_ref_t *) expr;
      pp_ident(printer, decl_ref->decl);
      break;
    }
    case AST_EXPR_KIND_ARRAY_SUBSCRIPT: {
      ast_expr_array_subscript_t *array_subscript = (ast_expr_array_subscript_t *) expr;
      pp_ident(printer, array_subscript->decl);
      printf("[");
      pp_expr(printer, array_subscript->subscript);
      printf("]");
      break;
    }
    case AST_EXPR_KIND_CONSTANT: {
      ast_expr_constant_t *constant = (ast_expr_constant_t *) expr;
      pp_lit(printer, constant->lit);
      break;
    }
    case AST_EXPR_KIND_EMPTY:
      /* do nothing */
      break;
    }
    if ((expr = expr->next)) {
      printf(", ");
    }
  }
}

void pp_stmt(printer_t *printer, const ast_stmt_t *stmt);

void pp_stmt_assign(printer_t *printer, const ast_stmt_assign_t *stmt)
{
  pp_expr(printer, stmt->lhs);
  printf(" ");
  pp_operator(printer, ":=");
  printf(" ");
  pp_expr(printer, stmt->rhs);
}

void pp_structured_stmt(printer_t *printer, const ast_stmt_t *stmt)
{
  if (stmt->kind != AST_STMT_KIND_EMPTY) {
    printf("\n");
  }
  if (stmt->kind != AST_STMT_KIND_COMPOUND) {
    ++printer->indent;
    pp_indent(printer);
    pp_stmt(printer, stmt);
    --printer->indent;
  } else {
    pp_indent(printer);
    pp_stmt(printer, stmt);
  }
}

void pp_stmt_if(printer_t *printer, const ast_stmt_if_t *stmt)
{
  pp_keyword(printer, "if");
  printf(" ");
  pp_expr(printer, stmt->cond);
  printf(" ");
  pp_keyword(printer, "then");
  pp_structured_stmt(printer, stmt->then_stmt);
  if (stmt->else_stmt) {
    printf("\n");
    pp_indent(printer);
    pp_keyword(printer, "else");
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
  pp_keyword(printer, "while");
  printf(" ");
  pp_expr(printer, stmt->cond);
  printf(" ");
  pp_keyword(printer, "do");
  pp_structured_stmt(printer, stmt->do_stmt);
}

void pp_stmt_call(printer_t *printer, const ast_stmt_call_t *stmt)
{
  pp_keyword(printer, "call");
  printf(" ");
  pp_procedure_ident(printer, "%.*s", (int) stmt->name->symbol->len, stmt->name->symbol->ptr);
  if (stmt->args) {
    printf("(");
    pp_expr(printer, stmt->args);
    printf(")");
  }
}

void pp_stmt_read(printer_t *printer, const ast_stmt_read_t *stmt)
{
  pp_procedure_ident(printer, stmt->newline ? "readln" : "read");
  if (stmt->args) {
    printf("(");
    pp_expr(printer, stmt->args);
    printf(")");
  }
}

void pp_stmt_write(printer_t *printer, const ast_stmt_write_t *stmt)
{
  pp_procedure_ident(printer, stmt->newline ? "writeln" : "write");
  if (stmt->formats) {
    ast_out_fmt_t *formats = stmt->formats;
    printf("(");
    while (formats) {
      if (formats != stmt->formats) {
        printf(", ");
      }
      pp_expr(printer, formats->expr);
      if (formats->len) {
        printf(" : ");
        pp_lit(printer, formats->len);
      }
      formats = formats->next;
    }
    printf(")");
  }
}

void pp_stmt_compound(printer_t *printer, const ast_stmt_compound_t *stmt)
{
  ast_stmt_t *stmts = stmt->stmts;

  pp_keyword(printer, "begin");
  if (stmts->next || stmts->kind != AST_STMT_KIND_EMPTY) {
    printf("\n");
  }
  ++printer->indent;
  while (stmts) {
    pp_indent(printer);
    pp_stmt(printer, stmts);
    if ((stmts = stmts->next)) {
      printf(";");
      if (!stmts->next && stmts->kind == AST_STMT_KIND_EMPTY) {
        break;
      }
      printf("\n");
    }
  }
  --printer->indent;
  printf("\n");
  pp_indent(printer);
  pp_keyword(printer, "end");
}

void pp_stmt(printer_t *printer, const ast_stmt_t *stmt)
{
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
    pp_keyword(printer, "break");
    break;
  case AST_STMT_KIND_CALL:
    pp_stmt_call(printer, (ast_stmt_call_t *) stmt);
    break;
  case AST_STMT_KIND_RETURN:
    pp_keyword(printer, "return");
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
    /* do nothing */
    break;
  }
}

void pp_decl_part(printer_t *printer, const ast_decl_part_t *decl_part);

void pp_decl_part_variable(printer_t *printer, const ast_decl_part_variable_t *decl_part)
{
  const ast_decl_variable_t *decls = decl_part->decls;

  pp_keyword(printer, "var");
  printf("\n");
  ++printer->indent;
  while (decls) {
    pp_indent(printer);
    pp_ident(printer, decls->names);
    printf(": ");
    pp_type(printer, decls->type);
    printf(";\n");
    decls = decls->next;
  }
  --printer->indent;
}

void pp_decl_part_procedure(printer_t *printer, const ast_decl_part_procedure_t *decl_part)
{
  pp_keyword(printer, "procedure");
  printf(" ");
  pp_procedure_ident(printer, "%.*s", decl_part->name->symbol->len, decl_part->name->symbol->ptr);

  if (decl_part->params) {
    const ast_decl_param_t *params = decl_part->params;
    printf("(");
    while (params) {
      const ast_ident_t *ident = params->names;
      while (ident) {
        pp_param_ident(printer, "%.*s", (int) ident->symbol->len, ident->symbol->ptr);
        if ((ident = ident->next)) {
          printf(", ");
        }
      }
      printf(": ");
      pp_type(printer, params->type);

      if ((params = params->next)) {
        printf("; ");
      }
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
  while (decl_part) {
    pp_indent(printer);
    switch (decl_part->kind) {
    case AST_DECL_PART_VARIABLE:
      pp_decl_part_variable(printer, (ast_decl_part_variable_t *) decl_part);
      break;
    case AST_DECL_PART_PROCEDURE:
      pp_decl_part_procedure(printer, (ast_decl_part_procedure_t *) decl_part);
      break;
    }
    if ((decl_part = decl_part->next)) {
      printf("\n");
    }
  }
}

void pp_program(printer_t *printer, const ast_program_t *program)
{
  pp_keyword(printer, "program");
  printf(" ");
  pp_program_ident(printer, "%.*s", (int) program->name->symbol->len, program->name->symbol->ptr);
  printf(";\n");
  if (program->decl_part) {
    ++printer->indent;
    pp_decl_part(printer, program->decl_part);
    --printer->indent;
    printf("\n");
  }
  pp_stmt(printer, program->stmt);
  printf(".\n");
}

void pretty_print(const ast_t *ast)
{
  if (ast) {
    printer_t printer;
    printer.indent = 0;
    printer.colors = &monokai;

    term_set(SGR_RESET);
    pp_program(&printer, ast->program);
    term_set(SGR_RESET);
  }
}
