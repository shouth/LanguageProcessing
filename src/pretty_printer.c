#include <stddef.h>
#include <stdio.h>

#include "mppl_syntax.h"
#include "pretty_printer.h"
#include "string.h"
#include "syntax_tree.h"

typedef struct Printer Printer;

struct Printer {
  unsigned long indent;
  PrinterOption option;
};

static void print_space(void);
static void print_newline(void);
static void print_indent(Printer *printer);
static void print_program(Printer *printer, const MpplProgram *syntax);
static void print_decl_part(Printer *printer, const AnyMpplDeclPart *syntax);
static void print_var_decl_part(Printer *printer, const MpplVarDeclPart *syntax);
static void print_var_decl(Printer *printer, const MpplVarDecl *syntax);
static void print_proc_decl(Printer *printer, const MpplProcDecl *syntax);
static void print_fml_param_list(Printer *printer, const MpplFmlParamList *syntax);
static void print_fml_param_sec(Printer *printer, const MpplFmlParamSec *syntax);
static void print_stmt(Printer *printer, const AnyMpplStmt *syntax);
static void print_assign_stmt(Printer *printer, const MpplAssignStmt *syntax);
static void print_if_stmt(Printer *printer, const MpplIfStmt *syntax);
static void print_while_stmt(Printer *printer, const MpplWhileStmt *syntax);
static void print_break_stmt(Printer *printer, const MpplBreakStmt *syntax);
static void print_call_stmt(Printer *printer, const MpplCallStmt *syntax);
static void print_return_stmt(Printer *printer, const MpplReturnStmt *syntax);
static void print_input_stmt(Printer *printer, const MpplInputStmt *syntax);
static void print_output_stmt(Printer *printer, const MpplOutputStmt *syntax);
static void print_comp_stmt(Printer *printer, const MpplCompStmt *syntax);
static void print_act_param_list(Printer *printer, const MpplActParamList *syntax);
static void print_input_list(Printer *printer, const MpplInputList *syntax);
static void print_expr(Printer *printer, const AnyMpplExpr *syntax);
static void print_binary_expr(Printer *printer, const MpplBinaryExpr *syntax);
static void print_paren_expr(Printer *printer, const MpplParenExpr *syntax);
static void print_not_expr(Printer *printer, const MpplNotExpr *syntax);
static void print_cast_expr(Printer *printer, const MpplCastExpr *syntax);
static void print_var(Printer *printer, const AnyMpplVar *syntax);
static void print_entire_var(Printer *printer, const MpplEntireVar *syntax);
static void print_indexed_var(Printer *printer, const MpplIndexedVar *syntax);
static void print_type(Printer *printer, const AnyMpplType *syntax);
static void print_std_type(Printer *printer, const AnyMpplStdType *syntax);
static void print_array_type(Printer *printer, const MpplArrayType *syntax);
static void print_out_list(Printer *printer, const MpplOutList *syntax);
static void print_out_value(Printer *printer, const MpplOutValue *syntax);
static void print_lit(Printer *printer, const AnyMpplLit *syntax);
static void print_token(Printer *printer, const MpplToken *syntax);
static void print_token_program(Printer *printer, const MpplToken *syntax);
static void print_token_keyword(Printer *printer, const MpplToken *syntax);
static void print_token_operator(Printer *printer, const MpplToken *syntax);
static void print_token_proc(Printer *printer, const MpplToken *syntax);
static void print_token_param(Printer *printer, const MpplToken *syntax);
static void print_token_string(Printer *printer, const MpplToken *syntax);
static void print_token_lit(Printer *printer, const MpplToken *syntax);

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

static void print_program(Printer *printer, const MpplProgram *syntax)
{
  MpplToken    *program_token = mppl_program__program_token(syntax);
  MpplToken    *name_token    = mppl_program__name(syntax);
  MpplToken    *semi_token    = mppl_program__semi_token(syntax);
  MpplCompStmt *stmt          = mppl_program__stmt(syntax);
  MpplToken    *dot_token     = mppl_program__dot_token(syntax);
  unsigned long i;

  print_token_keyword(printer, program_token);
  print_space();
  print_token_program(printer, name_token);
  print_token(printer, semi_token);
  print_newline();
  ++printer->indent;
  for (i = 0; i < mppl_program__decl_part_count(syntax); ++i) {
    AnyMpplDeclPart *part = mppl_program__decl_part(syntax, i);

    print_indent(printer);
    print_decl_part(printer, part);

    mppl_unref(part);
  }
  --printer->indent;
  print_indent(printer);
  print_comp_stmt(printer, stmt);
  print_token(printer, dot_token);
  print_newline();

  mppl_unref(program_token);
  mppl_unref(name_token);
  mppl_unref(semi_token);
  mppl_unref(stmt);
  mppl_unref(dot_token);
}

static void print_decl_part(Printer *printer, const AnyMpplDeclPart *syntax)
{
  switch (mppl_decl_part__kind(syntax)) {
  case MPPL_DECL_PART_VAR:
    print_var_decl_part(printer, (const MpplVarDeclPart *) syntax);
    break;
  case MPPL_DECL_PART_PROC:
    print_proc_decl(printer, (const MpplProcDecl *) syntax);
    break;
  }
}

static void print_var_decl_part(Printer *printer, const MpplVarDeclPart *syntax)
{
  MpplToken    *var_token = mppl_var_decl_part__var_token(syntax);
  unsigned long i;

  print_token_keyword(printer, var_token);
  print_newline();
  ++printer->indent;
  for (i = 0; i < mppl_var_decl_part__var_decl_count(syntax); ++i) {
    MpplVarDecl *decl       = mppl_var_decl_part__var_decl(syntax, i);
    MpplToken   *semi_token = mppl_var_decl_part__semi_token(syntax, i);

    print_indent(printer);
    print_var_decl(printer, decl);
    print_token(printer, semi_token);
    print_newline();

    mppl_unref(decl);
    mppl_unref(semi_token);
  }
  --printer->indent;

  mppl_unref(var_token);
}

static void print_var_decl(Printer *printer, const MpplVarDecl *syntax)
{
  MpplToken    *colon_token = mppl_var_decl__colon_token(syntax);
  AnyMpplType  *type        = mppl_var_decl__type(syntax);
  unsigned long i;

  for (i = 0; i < mppl_var_decl__name_count(syntax); ++i) {
    MpplToken *name_token = mppl_var_decl__name(syntax, i);

    print_token(printer, name_token);
    if (i + 1 < mppl_var_decl__name_count(syntax)) {
      MpplToken *comma_token = mppl_var_decl__comma_token(syntax, i);

      print_token(printer, comma_token);
      print_space();

      mppl_unref(comma_token);
    }

    mppl_unref(name_token);
  }
  print_space();
  print_token(printer, colon_token);
  print_space();
  print_type(printer, type);

  mppl_unref(colon_token);
  mppl_unref(type);
}

static void print_proc_decl(Printer *printer, const MpplProcDecl *syntax)
{
  MpplToken        *proc_token   = mppl_proc_decl__procedure_token(syntax);
  MpplToken        *name_token   = mppl_proc_decl__name(syntax);
  MpplFmlParamList *list         = mppl_proc_decl__fml_param_list(syntax);
  MpplToken        *semi_token_0 = mppl_proc_decl__semi_token_0(syntax);
  MpplVarDeclPart  *part         = mppl_proc_decl__var_decl_part(syntax);
  MpplCompStmt     *stmt         = mppl_proc_decl__comp_stmt(syntax);
  MpplToken        *semi_token_1 = mppl_proc_decl__semi_token_1(syntax);

  print_token_keyword(printer, proc_token);
  print_space();
  print_token_proc(printer, name_token);
  if (list) {
    print_fml_param_list(printer, list);
  }
  print_token(printer, semi_token_0);
  print_newline();
  if (part) {
    print_indent(printer);
    print_var_decl_part(printer, part);
  }
  print_indent(printer);
  print_comp_stmt(printer, stmt);
  print_token(printer, semi_token_1);
  print_newline();

  mppl_unref(proc_token);
  mppl_unref(name_token);
  mppl_unref(list);
  mppl_unref(semi_token_0);
  mppl_unref(part);
  mppl_unref(stmt);
  mppl_unref(semi_token_1);
}

static void print_fml_param_list(Printer *printer, const MpplFmlParamList *syntax)
{
  MpplToken    *lparen_token = mppl_fml_param_list__lparen_token(syntax);
  MpplToken    *rparen_token = mppl_fml_param_list__rparen_token(syntax);
  unsigned long i;

  print_token(printer, lparen_token);
  for (i = 0; i < mppl_fml_param_list__sec_count(syntax); ++i) {
    MpplFmlParamSec *sec = mppl_fml_param_list__sec(syntax, i);

    print_fml_param_sec(printer, sec);
    if (i + 1 < mppl_fml_param_list__sec_count(syntax)) {
      MpplToken *semi_token = mppl_fml_param_list__semi_token(syntax, i);

      print_token(printer, semi_token);
      print_space();

      mppl_unref(semi_token);
    }

    mppl_unref(sec);
  }
  print_token(printer, rparen_token);

  mppl_unref(lparen_token);
  mppl_unref(rparen_token);
}

static void print_fml_param_sec(Printer *printer, const MpplFmlParamSec *syntax)
{
  MpplToken    *colon_token = mppl_fml_param_sec__colon_token(syntax);
  AnyMpplType  *type        = mppl_fml_param_sec__type(syntax);
  unsigned long i;

  for (i = 0; i < mppl_fml_param_sec__name_count(syntax); ++i) {
    MpplToken *name_token = mppl_fml_param_sec__name(syntax, i);

    print_token_param(printer, name_token);
    if (i + 1 < mppl_fml_param_sec__name_count(syntax)) {
      MpplToken *comma_token = mppl_fml_param_sec__comma_token(syntax, i);

      print_token(printer, comma_token);
      print_space();

      mppl_unref(comma_token);
    }

    mppl_unref(name_token);
  }
  print_space();
  print_token(printer, colon_token);
  print_space();
  print_type(printer, type);

  mppl_unref(colon_token);
  mppl_unref(type);
}

static void print_stmt(Printer *printer, const AnyMpplStmt *syntax)
{
  switch (mppl_stmt__kind(syntax)) {
  case MPPL_STMT_ASSIGN:
    print_assign_stmt(printer, (const MpplAssignStmt *) syntax);
    break;
  case MPPL_STMT_IF:
    print_if_stmt(printer, (const MpplIfStmt *) syntax);
    break;
  case MPPL_STMT_WHILE:
    print_while_stmt(printer, (const MpplWhileStmt *) syntax);
    break;
  case MPPL_STMT_BREAK:
    print_break_stmt(printer, (const MpplBreakStmt *) syntax);
    break;
  case MPPL_STMT_CALL:
    print_call_stmt(printer, (const MpplCallStmt *) syntax);
    break;
  case MPPL_STMT_RETURN:
    print_return_stmt(printer, (const MpplReturnStmt *) syntax);
    break;
  case MPPL_STMT_INPUT:
    print_input_stmt(printer, (const MpplInputStmt *) syntax);
    break;
  case MPPL_STMT_OUTPUT:
    print_output_stmt(printer, (const MpplOutputStmt *) syntax);
    break;
  case MPPL_STMT_COMP:
    print_comp_stmt(printer, (const MpplCompStmt *) syntax);
    break;
  }
}

static void print_stmt__conditional(Printer *printer, const AnyMpplStmt *syntax)
{
  int is_comp = mppl_stmt__kind(syntax) == MPPL_STMT_COMP;
  if (!is_comp) {
    ++printer->indent;
    print_newline();
    print_indent(printer);
  } else {
    print_space();
  }
  print_stmt(printer, syntax);
  if (!is_comp) {
    --printer->indent;
  }
}

static void print_assign_stmt(Printer *printer, const MpplAssignStmt *syntax)
{
  AnyMpplVar  *var          = mppl_assign_stmt__lhs(syntax);
  MpplToken   *assign_token = mppl_assign_stmt__assign_token(syntax);
  AnyMpplExpr *rhs          = mppl_assign_stmt__rhs(syntax);

  print_var(printer, var);
  print_space();
  print_token(printer, assign_token);
  print_space();
  print_expr(printer, rhs);

  mppl_unref(var);
  mppl_unref(assign_token);
  mppl_unref(rhs);
}

static void print_if_stmt(Printer *printer, const MpplIfStmt *syntax)
{
  MpplToken   *if_token   = mppl_if_stmt__if_token(syntax);
  AnyMpplExpr *cond       = mppl_if_stmt__cond(syntax);
  MpplToken   *then_token = mppl_if_stmt__then_token(syntax);
  AnyMpplStmt *then_stmt  = mppl_if_stmt__then_stmt(syntax);
  MpplToken   *else_token = mppl_if_stmt__else_token(syntax);
  AnyMpplStmt *else_stmt  = mppl_if_stmt__else_stmt(syntax);

  print_token_keyword(printer, if_token);
  print_space();
  print_expr(printer, cond);
  print_space();
  print_token_keyword(printer, then_token);
  print_stmt__conditional(printer, then_stmt);
  if (else_token) {
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

  mppl_unref(if_token);
  mppl_unref(cond);
  mppl_unref(then_token);
  mppl_unref(then_stmt);
  mppl_unref(else_token);
  mppl_unref(else_stmt);
}

static void print_while_stmt(Printer *printer, const MpplWhileStmt *syntax)
{
  MpplToken   *while_token = mppl_while_stmt__while_token(syntax);
  AnyMpplExpr *cond        = mppl_while_stmt__cond(syntax);
  MpplToken   *do_token    = mppl_while_stmt__do_token(syntax);
  AnyMpplStmt *do_stmt     = mppl_while_stmt__do_stmt(syntax);

  print_token_keyword(printer, while_token);
  print_space();
  print_expr(printer, cond);
  print_space();
  print_token_keyword(printer, do_token);
  print_stmt__conditional(printer, do_stmt);

  mppl_unref(while_token);
  mppl_unref(cond);
  mppl_unref(do_token);
  mppl_unref(do_stmt);
}

static void print_break_stmt(Printer *printer, const MpplBreakStmt *syntax)
{
  MpplToken *break_token = mppl_break_stmt__break_token(syntax);

  print_token_keyword(printer, break_token);

  mppl_unref(break_token);
}

static void print_call_stmt(Printer *printer, const MpplCallStmt *syntax)
{
  MpplToken        *call_token = mppl_call_stmt__call_token(syntax);
  MpplToken        *name_token = mppl_call_stmt__name(syntax);
  MpplActParamList *list       = mppl_call_stmt__act_param_list(syntax);

  print_token_keyword(printer, call_token);
  print_space();
  print_token_proc(printer, name_token);
  if (list) {
    print_act_param_list(printer, list);
  }

  mppl_unref(call_token);
  mppl_unref(name_token);
  mppl_unref(list);
}

static void print_return_stmt(Printer *printer, const MpplReturnStmt *syntax)
{
  MpplToken *return_token = mppl_return_stmt__return_token(syntax);

  print_token_keyword(printer, return_token);

  mppl_unref(return_token);
}

static void print_input_stmt(Printer *printer, const MpplInputStmt *syntax)
{
  MpplToken     *read_token = mppl_input_stmt__read_token(syntax);
  MpplInputList *list       = mppl_input_stmt__input_list(syntax);

  print_token_keyword(printer, read_token);
  if (list) {
    print_input_list(printer, list);
  }

  mppl_unref(read_token);
  mppl_unref(list);
}

static void print_output_stmt(Printer *printer, const MpplOutputStmt *syntax)
{
  MpplToken   *write_token = mppl_output_stmt__write_token(syntax);
  MpplOutList *list        = mppl_output_stmt__output_list(syntax);

  print_token_keyword(printer, write_token);
  if (list) {
    print_out_list(printer, list);
  }

  mppl_unref(write_token);
  mppl_unref(list);
}

static void print_comp_stmt(Printer *printer, const MpplCompStmt *syntax)
{
  MpplToken    *begin_token = mppl_comp_stmt__begin_token(syntax);
  MpplToken    *end_token   = mppl_comp_stmt__end_token(syntax);
  unsigned long i;

  print_token_keyword(printer, begin_token);
  ++printer->indent;
  for (i = 0; i < mppl_comp_stmt__stmt_count(syntax); ++i) {
    AnyMpplStmt *inner_stmt = mppl_comp_stmt__stmt(syntax, i);

    if (i + 1 < mppl_comp_stmt__stmt_count(syntax)) {
      MpplToken *semi_token = mppl_comp_stmt__semi_token(syntax, i);

      print_newline();
      print_indent(printer);
      print_stmt(printer, inner_stmt);
      print_token(printer, semi_token);

      mppl_unref(semi_token);
    } else if (inner_stmt) {
      print_newline();
      print_indent(printer);
      print_stmt(printer, inner_stmt);
    }

    mppl_unref(inner_stmt);
  }
  --printer->indent;
  print_newline();
  print_indent(printer);
  print_token_keyword(printer, end_token);

  mppl_unref(begin_token);
  mppl_unref(end_token);
}

static void print_act_param_list(Printer *printer, const MpplActParamList *syntax)
{
  MpplToken    *lparen_token = mppl_act_param_list__lparen_token(syntax);
  MpplToken    *rparen_token = mppl_act_param_list__rparen_token(syntax);
  unsigned long i;

  print_token(printer, lparen_token);
  for (i = 0; i < mppl_act_param_list__expr_count(syntax); ++i) {
    AnyMpplExpr *expr = mppl_act_param_list__expr(syntax, i);

    print_expr(printer, expr);
    if (i + 1 < mppl_act_param_list__expr_count(syntax)) {
      MpplToken *comma_token = mppl_act_param_list__comma_token(syntax, i);

      print_token(printer, comma_token);
      print_space();

      mppl_unref(comma_token);
    }

    mppl_unref(expr);
  }
  print_token(printer, rparen_token);

  mppl_unref(lparen_token);
  mppl_unref(rparen_token);
}

static void print_input_list(Printer *printer, const MpplInputList *syntax)
{
  MpplToken    *lparen_token = mppl_input_list__lparen_token(syntax);
  MpplToken    *rparen_token = mppl_input_list__rparen_token(syntax);
  unsigned long i;

  print_token(printer, lparen_token);
  for (i = 0; i < mppl_input_list__var_count(syntax); ++i) {
    AnyMpplVar *var = mppl_input_list__var(syntax, i);

    print_var(printer, var);
    if (i + 1 < mppl_input_list__var_count(syntax)) {
      MpplToken *comma_token = mppl_input_list__comma_token(syntax, i);

      print_token(printer, comma_token);
      print_space();

      mppl_unref(comma_token);
    }

    mppl_unref(var);
  }
  print_token(printer, rparen_token);

  mppl_unref(lparen_token);
  mppl_unref(rparen_token);
}

static void print_expr(Printer *printer, const AnyMpplExpr *syntax)
{
  switch (mppl_expr__kind(syntax)) {
  case MPPL_EXPR_BINARY:
    print_binary_expr(printer, (const MpplBinaryExpr *) syntax);
    break;
  case MPPL_EXPR_PAREN:
    print_paren_expr(printer, (const MpplParenExpr *) syntax);
    break;
  case MPPL_EXPR_NOT:
    print_not_expr(printer, (const MpplNotExpr *) syntax);
    break;
  case MPPL_EXPR_CAST:
    print_cast_expr(printer, (const MpplCastExpr *) syntax);
    break;
  case MPPL_EXPR_VAR:
    print_var(printer, (const AnyMpplVar *) syntax);
    break;
  case MPPL_EXPR_LIT:
    print_lit(printer, (const AnyMpplLit *) syntax);
    break;
  }
}

static void print_binary_expr(Printer *printer, const MpplBinaryExpr *syntax)
{
  AnyMpplExpr *lhs_expr = mppl_binary_expr__lhs(syntax);
  MpplToken   *op_token = mppl_binary_expr__op_token(syntax);
  AnyMpplExpr *rhs_expr = mppl_binary_expr__rhs(syntax);

  if (lhs_expr) {
    print_expr(printer, lhs_expr);
    print_space();
    print_token_operator(printer, op_token);
    print_space();
    print_expr(printer, rhs_expr);
  } else {
    print_token_operator(printer, op_token);
    print_expr(printer, rhs_expr);
  }

  mppl_unref(lhs_expr);
  mppl_unref(op_token);
  mppl_unref(rhs_expr);
}

static void print_paren_expr(Printer *printer, const MpplParenExpr *syntax)
{
  MpplToken   *lparen_token = mppl_paren_expr__lparen_token(syntax);
  AnyMpplExpr *expr         = mppl_paren_expr__expr(syntax);
  MpplToken   *rparen_token = mppl_paren_expr__rparen_token(syntax);

  print_token(printer, lparen_token);
  print_expr(printer, expr);
  print_token(printer, rparen_token);

  mppl_unref(lparen_token);
  mppl_unref(expr);
  mppl_unref(rparen_token);
}

static void print_not_expr(Printer *printer, const MpplNotExpr *syntax)
{
  MpplToken   *not_token = mppl_not_expr__not_token(syntax);
  AnyMpplExpr *expr      = mppl_not_expr__expr(syntax);

  print_token_operator(printer, not_token);
  print_space();
  print_expr(printer, expr);

  mppl_unref(not_token);
  mppl_unref(expr);
}

static void print_cast_expr(Printer *printer, const MpplCastExpr *syntax)
{
  AnyMpplStdType *type         = mppl_cast_expr__type(syntax);
  MpplToken      *lparen_token = mppl_cast_expr__lparen_token(syntax);
  AnyMpplExpr    *expr_expr    = mppl_cast_expr__expr(syntax);
  MpplToken      *rparen_token = mppl_cast_expr__rparen_token(syntax);

  print_std_type(printer, type);
  print_token(printer, lparen_token);
  print_expr(printer, expr_expr);
  print_token(printer, rparen_token);

  mppl_unref(type);
  mppl_unref(lparen_token);
  mppl_unref(expr_expr);
  mppl_unref(rparen_token);
}

static void print_var(Printer *printer, const AnyMpplVar *syntax)
{
  switch (mppl_var__kind(syntax)) {
  case MPPL_VAR_ENTIRE:
    print_entire_var(printer, (const MpplEntireVar *) syntax);
    break;
  case MPPL_VAR_INDEXED:
    print_indexed_var(printer, (const MpplIndexedVar *) syntax);
    break;
  }
}

static void print_entire_var(Printer *printer, const MpplEntireVar *syntax)
{
  MpplToken *name_token = mppl_entire_var__name(syntax);

  print_token(printer, name_token);

  mppl_unref(name_token);
}

static void print_indexed_var(Printer *printer, const MpplIndexedVar *syntax)
{
  MpplToken   *name_token     = mppl_indexed_var__name(syntax);
  MpplToken   *lbracket_token = mppl_indexed_var__lbracket_token(syntax);
  AnyMpplExpr *expr_expr      = mppl_indexed_var__expr(syntax);
  MpplToken   *rbracket_token = mppl_indexed_var__rbracket_token(syntax);

  print_token(printer, name_token);
  print_token(printer, lbracket_token);
  print_expr(printer, expr_expr);
  print_token(printer, rbracket_token);

  mppl_unref(name_token);
  mppl_unref(lbracket_token);
  mppl_unref(expr_expr);
  mppl_unref(rbracket_token);
}

static void print_type(Printer *printer, const AnyMpplType *syntax)
{
  switch (mppl_type__kind(syntax)) {
  case MPPL_TYPE_STD:
    print_std_type(printer, (const AnyMpplStdType *) syntax);
    break;
  case MPPL_TYPE_ARRAY:
    print_array_type(printer, (const MpplArrayType *) syntax);
    break;
  }
}

static void print_std_type(Printer *printer, const AnyMpplStdType *syntax)
{
  print_token_keyword(printer, (const MpplToken *) syntax);
}

static void print_array_type(Printer *printer, const MpplArrayType *syntax)
{
  MpplToken      *array_token    = mppl_array_type__array_token(syntax);
  MpplToken      *lbracket_token = mppl_array_type__lbracket_token(syntax);
  MpplNumberLit  *size           = mppl_array_type__size(syntax);
  MpplToken      *rbracket_token = mppl_array_type__rbracket_token(syntax);
  MpplToken      *of_token       = mppl_array_type__of_token(syntax);
  AnyMpplStdType *type           = mppl_array_type__type(syntax);

  print_token_keyword(printer, array_token);
  print_token(printer, lbracket_token);
  print_lit(printer, (AnyMpplLit *) size);
  print_token(printer, rbracket_token);
  print_space();
  print_token_keyword(printer, of_token);
  print_space();
  print_std_type(printer, type);

  mppl_unref(array_token);
  mppl_unref(lbracket_token);
  mppl_unref(size);
  mppl_unref(rbracket_token);
  mppl_unref(of_token);
  mppl_unref(type);
}

static void print_out_list(Printer *printer, const MpplOutList *syntax)
{
  MpplToken    *lparen_token = mppl_out_list__lparen_token(syntax);
  MpplToken    *rparen_token = mppl_out_list__rparen_token(syntax);
  unsigned long i;

  print_token(printer, lparen_token);
  for (i = 0; i < mppl_out_list__out_value_count(syntax); ++i) {
    MpplOutValue *value = mppl_out_list__out_value(syntax, i);

    print_out_value(printer, value);
    if (i + 1 < mppl_out_list__out_value_count(syntax)) {
      MpplToken *comma_token = mppl_out_list__comma_token(syntax, i);

      print_token(printer, comma_token);
      print_space();

      mppl_unref(comma_token);
    }

    mppl_unref(value);
  }
  print_token(printer, rparen_token);

  mppl_unref(lparen_token);
  mppl_unref(rparen_token);
}

static void print_out_value(Printer *printer, const MpplOutValue *syntax)
{
  AnyMpplExpr   *expr  = mppl_out_value__expr(syntax);
  MpplToken     *token = mppl_out_value__colon_token(syntax);
  MpplNumberLit *width = mppl_out_value__width(syntax);

  print_expr(printer, expr);
  if (token) {
    print_space();
    print_token(printer, token);
    print_space();
    print_lit(printer, (AnyMpplLit *) width);
  }

  mppl_unref(expr);
  mppl_unref(token);
  mppl_unref(width);
}

static void print_lit(Printer *printer, const AnyMpplLit *syntax)
{
  switch (mppl_lit__kind(syntax)) {
  case MPPL_LIT_STRING:
    print_token_string(printer, (const MpplToken *) syntax);
    break;
  case MPPL_LIT_NUMBER:
    print_token_lit(printer, (const MpplToken *) syntax);
    break;
  case MPPL_LIT_BOOLEAN:
    print_token_lit(printer, (const MpplToken *) syntax);
    break;
  }
}

static void print_token__impl(Printer *printer, const MpplToken *token, unsigned long color)
{
  const SyntaxTree     *tree      = (SyntaxTree *) token;
  const RawSyntaxToken *raw_token = (RawSyntaxToken *) syntax_tree_raw(tree);
  if (printer->option.color.enabled) {
    printf("\033[38;2;%lu;%lu;%lum", color >> 16, (color >> 8) & 0xFF, color & 0xFF);
  }
  printf("%s", string_data(raw_token->string));
  if (printer->option.color.enabled) {
    printf("\033[0m");
  }
  fflush(stdout);
}

static void print_token(Printer *printer, const MpplToken *token)
{
  print_token__impl(printer, token, printer->option.color.foreground);
}

static void print_token_program(Printer *printer, const MpplToken *token)
{
  print_token__impl(printer, token, printer->option.color.program);
}

static void print_token_keyword(Printer *printer, const MpplToken *token)
{
  print_token__impl(printer, token, printer->option.color.keyword);
}

static void print_token_operator(Printer *printer, const MpplToken *token)
{
  print_token__impl(printer, token, printer->option.color.operator);
}

static void print_token_proc(Printer *printer, const MpplToken *token)
{
  print_token__impl(printer, token, printer->option.color.procedure);
}

static void print_token_param(Printer *printer, const MpplToken *token)
{
  print_token__impl(printer, token, printer->option.color.parameter);
}

static void print_token_string(Printer *printer, const MpplToken *token)
{
  print_token__impl(printer, token, printer->option.color.string);
}

static void print_token_lit(Printer *printer, const MpplToken *token)
{
  print_token__impl(printer, token, printer->option.color.literal);
}

void mppl_pretty_print(const MpplProgram *syntax, const PrinterOption *option)
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

  print_program(&printer, syntax);
}
