#include <stddef.h>
#include <stdio.h>

#include "mppl_syntax.h"
#include "pretty_printer.h"
#include "syntax_tree.h"
#include "token_tree.h"

typedef struct Printer Printer;

struct Printer {
  unsigned long indent;
  PrinterOption option;
};

static void print_space(void);
static void print_newline(void);
static void print_indent(Printer *printer);
static void print_program(Printer *printer, MpplProgram *program);
static void print_decl_part(Printer *printer, AnyMpplDeclPart *part);
static void print_var_decl_part(Printer *printer, MpplVarDeclPart *part);
static void print_var_decl(Printer *printer, MpplVarDecl *decl);
static void print_proc_decl(Printer *printer, MpplProcDecl *decl);
static void print_fml_param_list(Printer *printer, MpplFmlParamList *list);
static void print_fml_param_sec(Printer *printer, MpplFmlParamSec *sec);
static void print_stmt(Printer *printer, AnyMpplStmt *stmt);
static void print_assign_stmt(Printer *printer, MpplAssignStmt *stmt);
static void print_if_stmt(Printer *printer, MpplIfStmt *stmt);
static void print_while_stmt(Printer *printer, MpplWhileStmt *stmt);
static void print_break_stmt(Printer *printer, MpplBreakStmt *stmt);
static void print_call_stmt(Printer *printer, MpplCallStmt *stmt);
static void print_return_stmt(Printer *printer, MpplReturnStmt *stmt);
static void print_input_stmt(Printer *printer, MpplInputStmt *stmt);
static void print_output_stmt(Printer *printer, MpplOutputStmt *stmt);
static void print_comp_stmt(Printer *printer, MpplCompStmt *stmt);
static void print_act_param_list(Printer *printer, MpplActParamList *list);
static void print_input_list(Printer *printer, MpplInputList *list);
static void print_expr(Printer *printer, AnyMpplExpr *expr);
static void print_binary_expr(Printer *printer, MpplBinaryExpr *expr);
static void print_paren_expr(Printer *printer, MpplParenExpr *expr);
static void print_not_expr(Printer *printer, MpplNotExpr *expr);
static void print_cast_expr(Printer *printer, MpplCastExpr *expr);
static void print_var(Printer *printer, AnyMpplVar *var);
static void print_entire_var(Printer *printer, MpplEntireVar *var);
static void print_indexed_var(Printer *printer, MpplIndexedVar *var);
static void print_type(Printer *printer, AnyMpplType *type);
static void print_std_type(Printer *printer, AnyMpplStdType *type);
static void print_array_type(Printer *printer, MpplArrayType *type);
static void print_output_list(Printer *printer, MpplOutputList *list);
static void print_output_value(Printer *printer, MpplOutputValue *value);
static void print_literal(Printer *printer, AnyMpplLit *lit);
static void print_token(Printer *printer, MpplToken *token);
static void print_token_program(Printer *printer, MpplToken *token);
static void print_token_keyword(Printer *printer, MpplToken *token);
static void print_token_operator(Printer *printer, MpplToken *token);
static void print_token_proc(Printer *printer, MpplToken *token);
static void print_token_param(Printer *printer, MpplToken *token);
static void print_token_string(Printer *printer, MpplToken *token);
static void print_token_literal(Printer *printer, MpplToken *token);

static void print_space(void)
{
  printf(" ");
}

static void print_newline(void)
{
  printf("\n");
}

static void print_indent(Printer *printer)
{
  printf("%*.s", (int) printer->indent * 4, "");
}

static void print_program(Printer *printer, MpplProgram *program)
{
  unsigned long i;
  print_token_keyword(printer, mppl_program__program_token(program));
  print_space();
  print_token_program(printer, mppl_program__name(program));
  print_token(printer, mppl_program__semi_token(program));
  print_newline();
  ++printer->indent;
  for (i = 0; i < mppl_program__decl_part_count(program); ++i) {
    print_indent(printer);
    print_decl_part(printer, mppl_program__decl_part(program, i));
  }
  --printer->indent;
  print_indent(printer);
  print_comp_stmt(printer, mppl_program__comp_stmt(program));
  print_token(printer, mppl_program__dot_token(program));
  print_newline();
  mppl_free(program);
}

static void print_decl_part(Printer *printer, AnyMpplDeclPart *part)
{
  switch (mppl_decl_part__kind(part)) {
  case MPPL_DECL_PART_VAR:
    print_var_decl_part(printer, (MpplVarDeclPart *) part);
    break;
  case MPPL_DECL_PART_PROC:
    print_proc_decl(printer, (MpplProcDecl *) part);
    break;
  }
}

static void print_var_decl_part(Printer *printer, MpplVarDeclPart *part)
{
  unsigned long i;
  print_token_keyword(printer, mppl_var_decl_part__var_token(part));
  print_newline();
  ++printer->indent;
  for (i = 0; i < mppl_var_decl_part__var_decl_count(part); ++i) {
    print_indent(printer);
    print_var_decl(printer, mppl_var_decl_part__var_decl(part, i));
    print_token(printer, mppl_var_decl_part__semi_token(part, i));
    print_newline();
  }
  --printer->indent;
  mppl_free(part);
}

static void print_var_decl(Printer *printer, MpplVarDecl *decl)
{
  unsigned long i;
  for (i = 0; i < mppl_var_decl__name_count(decl); ++i) {
    print_token(printer, mppl_var_decl__name(decl, i));
    if (i + 1 < mppl_var_decl__name_count(decl)) {
      print_token(printer, mppl_var_decl__comma_token(decl, i));
      print_space();
    }
  }
  print_space();
  print_token(printer, mppl_var_decl__colon_token(decl));
  print_space();
  print_type(printer, mppl_var_decl__type(decl));
  mppl_free(decl);
}

static void print_proc_decl(Printer *printer, MpplProcDecl *decl)
{
  MpplFmlParamList *list;
  MpplVarDeclPart  *part;
  print_token_keyword(printer, mppl_proc_decl__procedure_token(decl));
  print_space();
  print_token_proc(printer, mppl_proc_decl__name(decl));
  if ((list = mppl_proc_decl__fml_param_list(decl))) {
    print_fml_param_list(printer, list);
  }
  print_token(printer, mppl_proc_decl__semi_token_0(decl));
  print_newline();
  if ((part = mppl_proc_decl__var_decl_part(decl))) {
    print_indent(printer);
    print_var_decl_part(printer, part);
  }
  print_indent(printer);
  print_comp_stmt(printer, mppl_proc_decl__comp_stmt(decl));
  print_token(printer, mppl_proc_decl__semi_token_1(decl));
  print_newline();
  mppl_free(decl);
}

static void print_fml_param_list(Printer *printer, MpplFmlParamList *list)
{
  unsigned long i;
  print_token(printer, mppl_fml_param_list__lparen_token(list));
  for (i = 0; i < mppl_fml_param_list__fml_param_sec_count(list); ++i) {
    print_fml_param_sec(printer, mppl_fml_param_list__fml_param_sec(list, i));
    if (i + 1 < mppl_fml_param_list__fml_param_sec_count(list)) {
      print_token(printer, mppl_fml_param_list__semi_token(list, i));
      print_space();
    }
  }
  print_token(printer, mppl_fml_param_list__rparen_token(list));
  mppl_free(list);
}

static void print_fml_param_sec(Printer *printer, MpplFmlParamSec *sec)
{
  unsigned long i;
  for (i = 0; i < mppl_fml_param_sec__name_count(sec); ++i) {
    print_token_param(printer, mppl_fml_param_sec__name(sec, i));
    if (i + 1 < mppl_fml_param_sec__name_count(sec)) {
      print_token(printer, mppl_fml_param_sec__comma_token(sec, i));
      print_space();
    }
  }
  print_space();
  print_token(printer, mppl_fml_param_sec__colon_token(sec));
  print_space();
  print_type(printer, mppl_fml_param_sec__type(sec));
  mppl_free(sec);
}

static void print_stmt(Printer *printer, AnyMpplStmt *stmt)
{
  switch (mppl_stmt__kind(stmt)) {
  case MPPL_STMT_ASSIGN:
    print_assign_stmt(printer, (MpplAssignStmt *) stmt);
    break;
  case MPPL_STMT_IF:
    print_if_stmt(printer, (MpplIfStmt *) stmt);
    break;
  case MPPL_STMT_WHILE:
    print_while_stmt(printer, (MpplWhileStmt *) stmt);
    break;
  case MPPL_STMT_BREAK:
    print_break_stmt(printer, (MpplBreakStmt *) stmt);
    break;
  case MPPL_STMT_CALL:
    print_call_stmt(printer, (MpplCallStmt *) stmt);
    break;
  case MPPL_STMT_RETURN:
    print_return_stmt(printer, (MpplReturnStmt *) stmt);
    break;
  case MPPL_STMT_INPUT:
    print_input_stmt(printer, (MpplInputStmt *) stmt);
    break;
  case MPPL_STMT_OUTPUT:
    print_output_stmt(printer, (MpplOutputStmt *) stmt);
    break;
  case MPPL_STMT_COMP:
    print_comp_stmt(printer, (MpplCompStmt *) stmt);
    break;
  }
}

static void print_stmt__conditional(Printer *printer, AnyMpplStmt *stmt)
{
  int is_comp = mppl_stmt__kind(stmt) == MPPL_STMT_COMP;
  if (!is_comp) {
    ++printer->indent;
    print_newline();
    print_indent(printer);
  } else {
    print_space();
  }
  print_stmt(printer, stmt);
  if (!is_comp) {
    --printer->indent;
  }
}

static void print_assign_stmt(Printer *printer, MpplAssignStmt *stmt)
{
  print_var(printer, mppl_assign_stmt__var(stmt));
  print_space();
  print_token_operator(printer, mppl_assign_stmt__assign_token(stmt));
  print_space();
  print_expr(printer, mppl_assign_stmt__expr(stmt));
  mppl_free(stmt);
}

static void print_if_stmt(Printer *printer, MpplIfStmt *stmt)
{
  MpplToken *else_token;
  print_token_keyword(printer, mppl_if_stmt__if_token(stmt));
  print_space();
  print_expr(printer, mppl_if_stmt__expr(stmt));
  print_space();
  print_token_keyword(printer, mppl_if_stmt__then_token(stmt));
  print_stmt__conditional(printer, mppl_if_stmt__then_stmt(stmt));
  if ((else_token = mppl_if_stmt__else_token(stmt))) {
    AnyMpplStmt *else_stmt = mppl_if_stmt__else_stmt(stmt);
    print_newline();
    print_indent(printer);
    print_token_keyword(printer, else_token);
    if (mppl_stmt__kind(else_stmt) == MPPL_STMT_IF) {
      print_space();
      print_stmt(printer, else_stmt);
    } else {
      print_stmt__conditional(printer, else_stmt);
    }
  }
  mppl_free(stmt);
}

static void print_while_stmt(Printer *printer, MpplWhileStmt *stmt)
{
  print_token_keyword(printer, mppl_while_stmt__while_token(stmt));
  print_space();
  print_expr(printer, mppl_while_stmt__expr(stmt));
  print_space();
  print_token_keyword(printer, mppl_while_stmt__do_token(stmt));
  print_stmt__conditional(printer, mppl_while_stmt__stmt(stmt));
  mppl_free(stmt);
}

static void print_break_stmt(Printer *printer, MpplBreakStmt *stmt)
{
  print_token_keyword(printer, mppl_break_stmt__break_token(stmt));
  mppl_free(stmt);
}

static void print_call_stmt(Printer *printer, MpplCallStmt *stmt)
{
  MpplActParamList *list;
  print_token_keyword(printer, mppl_call_stmt__call_token(stmt));
  print_space();
  print_token_proc(printer, mppl_call_stmt__name(stmt));
  if ((list = mppl_call_stmt__act_param_list(stmt))) {
    print_act_param_list(printer, list);
  }
  mppl_free(stmt);
}

static void print_return_stmt(Printer *printer, MpplReturnStmt *stmt)
{
  print_token_keyword(printer, mppl_return_stmt__return_token(stmt));
  mppl_free(stmt);
}

static void print_input_stmt(Printer *printer, MpplInputStmt *stmt)
{
  MpplInputList *list;
  print_token_keyword(printer, mppl_input_stmt__read_token(stmt));
  if ((list = mppl_input_stmt__input_list(stmt))) {
    print_input_list(printer, list);
  }
  mppl_free(stmt);
}

static void print_output_stmt(Printer *printer, MpplOutputStmt *stmt)
{
  MpplOutputList *list;
  print_token_keyword(printer, mppl_output_stmt__write_token(stmt));
  if ((list = mppl_output_stmt__output_list(stmt))) {
    print_output_list(printer, list);
  }
  mppl_free(stmt);
}

static void print_comp_stmt(Printer *printer, MpplCompStmt *stmt)
{
  unsigned long i;
  print_token_keyword(printer, mppl_comp_stmt__begin_token(stmt));
  ++printer->indent;
  for (i = 0; i < mppl_comp_stmt__stmt_count(stmt); ++i) {
    AnyMpplStmt *inner_stmt = mppl_comp_stmt__stmt(stmt, i);
    if (i + 1 < mppl_comp_stmt__stmt_count(stmt)) {
      print_newline();
      print_indent(printer);
      print_stmt(printer, inner_stmt);
      print_token(printer, mppl_comp_stmt__semi_token(stmt, i));
    } else if (inner_stmt) {
      print_newline();
      print_indent(printer);
      print_stmt(printer, inner_stmt);
    }
  }
  --printer->indent;
  print_newline();
  print_indent(printer);
  print_token_keyword(printer, mppl_comp_stmt__end_token(stmt));
  mppl_free(stmt);
}

static void print_act_param_list(Printer *printer, MpplActParamList *list)
{
  unsigned long i;
  print_token(printer, mppl_act_param_list__lparen_token(list));
  for (i = 0; i < mppl_act_param_list__expr_count(list); ++i) {
    print_expr(printer, mppl_act_param_list__expr(list, i));
    if (i + 1 < mppl_act_param_list__expr_count(list)) {
      print_token(printer, mppl_act_param_list__comma_token(list, i));
      print_space();
    }
  }
  print_token(printer, mppl_act_param_list__rparen_token(list));
  mppl_free(list);
}

static void print_input_list(Printer *printer, MpplInputList *list)
{
  unsigned long i;
  print_token(printer, mppl_input_list__lparen_token(list));
  for (i = 0; i < mppl_input_list__var_count(list); ++i) {
    print_var(printer, mppl_input_list__var(list, i));
    if (i + 1 < mppl_input_list__var_count(list)) {
      print_token(printer, mppl_input_list__comma_token(list, i));
      print_space();
    }
  }
  print_token(printer, mppl_input_list__rparen_token(list));
  mppl_free(list);
}

static void print_expr(Printer *printer, AnyMpplExpr *expr)
{
  switch (mppl_expr__kind(expr)) {
  case MPPL_EXPR_BINARY:
    print_binary_expr(printer, (MpplBinaryExpr *) expr);
    break;
  case MPPL_EXPR_PAREN:
    print_paren_expr(printer, (MpplParenExpr *) expr);
    break;
  case MPPL_EXPR_NOT:
    print_not_expr(printer, (MpplNotExpr *) expr);
    break;
  case MPPL_EXPR_CAST:
    print_cast_expr(printer, (MpplCastExpr *) expr);
    break;
  case MPPL_EXPR_VAR:
    print_var(printer, (AnyMpplVar *) expr);
    break;
  case MPPL_EXPR_LIT:
    print_literal(printer, (AnyMpplLit *) expr);
    break;
  }
}

static void print_binary_expr(Printer *printer, MpplBinaryExpr *expr)
{
  AnyMpplExpr *lhs_expr = mppl_binary_expr__lhs(expr);
  if (lhs_expr) {
    print_expr(printer, lhs_expr);
    print_space();
    print_token_operator(printer, mppl_binary_expr__op_token(expr));
    print_space();
    print_expr(printer, mppl_binary_expr__rhs(expr));
  } else {
    print_token_operator(printer, mppl_binary_expr__op_token(expr));
    print_expr(printer, mppl_binary_expr__rhs(expr));
  }
  mppl_free(expr);
}

static void print_paren_expr(Printer *printer, MpplParenExpr *expr)
{
  print_token(printer, mppl_paren_expr__lparen_token(expr));
  print_expr(printer, mppl_paren_expr__expr(expr));
  print_token(printer, mppl_paren_expr__rparen_token(expr));
  mppl_free(expr);
}

static void print_not_expr(Printer *printer, MpplNotExpr *expr)
{
  print_token(printer, mppl_not_expr__not_token(expr));
  print_space();
  print_expr(printer, mppl_not_expr__expr(expr));
  mppl_free(expr);
}

static void print_cast_expr(Printer *printer, MpplCastExpr *expr)
{
  print_type(printer, mppl_cast_expr__type(expr));
  print_token(printer, mppl_cast_expr__lparen_token(expr));
  print_expr(printer, mppl_cast_expr__expr(expr));
  print_token(printer, mppl_cast_expr__rparen_token(expr));
  mppl_free(expr);
}

static void print_var(Printer *printer, AnyMpplVar *var)
{
  switch (mppl_var__kind(var)) {
  case MPPL_VAR_ENTIRE:
    print_entire_var(printer, (MpplEntireVar *) var);
    break;
  case MPPL_VAR_INDEXED:
    print_indexed_var(printer, (MpplIndexedVar *) var);
    break;
  }
}

static void print_entire_var(Printer *printer, MpplEntireVar *var)
{
  print_token(printer, mppl_entire_var__name(var));
  mppl_free(var);
}

static void print_indexed_var(Printer *printer, MpplIndexedVar *var)
{
  print_token(printer, mppl_indexed_var__name(var));
  print_token(printer, mppl_indexed_var__lbracket_token(var));
  print_expr(printer, mppl_indexed_var__expr(var));
  print_token(printer, mppl_indexed_var__rbracket_token(var));
  mppl_free(var);
}

static void print_type(Printer *printer, AnyMpplType *type)
{
  switch (mppl_type__kind(type)) {
  case MPPL_TYPE_STD:
    print_std_type(printer, (AnyMpplStdType *) type);
    break;
  case MPPL_TYPE_ARRAY:
    print_array_type(printer, (MpplArrayType *) type);
    break;
  }
}

static void print_std_type(Printer *printer, AnyMpplStdType *type)
{
  print_token_keyword(printer, (MpplToken *) type);
}

static void print_array_type(Printer *printer, MpplArrayType *type)
{
  print_token_keyword(printer, mppl_array_type__array_token(type));
  print_token(printer, mppl_array_type__lbracket_token(type));
  print_literal(printer, (AnyMpplLit *) mppl_array_type__size(type));
  print_token(printer, mppl_array_type__rbracket_token(type));
  print_space();
  print_token_keyword(printer, mppl_array_type__of_token(type));
  print_space();
  print_std_type(printer, mppl_array_type__std_type(type));
  mppl_free(type);
}

static void print_output_list(Printer *printer, MpplOutputList *list)
{
  unsigned long i;
  print_token(printer, mppl_output_list__lparen_token(list));
  for (i = 0; i < mppl_output_list__output_value_count(list); ++i) {
    print_output_value(printer, mppl_output_list__output_value(list, i));
    if (i + 1 < mppl_output_list__output_value_count(list)) {
      print_token(printer, mppl_output_list__comma_token(list, i));
      print_space();
    }
  }
  print_token(printer, mppl_output_list__rparen_token(list));
  mppl_free(list);
}

static void print_output_value(Printer *printer, MpplOutputValue *value)
{
  MpplToken *token;
  print_expr(printer, mppl_output_value__expr(value));
  if ((token = mppl_output_value__colon_token(value))) {
    print_space();
    print_token(printer, token);
    print_space();
    print_literal(printer, (AnyMpplLit *) mppl_output_value__width(value));
  }
  mppl_free(value);
}

static void print_literal(Printer *printer, AnyMpplLit *lit)
{
  switch (mppl_lit__kind(lit)) {
  case MPPL_LIT_STRING:
    print_token_string(printer, (MpplToken *) lit);
    break;
  case MPPL_LIT_NUMBER:
    print_token_literal(printer, (MpplToken *) lit);
    break;
  case MPPL_LIT_BOOLEAN:
    print_token_literal(printer, (MpplToken *) lit);
    break;
  }
}

static void print_token__impl(Printer *printer, MpplToken *token, unsigned long color)
{
  const SyntaxTree *tree      = (SyntaxTree *) token;
  const Token      *raw_token = (Token *) syntax_tree_raw(tree);
  if (printer->option.color.enabled) {
    printf("\033[38;2;%lu;%lu;%lum", color >> 16, (color >> 8) & 0xFF, color & 0xFF);
  }
  printf("%s", raw_token->text);
  if (printer->option.color.enabled) {
    printf("\033[0m");
  }
  fflush(stdout);
  mppl_free(token);
}

static void print_token(Printer *printer, MpplToken *token)
{
  print_token__impl(printer, token, printer->option.color.foreground);
}

static void print_token_program(Printer *printer, MpplToken *token)
{
  print_token__impl(printer, token, printer->option.color.program);
}

static void print_token_keyword(Printer *printer, MpplToken *token)
{
  print_token__impl(printer, token, printer->option.color.keyword);
}

static void print_token_operator(Printer *printer, MpplToken *token)
{
  print_token__impl(printer, token, printer->option.color.operator);
}

static void print_token_proc(Printer *printer, MpplToken *token)
{
  print_token__impl(printer, token, printer->option.color.procedure);
}

static void print_token_param(Printer *printer, MpplToken *token)
{
  print_token__impl(printer, token, printer->option.color.parameter);
}

static void print_token_string(Printer *printer, MpplToken *token)
{
  print_token__impl(printer, token, printer->option.color.string);
}

static void print_token_literal(Printer *printer, MpplToken *token)
{
  print_token__impl(printer, token, printer->option.color.literal);
}

void mppl_pretty_print(const TokenNode *node, const PrinterOption *option)
{
  Printer printer;
  printer.indent = 0;
  if (option) {
    printer.option = *option;
  } else {
    /* clang-format off */
    printer.option.color.enabled    = 1;
    printer.option.color.foreground = 0xE6EDF3;
    printer.option.color.program    = 0xD2A8FF;
    printer.option.color.keyword    = 0xFF7B72;
    printer.option.color.operator   = 0xFF7B72;
    printer.option.color.procedure  = 0xD2A8FF;
    printer.option.color.parameter  = 0xFFA657;
    printer.option.color.string     = 0xA5D6FF;
    printer.option.color.literal    = 0x79C0FF;
    /* clang-format on */
  }

  print_program(&printer, (MpplProgram *) syntax_tree_root((const TokenTree *) node));
}
