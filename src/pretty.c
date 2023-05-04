#include <assert.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>

#include "ast.h"
#include "context.h"
#include "lexer.h"

#if defined(__GNUC__)
#define pp_check_format(style, fmt_idx, arg_idx) __attribute__((format(style, fmt_idx, arg_idx)))
#else
#define pp_check_format
#endif

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

const color_scheme_t github = {
  0xE6EDF3, 0xD2A8FF, 0xFF7B72, 0xFF7B72, 0xD2A8FF, 0xFFA657, 0xA5D6FF, 0x79C0FF
};

typedef struct {
  long                  indent;
  const color_scheme_t *colors;
  FILE                 *stream;
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

void pp_vfprintf(printer_t *printer, const char *format, va_list args)
{
  vfprintf(printer->stream, format, args);
}

void pp_check_format(printf, 2, 3) pp_fprintf(printer_t *printer, const char *format, ...)
{
  va_list args;
  va_start(args, format);
  pp_vfprintf(printer, format, args);
  va_end(args);
}

void pp_indent(printer_t *printer)
{
  long i;
  for (i = 0; i < printer->indent; ++i) {
    pp_fprintf(printer, "    ");
  }
}

#define define_colored_fprintf(name, color)                                            \
  void pp_check_format(printf, 2, 3) name(printer_t *printer, const char *format, ...) \
  {                                                                                    \
    va_list args;                                                                      \
    va_start(args, format);                                                            \
    term_set(printer->colors->color);                                                  \
    pp_vfprintf(printer, format, args);                                                \
    term_set(SGR_RESET);                                                               \
    va_end(args);                                                                      \
  }

/* clang-format off */
define_colored_fprintf(pp_keyword, keyword)
define_colored_fprintf(pp_operator, operator)
define_colored_fprintf(pp_ident_program, program)
define_colored_fprintf(pp_ident_procedure, procedure)
define_colored_fprintf(pp_ident_param, argument)
/* clang-format on */

#undef define_colored_fprintf

          void pp_lit(printer_t *printer, const ast_lit_t *lit)
{
  switch (lit->kind) {
  case AST_LIT_KIND_NUMBER: {
    ast_lit_number_t *number = (ast_lit_number_t *) lit;
    term_set(printer->colors->literal);
    pp_fprintf(printer, "%s", number->symbol->ptr);
    term_set(SGR_RESET);
    break;
  }
  case AST_LIT_KIND_BOOLEAN: {
    ast_lit_boolean_t *boolean = (ast_lit_boolean_t *) lit;
    term_set(printer->colors->literal);
    pp_fprintf(printer, "%s", boolean->value ? "true" : "false");
    term_set(SGR_RESET);
    break;
  }
  case AST_LIT_KIND_STRING: {
    ast_lit_string_t *string = (ast_lit_string_t *) lit;
    term_set(printer->colors->string);
    pp_fprintf(printer, "'%s'", string->symbol->ptr);
    term_set(SGR_RESET);
    break;
  }
  }
}

void pp_ident(printer_t *printer, const ast_ident_t *ident)
{
  pp_fprintf(printer, "%s", ident->symbol->ptr);
  while ((ident = ident->next)) {
    pp_fprintf(printer, ", %s", ident->symbol->ptr);
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
    pp_fprintf(printer, "[");
    pp_lit(printer, array->size);
    pp_fprintf(printer, "] ");
    pp_keyword(printer, "of");
    pp_fprintf(printer, " ");
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
        pp_fprintf(printer, " ");
      }
      pp_operator(printer, "%s", pp_binary_operator_str(binary->kind));
      if (binary->lhs->kind != AST_EXPR_KIND_EMPTY) {
        pp_fprintf(printer, " ");
      }
      pp_expr(printer, binary->rhs);
      break;
    }
    case AST_EXPR_KIND_NOT: {
      ast_expr_not_t *not = (ast_expr_not_t *) expr;
      pp_operator(printer, "not");
      pp_fprintf(printer, " ");
      pp_expr(printer, not ->expr);
      break;
    }
    case AST_EXPR_KIND_PAREN: {
      ast_expr_paren_t *paren = (ast_expr_paren_t *) expr;
      pp_fprintf(printer, "(");
      pp_expr(printer, paren->inner);
      pp_fprintf(printer, ")");
      break;
    }
    case AST_EXPR_KIND_CAST: {
      ast_expr_cast_t *cast = (ast_expr_cast_t *) expr;
      pp_type(printer, cast->type);
      pp_fprintf(printer, "(");
      pp_expr(printer, cast->cast);
      pp_fprintf(printer, ")");
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
      pp_fprintf(printer, "[");
      pp_expr(printer, array_subscript->subscript);
      pp_fprintf(printer, "]");
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
      pp_fprintf(printer, ", ");
    }
  }
}

void pp_stmt(printer_t *printer, const ast_stmt_t *stmt);

void pp_stmt_assign(printer_t *printer, const ast_stmt_assign_t *stmt)
{
  pp_expr(printer, stmt->lhs);
  pp_fprintf(printer, " ");
  pp_operator(printer, ":=");
  pp_fprintf(printer, " ");
  pp_expr(printer, stmt->rhs);
}

void pp_structured_stmt(printer_t *printer, const ast_stmt_t *stmt)
{
  if (stmt->kind != AST_STMT_KIND_EMPTY) {
    pp_fprintf(printer, "\n");
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
  pp_fprintf(printer, " ");
  pp_expr(printer, stmt->cond);
  pp_fprintf(printer, " ");
  pp_keyword(printer, "then");
  pp_structured_stmt(printer, stmt->then_stmt);
  if (stmt->else_stmt) {
    pp_fprintf(printer, "\n");
    pp_indent(printer);
    pp_keyword(printer, "else");
    if (stmt->else_stmt->kind == AST_STMT_KIND_IF) {
      pp_fprintf(printer, " ");
      pp_stmt(printer, stmt->else_stmt);
    } else {
      pp_structured_stmt(printer, stmt->else_stmt);
    }
  }
}

void pp_stmt_while(printer_t *printer, const ast_stmt_while_t *stmt)
{
  pp_keyword(printer, "while");
  pp_fprintf(printer, " ");
  pp_expr(printer, stmt->cond);
  pp_fprintf(printer, " ");
  pp_keyword(printer, "do");
  pp_structured_stmt(printer, stmt->do_stmt);
}

void pp_stmt_call(printer_t *printer, const ast_stmt_call_t *stmt)
{
  pp_keyword(printer, "call");
  pp_fprintf(printer, " ");
  pp_ident_procedure(printer, "%s", stmt->name->symbol->ptr);
  if (stmt->args) {
    pp_fprintf(printer, "(");
    pp_expr(printer, stmt->args);
    pp_fprintf(printer, ")");
  }
}

void pp_stmt_read(printer_t *printer, const ast_stmt_read_t *stmt)
{
  pp_ident_procedure(printer, stmt->newline ? "readln" : "read");
  if (stmt->args) {
    pp_fprintf(printer, "(");
    pp_expr(printer, stmt->args);
    pp_fprintf(printer, ")");
  }
}

void pp_stmt_write(printer_t *printer, const ast_stmt_write_t *stmt)
{
  pp_ident_procedure(printer, stmt->newline ? "writeln" : "write");
  if (stmt->formats) {
    ast_out_fmt_t *formats = stmt->formats;
    pp_fprintf(printer, "(");
    while (formats) {
      pp_expr(printer, formats->expr);
      if (formats->len) {
        pp_fprintf(printer, " : ");
        pp_lit(printer, formats->len);
      }
      if ((formats = formats->next)) {
        pp_fprintf(printer, ", ");
      }
    }
    pp_fprintf(printer, ")");
  }
}

void pp_stmt_compound(printer_t *printer, const ast_stmt_compound_t *stmt)
{
  ast_stmt_t *stmts = stmt->stmts;

  pp_keyword(printer, "begin");
  if (stmts->next || stmts->kind != AST_STMT_KIND_EMPTY) {
    pp_fprintf(printer, "\n");
  }
  ++printer->indent;
  while (stmts) {
    pp_indent(printer);
    pp_stmt(printer, stmts);
    if ((stmts = stmts->next)) {
      pp_fprintf(printer, ";");
      if (!stmts->next && stmts->kind == AST_STMT_KIND_EMPTY) {
        break;
      }
      pp_fprintf(printer, "\n");
    }
  }
  --printer->indent;
  pp_fprintf(printer, "\n");
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

void pp_decl_part(printer_t *printer, const ast_decl_part_t *decl_part)
{
  while (decl_part) {
    pp_indent(printer);
    switch (decl_part->kind) {
    case AST_DECL_PART_VARIABLE: {
      ast_decl_part_variable_t *variable = (ast_decl_part_variable_t *) decl_part;
      ast_decl_variable_t      *decls    = variable->decls;

      pp_keyword(printer, "var");
      pp_fprintf(printer, "\n");
      ++printer->indent;
      while (decls) {
        pp_indent(printer);
        pp_ident(printer, decls->names);
        pp_fprintf(printer, ": ");
        pp_type(printer, decls->type);
        pp_fprintf(printer, ";\n");
        decls = decls->next;
      }
      --printer->indent;
      break;
    }
    case AST_DECL_PART_PROCEDURE: {
      ast_decl_part_procedure_t *procedure = (ast_decl_part_procedure_t *) decl_part;
      pp_keyword(printer, "procedure");
      pp_fprintf(printer, " ");
      pp_ident_procedure(printer, "%s", procedure->name->symbol->ptr);
      if (procedure->params) {
        ast_decl_param_t *params = procedure->params;
        pp_fprintf(printer, "(");
        while (params) {
          ast_ident_t *ident = params->names;
          while (ident) {
            pp_ident_param(printer, "%s", ident->symbol->ptr);
            if ((ident = ident->next)) {
              pp_fprintf(printer, ", ");
            }
          }
          pp_fprintf(printer, ": ");
          pp_type(printer, params->type);
          if ((params = params->next)) {
            pp_fprintf(printer, "; ");
          }
        }
        pp_fprintf(printer, ")");
      }
      pp_fprintf(printer, ";\n");
      if (procedure->variables) {
        pp_decl_part(printer, procedure->variables);
      }
      pp_indent(printer);
      pp_stmt(printer, procedure->stmt);
      pp_fprintf(printer, ";\n");
      break;
    }
    }
    if ((decl_part = decl_part->next)) {
      pp_fprintf(printer, "\n");
    }
  }
}

void pp_program(printer_t *printer, const ast_program_t *program)
{
  pp_keyword(printer, "program");
  pp_fprintf(printer, " ");
  pp_ident_program(printer, "%s", program->name->symbol->ptr);
  pp_fprintf(printer, ";\n");
  if (program->decl_part) {
    ++printer->indent;
    pp_decl_part(printer, program->decl_part);
    --printer->indent;
    pp_fprintf(printer, "\n");
  }
  pp_stmt(printer, program->stmt);
  pp_fprintf(printer, ".\n");
}

void pretty(context_t *ctx)
{
  printer_t printer;
  printer.indent = 0;
  printer.colors = &github;
  printer.stream = stdout;

  term_set(SGR_RESET);
  pp_program(&printer, ctx->ast->program);
  term_set(SGR_RESET);
}
