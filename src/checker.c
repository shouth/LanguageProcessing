#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "array.h"
#include "checker.h"
#include "inference.h"
#include "mppl_syntax.h"
#include "mppl_syntax_ext.h"
#include "report.h"
#include "resolution.h"
#include "syntax_kind.h"
#include "syntax_tree.h"
#include "token_tree.h"
#include "type.h"
#include "utility.h"

typedef struct Checker Checker;

struct Checker {
  const Res *res;
  Infer     *inference;
  Array     *errors;
};

static const Type *get_def_type(const Checker *checker, const SyntaxTree *tree)
{
  const Def *def = res_get_def(checker->res, syntax_tree_raw(tree));
  if (def) {
    return infer_get_def_type(checker->inference, def);
  } else {
    unreachable();
  }
}

static const Type *get_ref_type(const Checker *checker, const SyntaxTree *tree)
{
  const Def *def = res_get_ref(checker->res, syntax_tree_raw(tree));
  if (def) {
    return infer_get_def_type(checker->inference, def);
  } else {
    unreachable();
  }
}

static const Type *get_expr_type(const Checker *checker, const SyntaxTree *tree)
{
  return infer_get_expr_type(checker->inference, syntax_tree_raw(tree));
}

static void record_def_type(Checker *checker, const SyntaxTree *name_syntax, Type *type)
{
  const Def *def = res_get_def(checker->res, syntax_tree_raw(name_syntax));
  if (def) {
    infer_record_def_type(checker->inference, def, type);
  } else {
    unreachable();
  }
}

static void record_expr_type(Checker *checker, const SyntaxTree *tree, Type *type)
{
  infer_record_expr_type(checker->inference, syntax_tree_raw(tree), type);
}

static void infer_var(Checker *checker, const MpplVarDecl *syntax)
{
  unsigned long i;
  AnyMpplType  *type_syntax = mppl_var_decl__type(syntax);
  for (i = 0; i < mppl_var_decl__name_count(syntax); ++i) {
    Type      *type        = mppl_type__to_type(type_syntax);
    MpplToken *name_syntax = mppl_var_decl__name(syntax, i);
    record_def_type(checker, (const SyntaxTree *) name_syntax, type);
    mppl_free(name_syntax);
  }
  mppl_free(type_syntax);
}

static void infer_param(Checker *checker, const MpplFmlParamSec *syntax)
{
  unsigned long i;
  AnyMpplType  *type_syntax = mppl_fml_param_sec__type(syntax);
  for (i = 0; i < mppl_fml_param_sec__name_count(syntax); ++i) {
    Type      *type        = mppl_type__to_type(type_syntax);
    MpplToken *name_syntax = mppl_fml_param_sec__name(syntax, i);
    record_def_type(checker, (SyntaxTree *) name_syntax, type);
    mppl_free(name_syntax);
  }
  mppl_free(type_syntax);
}

static void infer_proc(Checker *checker, const MpplProcDecl *syntax)
{
  unsigned long i, j;
  Array        *params = array_new(sizeof(Type *));

  {
    MpplFmlParamList *param_list_syntax = mppl_proc_decl__fml_param_list(syntax);
    for (i = 0; i < mppl_fml_param_list__sec_count(param_list_syntax); ++i) {
      MpplFmlParamSec *param_sec_syntax = mppl_fml_param_list__sec(param_list_syntax, i);
      infer_param(checker, param_sec_syntax);
      for (j = 0; j < mppl_fml_param_sec__name_count(param_sec_syntax); ++j) {
        MpplToken  *name_syntax = mppl_fml_param_sec__name(param_sec_syntax, j);
        const Type *param_type  = get_def_type(checker, (SyntaxTree *) name_syntax);
        Type       *infer_type  = type_clone(param_type);
        array_push(params, &infer_type);
        mppl_free(name_syntax);
      }
      mppl_free(param_sec_syntax);
    }
    mppl_free(param_list_syntax);
  }

  {
    MpplToken    *name_syntax = mppl_proc_decl__name(syntax);
    unsigned long param_count = array_count(params);
    Type        **param_types = array_steal(params);
    Type         *infer_type  = type_new_proc(param_types, param_count);
    record_def_type(checker, (SyntaxTree *) name_syntax, infer_type);
    mppl_free(name_syntax);
  }
}

static void error_assign_impossible(const Checker *checker, const SyntaxTree *lhs, const Type *lhs_type)
{
  char         *lhs_type_string = type_to_string(lhs_type);
  unsigned long offset          = syntax_tree_offset(lhs);
  unsigned long length          = syntax_tree_text_length(lhs);
  Report       *report          = report_new(REPORT_KIND_ERROR, offset, "assignment operation cannot be applied to `%s`", lhs_type_string);
  report_annotation(report, offset, offset + length,
    "left operand of assignment statement should be a variable of standard type", lhs_type_string);
  array_push(checker->errors, &report);
  free(lhs_type_string);
}

static void error_assign_type_mismatch(
  const Checker *checker, const SyntaxTree *node,
  const SyntaxTree *lhs, const Type *lhs_type,
  const SyntaxTree *rhs, const Type *rhs_type)
{
  char         *lhs_type_string = type_to_string(lhs_type);
  char         *rhs_type_string = type_to_string(rhs_type);
  unsigned long lhs_offset      = syntax_tree_offset(lhs);
  unsigned long lhs_length      = syntax_tree_text_length(lhs);
  unsigned long rhs_offset      = syntax_tree_offset(rhs);
  unsigned long rhs_length      = syntax_tree_text_length(rhs);
  Report       *report          = report_new(REPORT_KIND_ERROR, syntax_tree_offset(node), "mismatched types");
  report_annotation(report, lhs_offset, lhs_offset + lhs_length,
    "`%s`", lhs_type_string);
  report_annotation(report, rhs_offset, rhs_offset + rhs_length,
    "expected `%s`, found `%s`", lhs_type_string, rhs_type_string);

  array_push(checker->errors, &report);
  free(lhs_type_string);
  free(rhs_type_string);
}

static void check_assign_stmt(Checker *checker, const MpplAssignStmt *syntax)
{
  AnyMpplVar  *lhs_syntax = mppl_assign_stmt__lhs(syntax);
  AnyMpplExpr *rhs_syntax = mppl_assign_stmt__rhs(syntax);
  const Type  *lhs_type   = get_expr_type(checker, (SyntaxTree *) lhs_syntax);
  const Type  *rhs_type   = get_expr_type(checker, (SyntaxTree *) rhs_syntax);

  if (lhs_type && rhs_type) {
    if (!type_is_std(lhs_type)) {
      error_assign_impossible(checker, (SyntaxTree *) lhs_syntax, lhs_type);
    } else if (!type_equal(lhs_type, rhs_type)) {
      error_assign_type_mismatch(checker,
        (SyntaxTree *) syntax, (SyntaxTree *) lhs_syntax, lhs_type, (SyntaxTree *) rhs_syntax, rhs_type);
    }
  }
  mppl_free(lhs_syntax);
  mppl_free(rhs_syntax);
}

static void error_conditional_stmt_invalid_condition(Checker *checker, const SyntaxTree *node, const Type *type)
{
  char         *type_string = type_to_string(type);
  unsigned long offset      = syntax_tree_offset(node);
  unsigned long length      = syntax_tree_text_length(node);
  Report       *report      = report_new(REPORT_KIND_ERROR, offset, "`%s` cannot be used as a condition", type_string);
  report_annotation(report, offset, offset + length, "condition should be `boolean`");
  array_push(checker->errors, &report);
  free(type_string);
}

static void check_if_stmt(Checker *checker, const MpplIfStmt *syntax)
{
  AnyMpplExpr *cond_syntax = mppl_if_stmt__cond(syntax);
  const Type  *cond_type   = get_expr_type(checker, (SyntaxTree *) cond_syntax);

  if (cond_type && type_kind(cond_type) != TYPE_BOOLEAN) {
    error_conditional_stmt_invalid_condition(checker, (SyntaxTree *) cond_syntax, cond_type);
  }
  mppl_free(cond_syntax);
}

static void check_while_stmt(Checker *checker, const MpplWhileStmt *syntax)
{
  AnyMpplExpr *cond_syntax = mppl_while_stmt__cond(syntax);
  const Type  *cond_type   = get_expr_type(checker, (SyntaxTree *) cond_syntax);

  if (cond_type && type_kind(cond_type) != TYPE_BOOLEAN) {
    error_conditional_stmt_invalid_condition(checker, (SyntaxTree *) cond_syntax, cond_type);
  }
  mppl_free(cond_syntax);
}

static void error_call_stmt_non_callable(Checker *checker, const MpplToken *syntax)
{
  const Token  *token  = (const Token *) syntax_tree_raw((SyntaxTree *) syntax);
  unsigned long offset = syntax_tree_offset((SyntaxTree *) syntax);
  unsigned long length = syntax_tree_text_length((SyntaxTree *) syntax);
  Report       *report = report_new(REPORT_KIND_ERROR, offset, "`%s` cannot be called", token->text);
  report_annotation(report, offset, offset + length, "an item to be called should be a `procedure`");
  array_push(checker->errors, &report);
}

static void error_call_stmt_mismatched_param_count(
  Checker *checker, const MpplActParamList *syntax,
  const TypeProc *proc_type, unsigned long act_param_count)
{
  unsigned long offset      = syntax_tree_offset((SyntaxTree *) syntax);
  unsigned long length      = syntax_tree_text_length((SyntaxTree *) syntax);
  Report       *report      = report_new(REPORT_KIND_ERROR, offset, "mismatched the number of parameter");
  unsigned long param_count = type_proc_param_count(proc_type);
  report_annotation(report, offset, offset + length,
    "expected %lu %s, found %lu", param_count, param_count > 1 ? "parameters" : "parameter", act_param_count);
  array_push(checker->errors, &report);
}

static void error_call_stmt_mismatched_param_type(
  Checker *checker, const MpplCallStmt *syntax,
  const TypeProc *defined_type, const TypeProc *act_type)
{
  unsigned long i;
  unsigned long offset = syntax_tree_offset((SyntaxTree *) syntax);
  Report       *report = report_new(REPORT_KIND_ERROR, offset, "mismatched parameter type");

  MpplActParamList *act_param_list_syntax = mppl_call_stmt__act_param_list(syntax);
  if (act_param_list_syntax) {
    for (i = 0; i < type_proc_param_count(defined_type); ++i) {
      AnyMpplExpr *act_param_syntax   = mppl_act_param_list__expr(act_param_list_syntax, i);
      const Type  *defined_param_type = type_proc_param(defined_type, i);
      const Type  *act_param_type     = type_proc_param(act_type, i);
      if (!type_equal(defined_param_type, act_param_type)) {
        unsigned long act_param_offset          = syntax_tree_offset((SyntaxTree *) act_param_syntax);
        unsigned long act_param_length          = syntax_tree_text_length((SyntaxTree *) act_param_syntax);
        char         *defined_param_type_string = type_to_string(defined_param_type);
        char         *act_param_type_string     = type_to_string(act_param_type);
        report_annotation(report, act_param_offset, act_param_offset + act_param_length,
          "expected `%s`, found `%s`", defined_param_type_string, act_param_type_string);
        free(defined_param_type_string);
        free(act_param_type_string);
      }
      mppl_free(act_param_syntax);
    }
  }
  mppl_free(act_param_list_syntax);
  array_push(checker->errors, &report);
}

static void check_call_stmt(Checker *checker, const MpplCallStmt *syntax)
{
  MpplToken  *name_syntax = mppl_call_stmt__name(syntax);
  const Type *type        = get_ref_type(checker, (SyntaxTree *) name_syntax);

  if (type && type_kind(type) != TYPE_PROC) {
    error_call_stmt_non_callable(checker, name_syntax);
  } else {
    MpplActParamList *act_param_list_syntax = mppl_call_stmt__act_param_list(syntax);
    Type             *act_type;
    {
      Array *act_param_array = array_new(sizeof(Type *));
      if (act_param_list_syntax) {
        unsigned long i;
        for (i = 0; i < mppl_act_param_list__expr_count(act_param_list_syntax); ++i) {
          AnyMpplExpr *act_param_syntax = mppl_act_param_list__expr(act_param_list_syntax, i);
          const Type  *act_param_type   = get_expr_type(checker, (SyntaxTree *) act_param_syntax);
          Type        *act_param_clone  = type_clone(act_param_type);
          array_push(act_param_array, &act_param_clone);
          mppl_free(act_param_syntax);
        }
      }
      {
        unsigned long act_param_count = array_count(act_param_array);
        Type        **act_param_types = array_steal(act_param_array);
        act_type                      = type_new_proc(act_param_types, act_param_count);
      }
    }

    if (type_proc_param_count((const TypeProc *) type) != type_proc_param_count((const TypeProc *) act_type)) {
      error_call_stmt_mismatched_param_count(checker,
        act_param_list_syntax, (const TypeProc *) type, type_proc_param_count((const TypeProc *) act_type));
    } else if (!type_equal(type, act_type)) {
      error_call_stmt_mismatched_param_type(checker,
        syntax, (const TypeProc *) type, (TypeProc *) act_type);
    }
    mppl_free(act_param_list_syntax);
    type_free(act_type);
  }
  mppl_free(name_syntax);
}

static void error_input_stmt_invalid_operand(Checker *checker, const MpplInputList *syntax)
{
  unsigned long i;
  unsigned long offset = syntax_tree_offset((SyntaxTree *) syntax);

  Report *report = report_new(REPORT_KIND_ERROR, offset, "invalid operand");
  for (i = 0; i < mppl_input_list__var_count(syntax); ++i) {
    AnyMpplVar *var_syntax = mppl_input_list__var(syntax, i);
    const Type *var_type   = get_expr_type(checker, (SyntaxTree *) var_syntax);

    if (type_kind(var_type) != TYPE_INTEGER && type_kind(var_type) != TYPE_CHAR) {
      unsigned long var_offset      = syntax_tree_offset((SyntaxTree *) var_syntax);
      unsigned long var_length      = syntax_tree_text_length((SyntaxTree *) var_syntax);
      char         *var_type_string = type_to_string(var_type);
      report_annotation(report, var_offset, var_offset + var_length,
        "expected `integer` or `char`, found `%s`", var_type_string);
      free(var_type_string);
    }
  }
}

static void check_input_stmt(Checker *checker, const MpplInputStmt *syntax)
{
  unsigned long  i;
  MpplInputList *input_list_syntax = mppl_input_stmt__input_list(syntax);

  if (input_list_syntax) {
    for (i = 0; i < mppl_input_list__var_count(input_list_syntax); ++i) {
      AnyMpplVar *var_syntax = mppl_input_list__var(input_list_syntax, i);
      const Type *var_type   = get_expr_type(checker, (SyntaxTree *) var_syntax);

      mppl_free(var_syntax);

      if (var_type && type_kind(var_type) != TYPE_INTEGER && type_kind(var_type) != TYPE_CHAR) {
        error_input_stmt_invalid_operand(checker, input_list_syntax);
        break;
      }
    }
  }
  mppl_free(input_list_syntax);
}

static void error_output_stmt_invalid_operand(Checker *checker, const MpplOutList *syntax)
{
  unsigned long i;
  unsigned long offset = syntax_tree_offset((SyntaxTree *) syntax);

  Report *report = report_new(REPORT_KIND_ERROR, offset, "invalid operand");
  for (i = 0; i < mppl_out_list__out_value_count(syntax); ++i) {
    MpplOutValue  *out_value_syntax = mppl_out_list__out_value(syntax, i);
    MpplToken     *colon_syntax     = mppl_out_value__colon_token(out_value_syntax);
    MpplLitNumber *width_syntax     = mppl_out_value__width(out_value_syntax);
    AnyMpplExpr   *expr_syntax      = mppl_out_value__expr(out_value_syntax);
    const Type    *expr_type        = get_expr_type(checker, (SyntaxTree *) expr_syntax);

    if (!type_is_std(expr_type)) {
      if (type_kind(expr_type) != TYPE_STRING) {
        unsigned long expr_offset      = syntax_tree_offset((SyntaxTree *) expr_syntax);
        unsigned long expr_length      = syntax_tree_text_length((SyntaxTree *) expr_syntax);
        char         *expr_type_string = type_to_string(expr_type);
        report_annotation(report, expr_offset, expr_offset + expr_length,
          "expected `integer`, `char` or `boolean`, found `%s`", expr_type_string);
        free(expr_type_string);
      } else if (colon_syntax) {
        unsigned long start
          = syntax_tree_offset((SyntaxTree *) colon_syntax);
        unsigned long end
          = syntax_tree_offset((SyntaxTree *) width_syntax) + syntax_tree_text_length((SyntaxTree *) width_syntax);
        report_annotation(report, start, end,
          "field width cannot be applied to `string`");
        mppl_free(width_syntax);
      }
    }
    mppl_free(expr_syntax);
    mppl_free(colon_syntax);
    mppl_free(out_value_syntax);
  }
  array_push(checker->errors, &report);
}

static void check_output_stmt(Checker *checker, const MpplOutputStmt *syntax)
{
  unsigned long i;
  MpplOutList  *out_list_syntax = mppl_output_stmt__output_list(syntax);

  if (out_list_syntax) {
    for (i = 0; i < mppl_out_list__out_value_count(out_list_syntax); ++i) {
      MpplOutValue *out_value_syntax = mppl_out_list__out_value(out_list_syntax, i);
      MpplToken    *colon_syntax     = mppl_out_value__colon_token(out_value_syntax);
      AnyMpplExpr  *expr_syntax      = mppl_out_value__expr(out_value_syntax);
      const Type   *expr_type        = get_expr_type(checker, (SyntaxTree *) expr_syntax);

      mppl_free(expr_syntax);
      mppl_free(colon_syntax);
      mppl_free(out_value_syntax);

      if (expr_type && !type_is_std(expr_type) && (type_kind(expr_type) != TYPE_STRING || colon_syntax)) {
        error_output_stmt_invalid_operand(checker, out_list_syntax);
        break;
      }
    }
  }
  mppl_free(out_list_syntax);
}

static const char *op_to_str(SyntaxKind kind)
{
  switch (kind) {
  case SYNTAX_EQUAL_TOKEN:
    return "=";
  case SYNTAX_NOTEQ_TOKEN:
    return "<>";
  case SYNTAX_LESS_TOKEN:
    return "<";
  case SYNTAX_LESSEQ_TOKEN:
    return "<=";
  case SYNTAX_GREATER_TOKEN:
    return ">";
  case SYNTAX_GREATEREQ_TOKEN:
    return ">=";
  case SYNTAX_PLUS_TOKEN:
    return "+";
  case SYNTAX_MINUS_TOKEN:
    return "-";
  case SYNTAX_STAR_TOKEN:
    return "*";
  case SYNTAX_DIV_KW:
    return "div";
  case SYNTAX_AND_KW:
    return "and";
  case SYNTAX_OR_KW:
    return "or";
  default:
    unreachable();
  }
}

static void error_binary_expr_invalid_operand(
  const Checker    *checker,
  const SyntaxTree *node, const SyntaxTree *op_token,
  int lhs_invalid, const SyntaxTree *lhs, const Type *lhs_type,
  int rhs_invalid, const SyntaxTree *rhs, const Type *rhs_type,
  const char *type_str)
{
  char         *lhs_type_str = type_to_string(lhs_type);
  char         *rhs_type_str = type_to_string(rhs_type);
  unsigned long lhs_offset   = syntax_tree_offset(lhs);
  unsigned long lhs_length   = syntax_tree_text_length(lhs);
  unsigned long rhs_offset   = syntax_tree_offset(rhs);
  unsigned long rhs_length   = syntax_tree_text_length(rhs);
  const char   *op_str       = op_to_str(syntax_tree_kind(op_token));
  unsigned long op_offset    = syntax_tree_offset(op_token);
  unsigned long op_length    = syntax_tree_text_length(op_token);
  Report       *report;

  if (lhs_invalid && rhs_invalid) {
    report = report_new(REPORT_KIND_ERROR, syntax_tree_offset(node),
      "binary operation `%s` cannot be applied to `%s` and `%s`", op_str, lhs_type_str, rhs_type_str);
  } else if (lhs_invalid) {
    report = report_new(REPORT_KIND_ERROR, lhs_offset,
      "binary operation `%s` cannot be applied to `%s`", op_str, lhs_type_str);
  } else if (rhs_invalid) {
    report = report_new(REPORT_KIND_ERROR, rhs_offset,
      "binary operation `%s` cannot be applied to `%s`", op_str, rhs_type_str);
  } else {
    unreachable();
  }

  report_annotation(report, lhs_offset, lhs_offset + lhs_length, "`%s`", lhs_type_str);
  report_annotation(report, op_offset, op_offset + op_length, "binary operation `%s` can be applied to %s", op_str, type_str);
  report_annotation(report, rhs_offset, rhs_offset + rhs_length, "`%s`", rhs_type_str);

  array_push(checker->errors, &report);
  free(lhs_type_str);
  free(rhs_type_str);
}

static void error_unary_expr_invalid_operand(
  const Checker    *checker,
  const SyntaxTree *node, const SyntaxTree *op_token,
  const SyntaxTree *rhs, const Type *rhs_type,
  const char *type_str)
{
  char         *rhs_type_str = type_to_string(rhs_type);
  unsigned long rhs_offset   = syntax_tree_offset(rhs);
  unsigned long rhs_length   = syntax_tree_text_length(rhs);
  const char   *op_str       = op_to_str(syntax_tree_kind(op_token));
  unsigned long op_offset    = syntax_tree_offset(op_token);
  unsigned long op_length    = syntax_tree_text_length(op_token);

  Report *report = report_new(REPORT_KIND_ERROR, syntax_tree_offset(node),
    "unary operation `%s` cannot be applied to `%s`", op_str, rhs_type_str);
  report_annotation(report, op_offset, op_offset + op_length, "unary operation `%s` can be applied to %s", op_str, type_str);
  report_annotation(report, rhs_offset, rhs_offset + rhs_length, "`%s`", rhs_type_str);

  array_push(checker->errors, &report);
  free(rhs_type_str);
}

static void error_relational_mismatched_type(
  Checker *checker, const SyntaxTree *node,
  const SyntaxTree *lhs, const Type *lhs_type,
  const SyntaxTree *rhs, const Type *rhs_type)
{
  char         *lhs_type_string = type_to_string(lhs_type);
  char         *rhs_type_string = type_to_string(rhs_type);
  unsigned long lhs_offset      = syntax_tree_offset(lhs);
  unsigned long lhs_length      = syntax_tree_text_length(lhs);
  unsigned long rhs_offset      = syntax_tree_offset(rhs);
  unsigned long rhs_length      = syntax_tree_text_length(rhs);
  Report       *report          = report_new(REPORT_KIND_ERROR, syntax_tree_offset(node), "mismatched types");
  report_annotation(report, lhs_offset, lhs_offset + lhs_length, "`%s`", lhs_type_string);
  report_annotation(report, rhs_offset, rhs_offset + rhs_length, "`%s`", rhs_type_string);

  array_push(checker->errors, &report);
  free(lhs_type_string);
  free(rhs_type_string);
}

static void check_binary_expr(Checker *checker, const MpplBinaryExpr *syntax)
{
  AnyMpplExpr *lhs_syntax = mppl_binary_expr__lhs(syntax);
  AnyMpplExpr *rhs_syntax = mppl_binary_expr__rhs(syntax);
  MpplToken   *op_syntax  = mppl_binary_expr__op_token(syntax);

  if (lhs_syntax) {
    const Type *lhs_type = get_expr_type(checker, (SyntaxTree *) lhs_syntax);
    const Type *rhs_type = get_expr_type(checker, (SyntaxTree *) rhs_syntax);
    int         lhs_invalid, rhs_invalid;

    if (lhs_type && rhs_type) {
      switch (syntax_tree_kind((SyntaxTree *) op_syntax)) {
      case SYNTAX_EQUAL_TOKEN:
      case SYNTAX_NOTEQ_TOKEN:
      case SYNTAX_LESS_TOKEN:
      case SYNTAX_LESSEQ_TOKEN:
      case SYNTAX_GREATER_TOKEN:
      case SYNTAX_GREATEREQ_TOKEN:
        lhs_invalid = !type_is_std(lhs_type);
        rhs_invalid = !type_is_std(rhs_type);
        if (lhs_invalid || rhs_invalid) {
          error_binary_expr_invalid_operand(checker,
            (SyntaxTree *) syntax, (SyntaxTree *) op_syntax,
            lhs_invalid, (SyntaxTree *) lhs_syntax, lhs_type,
            rhs_invalid, (SyntaxTree *) rhs_syntax, rhs_type,
            "`integer`, `char` or `boolean`");
        } else if (!type_equal(lhs_type, rhs_type)) {
          error_relational_mismatched_type(checker,
            (SyntaxTree *) syntax, (SyntaxTree *) lhs_syntax, lhs_type, (SyntaxTree *) rhs_syntax, rhs_type);
        } else {
          Type *infer_type = type_new(TYPE_BOOLEAN);
          record_expr_type(checker, (SyntaxTree *) syntax, infer_type);
        }
        break;

      case SYNTAX_PLUS_TOKEN:
      case SYNTAX_MINUS_TOKEN:
      case SYNTAX_STAR_TOKEN:
      case SYNTAX_DIV_KW:
        lhs_invalid = type_kind(lhs_type) != TYPE_INTEGER;
        rhs_invalid = type_kind(rhs_type) != TYPE_INTEGER;
        if (lhs_invalid || rhs_invalid) {
          error_binary_expr_invalid_operand(checker,
            (SyntaxTree *) syntax, (SyntaxTree *) op_syntax,
            lhs_invalid, (SyntaxTree *) lhs_syntax, lhs_type,
            rhs_invalid, (SyntaxTree *) rhs_syntax, rhs_type,
            "`integer`");
        } else {
          Type *infer_type = type_new(TYPE_INTEGER);
          record_expr_type(checker, (SyntaxTree *) syntax, infer_type);
        }
        break;

      case SYNTAX_AND_KW:
      case SYNTAX_OR_KW:
        lhs_invalid = type_kind(lhs_type) != TYPE_BOOLEAN;
        rhs_invalid = type_kind(rhs_type) != TYPE_BOOLEAN;
        if (lhs_invalid || rhs_invalid) {
          error_binary_expr_invalid_operand(checker,
            (SyntaxTree *) syntax, (SyntaxTree *) op_syntax,
            lhs_invalid, (SyntaxTree *) lhs_syntax, lhs_type,
            rhs_invalid, (SyntaxTree *) rhs_syntax, rhs_type,
            "`boolean`");
        } else {
          Type *infer_type = type_new(TYPE_BOOLEAN);
          record_expr_type(checker, (SyntaxTree *) syntax, infer_type);
        }
        break;

      default:
        unreachable();
      }
    }
  } else {
    const Type *rhs_type = get_expr_type(checker, (SyntaxTree *) rhs_syntax);
    switch (syntax_tree_kind((SyntaxTree *) op_syntax)) {
    case SYNTAX_PLUS_TOKEN:
    case SYNTAX_MINUS_TOKEN:
      if (type_kind(rhs_type) != TYPE_INTEGER) {
        error_unary_expr_invalid_operand(checker,
          (SyntaxTree *) syntax, (SyntaxTree *) op_syntax, (SyntaxTree *) rhs_syntax, rhs_type, "`integer`");
      } else {
        Type *infer_type = type_new(TYPE_INTEGER);
        record_expr_type(checker, (SyntaxTree *) syntax, infer_type);
      }
      break;
    default:
      unreachable();
    }
  }

  mppl_free(op_syntax);
  mppl_free(lhs_syntax);
  mppl_free(rhs_syntax);
}

static void check_paren_expr(Checker *checker, const MpplParenExpr *syntax)
{
  AnyMpplExpr *expr_syntax = mppl_paren_expr__expr(syntax);
  const Type  *type        = get_expr_type(checker, (SyntaxTree *) expr_syntax);

  if (type) {
    Type *infer_type = type_clone(type);
    record_expr_type(checker, (SyntaxTree *) syntax, infer_type);
  }
  mppl_free(expr_syntax);
}

static void error_not_expr_invalid_operand(
  Checker *checker, const SyntaxTree *node,
  const SyntaxTree *not_token, const SyntaxTree *expr, const Type *expr_type)
{
  char         *expr_type_string = type_to_string(expr_type);
  unsigned long not_offset       = syntax_tree_offset(not_token);
  unsigned long not_length       = syntax_tree_text_length(not_token);
  unsigned long expr_offset      = syntax_tree_offset(expr);
  unsigned long expr_length      = syntax_tree_text_length(expr);

  Report *report = report_new(REPORT_KIND_ERROR, syntax_tree_offset(node),
    "`not` operation cannot be applied to `%s`", expr_type_string);
  report_annotation(report, not_offset, not_offset + not_length, "`not` operation can be applied to `boolean`");
  report_annotation(report, expr_offset, expr_offset + expr_length, "`%s`", expr_type_string);
  array_push(checker->errors, &report);
  free(expr_type_string);
}

static void check_not_expr(Checker *checker, const MpplNotExpr *syntax)
{
  AnyMpplExpr *expr_syntax = mppl_not_expr__expr(syntax);
  const Type  *type        = get_expr_type(checker, (SyntaxTree *) expr_syntax);

  if (type) {
    if (type_kind(type) != TYPE_BOOLEAN) {
      MpplToken *not_token = mppl_not_expr__not_token(syntax);
      error_not_expr_invalid_operand(checker,
        (SyntaxTree *) syntax, (SyntaxTree *) not_token, (SyntaxTree *) expr_syntax, type);
      mppl_free(not_token);
    } else {
      Type *infer_type = type_new(TYPE_BOOLEAN);
      record_expr_type(checker, (SyntaxTree *) syntax, infer_type);
    }
  }
  mppl_free(expr_syntax);
}

static void error_cast_expr_invalid_operand(
  Checker *checker, const SyntaxTree *node,
  const SyntaxTree *type_token, const Type *cast_type,
  const SyntaxTree *expr, const Type *expr_type)
{
  char         *cast_type_string = type_to_string(cast_type);
  char         *expr_type_string = type_to_string(expr_type);
  unsigned long type_offset      = syntax_tree_offset(type_token);
  unsigned long type_length      = syntax_tree_text_length(type_token);
  unsigned long expr_offset      = syntax_tree_offset(expr);
  unsigned long expr_length      = syntax_tree_text_length(expr);

  Report *report = report_new(REPORT_KIND_ERROR, syntax_tree_offset(node), "non-standard type cast");
  if (!type_is_std(cast_type)) {
    report_annotation(report, type_offset, type_offset + type_length,
      "expression can be cast to `integer`, `boolean` or `char`");
  }
  if (!type_is_std(expr_type)) {
    report_annotation(report, expr_offset, expr_offset + expr_length,
      "expression to be cast should be `integer`, `boolean` or `char`");
  }
  array_push(checker->errors, &report);
  free(cast_type_string);
  free(expr_type_string);
}

static void check_cast_expr(Checker *checker, const MpplCastExpr *syntax)
{
  AnyMpplExpr    *expr_syntax = mppl_cast_expr__expr(syntax);
  AnyMpplStdType *type_syntax = mppl_cast_expr__type(syntax);
  const Type     *type        = get_expr_type(checker, (SyntaxTree *) expr_syntax);
  Type           *infer_type  = mppl_std_type__to_type(type_syntax);

  if (type) {
    if (!type_is_std(type)) {
      error_cast_expr_invalid_operand(checker,
        (SyntaxTree *) syntax, (SyntaxTree *) type_syntax, infer_type, (SyntaxTree *) expr_syntax, type);
      type_free(infer_type);
    } else {
      record_expr_type(checker, (SyntaxTree *) syntax, infer_type);
    }
  }
  mppl_free(expr_syntax);
  mppl_free(type_syntax);
}

static void check_entire_var(Checker *checker, const MpplEntireVar *syntax)
{
  MpplToken  *name_syntax = mppl_entire_var__name(syntax);
  const Type *type        = get_ref_type(checker, (SyntaxTree *) name_syntax);
  Type       *infer_type  = type_clone(type);
  record_expr_type(checker, (SyntaxTree *) syntax, infer_type);
  mppl_free(name_syntax);
}

static void error_non_array_subscript(Checker *checker, const SyntaxTree *node, const Type *type)
{
  char         *type_string = type_to_string(type);
  unsigned long offset      = syntax_tree_offset(node);
  unsigned long length      = syntax_tree_text_length(node);
  Report       *report      = report_new(REPORT_KIND_ERROR, offset, "variable of type `%s` cannot be indexed", type_string);
  report_annotation(report, offset, offset + length, "variable to be indexed should be an `array`");
  array_push(checker->errors, &report);
  free(type_string);
}

static void error_array_non_integer_index(Checker *checker, const SyntaxTree *node, const Type *type)
{
  char         *type_string = type_to_string(type);
  unsigned long offset      = syntax_tree_offset(node);
  unsigned long length      = syntax_tree_text_length(node);
  Report       *report      = report_new(REPORT_KIND_ERROR, offset, "array index should be an `integer`");
  report_annotation(report, offset, offset + length, "`%s`", type_string);
  array_push(checker->errors, &report);
  free(type_string);
}

static void check_indexed_var(Checker *checker, const MpplIndexedVar *syntax)
{
  MpplToken   *name_syntax  = mppl_indexed_var__name(syntax);
  AnyMpplExpr *index_syntax = mppl_indexed_var__expr(syntax);
  const Type  *def_type     = get_ref_type(checker, (SyntaxTree *) name_syntax);
  const Type  *index_type   = get_expr_type(checker, (SyntaxTree *) index_syntax);

  if (index_type) {
    if (type_kind(def_type) != TYPE_ARRAY) {
      error_non_array_subscript(checker, (SyntaxTree *) name_syntax, def_type);
    } else if (type_kind(index_type) != TYPE_INTEGER) {
      error_array_non_integer_index(checker, (SyntaxTree *) index_syntax, index_type);
    } else {
      const Type *elem_type  = type_array_elem((const TypeArray *) def_type);
      Type       *infer_type = type_clone(elem_type);
      record_expr_type(checker, (SyntaxTree *) syntax, infer_type);
    }
  }
  mppl_free(name_syntax);
  mppl_free(index_syntax);
}

static void check_number_lit(Checker *checker, const MpplLitNumber *syntax)
{
  Type *infer_type = type_new(TYPE_INTEGER);
  record_expr_type(checker, (SyntaxTree *) syntax, infer_type);
}

static void check_string_lit(Checker *checker, const MpplLitString *syntax)
{
  char *text = mppl_lit_string__to_string(syntax);
  Type *infer_type;
  if (strlen(text) == 1) {
    infer_type = type_new(TYPE_CHAR);
  } else {
    infer_type = type_new(TYPE_STRING);
  }
  record_expr_type(checker, (SyntaxTree *) syntax, infer_type);
  free(text);
}

static void check_boolean_lit(Checker *checker, const MpplLitBoolean *syntax)
{
  Type *infer_type = type_new(TYPE_BOOLEAN);
  record_expr_type(checker, (SyntaxTree *) syntax, infer_type);
}

static int visit_syntax_tree(const SyntaxTree *syntax, void *checker, int enter)
{
  if (enter) {
    switch (syntax_tree_kind(syntax)) {
    case SYNTAX_PROC_DECL:
      infer_proc(checker, (const MpplProcDecl *) syntax);
      return 0;

    case SYNTAX_VAR_DECL:
      infer_var(checker, (const MpplVarDecl *) syntax);
      return 0;

    default:
      return 1;
    }
  } else {
    switch (syntax_tree_kind(syntax)) {
    case SYNTAX_ASSIGN_STMT:
      check_assign_stmt(checker, (const MpplAssignStmt *) syntax);
      return 1;

    case SYNTAX_IF_STMT:
      check_if_stmt(checker, (const MpplIfStmt *) syntax);
      return 1;

    case SYNTAX_WHILE_STMT:
      check_while_stmt(checker, (const MpplWhileStmt *) syntax);
      return 1;

    case SYNTAX_CALL_STMT:
      check_call_stmt(checker, (const MpplCallStmt *) syntax);
      return 1;

    case SYNTAX_INPUT_STMT:
      check_input_stmt(checker, (const MpplInputStmt *) syntax);
      return 1;

    case SYNTAX_OUTPUT_STMT:
      check_output_stmt(checker, (const MpplOutputStmt *) syntax);
      return 1;

    case SYNTAX_BINARY_EXPR:
      check_binary_expr(checker, (const MpplBinaryExpr *) syntax);
      return 1;

    case SYNTAX_PAREN_EXPR:
      check_paren_expr(checker, (const MpplParenExpr *) syntax);
      return 1;

    case SYNTAX_NOT_EXPR:
      check_not_expr(checker, (const MpplNotExpr *) syntax);
      return 1;

    case SYNTAX_CAST_EXPR:
      check_cast_expr(checker, (const MpplCastExpr *) syntax);
      return 1;

    case SYNTAX_ENTIRE_VAR:
      check_entire_var(checker, (const MpplEntireVar *) syntax);
      return 1;

    case SYNTAX_INDEXED_VAR:
      check_indexed_var(checker, (const MpplIndexedVar *) syntax);
      return 1;

    case SYNTAX_NUMBER_LIT:
      check_number_lit(checker, (const MpplLitNumber *) syntax);
      return 1;

    case SYNTAX_STRING_LIT:
      check_string_lit(checker, (const MpplLitString *) syntax);
      return 1;

    case SYNTAX_TRUE_KW:
    case SYNTAX_FALSE_KW:
      check_boolean_lit(checker, (const MpplLitBoolean *) syntax);
      return 1;

    default:
      return 1;
    }
  }
}

int mppl_check(const Source *source, const TokenNode *tree, const Res *resolution, Infer **inference)
{
  SyntaxTree *syntax;
  Checker     checker;
  checker.res       = resolution;
  checker.inference = infer_new();
  checker.errors    = array_new(sizeof(Report *));

  syntax = syntax_tree_root((const TokenTree *) tree);
  syntax_tree_visit(syntax, &visit_syntax_tree, &checker);
  syntax_tree_free(syntax);

  if (array_count(checker.errors)) {
    unsigned long i;
    for (i = 0; i < array_count(checker.errors); ++i) {
      report_emit(*(Report **) array_at(checker.errors, i), source);
    }
    infer_free(checker.inference);
    *inference = NULL;
  } else {
    *inference = checker.inference;
  }
  array_free(checker.errors);
  return !!*inference;
}
