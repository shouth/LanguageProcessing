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
#include "string.h"
#include "syntax_kind.h"
#include "syntax_tree.h"
#include "type.h"
#include "utility.h"

typedef struct Checker Checker;

struct Checker {
  const Res *res;
  Infer     *inference;
  Array     *errors;
};

static const Type *get_ref_type(const Checker *checker, const SyntaxTree *tree)
{
  const Def *def = res_get_ref(checker->res, syntax_tree_raw(tree));
  if (def) {
    return infer_get_def_type(checker->inference, def);
  } else {
    unreachable();
  }
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

static const Type *check_expr(Checker *checker, const AnyMpplExpr *syntax);

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
  report_annotation(report, op_offset, op_offset + op_length, "operands of `%s` should be %s", op_str, type_str);
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
  report_annotation(report, op_offset, op_offset + op_length, "operands of `%s` should be %s", op_str, type_str);
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

  Report *report = report_new(REPORT_KIND_ERROR, syntax_tree_offset(node), "mismatched types");
  report_annotation(report, lhs_offset, lhs_offset + lhs_length, "`%s`", lhs_type_string);
  report_annotation(report, rhs_offset, rhs_offset + rhs_length, "`%s`", rhs_type_string);
  array_push(checker->errors, &report);

  free(lhs_type_string);
  free(rhs_type_string);
}

static const Type *check_binary_expr(Checker *checker, const MpplBinaryExpr *syntax)
{
  AnyMpplExpr *lhs_syntax = mppl_binary_expr__lhs(syntax);
  AnyMpplExpr *rhs_syntax = mppl_binary_expr__rhs(syntax);
  MpplToken   *op_syntax  = mppl_binary_expr__op_token(syntax);
  Type        *result     = NULL;

  if (lhs_syntax) {
    const Type *lhs_type = check_expr(checker, lhs_syntax);
    const Type *rhs_type = check_expr(checker, rhs_syntax);
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
            "one of `integer`, `char`, or `boolean`");
        } else if (!type_equal(lhs_type, rhs_type)) {
          error_relational_mismatched_type(checker,
            (SyntaxTree *) syntax, (SyntaxTree *) lhs_syntax, lhs_type, (SyntaxTree *) rhs_syntax, rhs_type);
        } else {
          result = type_new(TYPE_BOOLEAN);
          record_expr_type(checker, (SyntaxTree *) syntax, result);
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
          result = type_new(TYPE_INTEGER);
          record_expr_type(checker, (SyntaxTree *) syntax, result);
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
          result = type_new(TYPE_BOOLEAN);
          record_expr_type(checker, (SyntaxTree *) syntax, result);
        }
        break;

      default:
        unreachable();
      }
    }
  } else {
    const Type *rhs_type = check_expr(checker, rhs_syntax);
    if (rhs_type) {
      switch (syntax_tree_kind((SyntaxTree *) op_syntax)) {
      case SYNTAX_PLUS_TOKEN:
      case SYNTAX_MINUS_TOKEN:
        if (type_kind(rhs_type) != TYPE_INTEGER) {
          error_unary_expr_invalid_operand(checker,
            (SyntaxTree *) syntax, (SyntaxTree *) op_syntax, (SyntaxTree *) rhs_syntax, rhs_type, "`integer`");
        } else {
          result = type_new(TYPE_INTEGER);
          record_expr_type(checker, (SyntaxTree *) syntax, result);
        }
        break;

      default:
        unreachable();
      }
    }
  }

  mppl_free(op_syntax);
  mppl_free(lhs_syntax);
  mppl_free(rhs_syntax);
  return result;
}

static const Type *check_paren_expr(Checker *checker, const MpplParenExpr *syntax)
{
  AnyMpplExpr *expr_syntax = mppl_paren_expr__expr(syntax);
  const Type  *type        = check_expr(checker, expr_syntax);
  Type        *result      = NULL;

  if (type) {
    result = type_clone(type);
    record_expr_type(checker, (SyntaxTree *) syntax, result);
  }

  mppl_free(expr_syntax);
  return result;
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

static const Type *check_not_expr(Checker *checker, const MpplNotExpr *syntax)
{
  AnyMpplExpr *expr_syntax = mppl_not_expr__expr(syntax);
  const Type  *type        = check_expr(checker, expr_syntax);
  Type        *result      = NULL;

  if (type) {
    if (type_kind(type) != TYPE_BOOLEAN) {
      MpplToken *not_token = mppl_not_expr__not_token(syntax);
      error_not_expr_invalid_operand(checker,
        (SyntaxTree *) syntax, (SyntaxTree *) not_token, (SyntaxTree *) expr_syntax, type);
      mppl_free(not_token);
    } else {
      result = type_new(TYPE_BOOLEAN);
      record_expr_type(checker, (SyntaxTree *) syntax, result);
    }
  }

  mppl_free(expr_syntax);
  return result;
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
      "expression can be cast to one of `integer`, `boolean`, or `char`");
  }
  if (!type_is_std(expr_type)) {
    report_annotation(report, expr_offset, expr_offset + expr_length,
      "expression to be cast should be one of `integer`, `boolean`, or `char`");
  }
  array_push(checker->errors, &report);
  free(cast_type_string);
  free(expr_type_string);
}

static const Type *check_cast_expr(Checker *checker, const MpplCastExpr *syntax)
{
  AnyMpplExpr    *expr_syntax = mppl_cast_expr__expr(syntax);
  AnyMpplStdType *type_syntax = mppl_cast_expr__type(syntax);
  const Type     *type        = check_expr(checker, expr_syntax);
  Type           *cast_type   = mppl_std_type__to_type(type_syntax);
  Type           *result      = NULL;

  if (type) {
    if (!type_is_std(type)) {
      error_cast_expr_invalid_operand(checker,
        (SyntaxTree *) syntax, (SyntaxTree *) type_syntax, cast_type, (SyntaxTree *) expr_syntax, type);
      type_free(cast_type);
    } else {
      result = cast_type;
      record_expr_type(checker, (SyntaxTree *) syntax, result);
    }
  }

  mppl_free(expr_syntax);
  mppl_free(type_syntax);
  return result;
}

static const Type *check_entire_var(Checker *checker, const MpplEntireVar *syntax)
{
  MpplToken  *name_syntax = mppl_entire_var__name(syntax);
  const Type *type        = get_ref_type(checker, (SyntaxTree *) name_syntax);

  if (type) {
    Type *infer_type = type_clone(type);
    record_expr_type(checker, (SyntaxTree *) syntax, infer_type);
  }

  mppl_free(name_syntax);
  return type;
}

static void error_non_array_subscript(Checker *checker, const SyntaxTree *node, const Type *type)
{
  char         *type_string = type_to_string(type);
  unsigned long offset      = syntax_tree_offset(node);
  unsigned long length      = syntax_tree_text_length(node);

  Report *report = report_new(REPORT_KIND_ERROR, offset, "variable of type `%s` cannot be indexed", type_string);
  report_annotation(report, offset, offset + length, "variable to be indexed should be an `array`");
  array_push(checker->errors, &report);
  free(type_string);
}

static void error_array_non_integer_index(Checker *checker, const SyntaxTree *node, const Type *type)
{
  char         *type_string = type_to_string(type);
  unsigned long offset      = syntax_tree_offset(node);
  unsigned long length      = syntax_tree_text_length(node);

  Report *report = report_new(REPORT_KIND_ERROR, offset, "array index should be an `integer`");
  report_annotation(report, offset, offset + length, "`%s`", type_string);
  array_push(checker->errors, &report);
  free(type_string);
}

static const Type *check_indexed_var(Checker *checker, const MpplIndexedVar *syntax)
{
  MpplToken   *name_syntax  = mppl_indexed_var__name(syntax);
  AnyMpplExpr *index_syntax = mppl_indexed_var__expr(syntax);
  const Type  *def_type     = get_ref_type(checker, (SyntaxTree *) name_syntax);
  const Type  *index_type   = check_expr(checker, index_syntax);
  Type        *result       = NULL;

  if (def_type && index_type) {
    if (type_kind(def_type) != TYPE_ARRAY) {
      error_non_array_subscript(checker, (SyntaxTree *) name_syntax, def_type);
    } else if (type_kind(index_type) != TYPE_INTEGER) {
      error_array_non_integer_index(checker, (SyntaxTree *) index_syntax, index_type);
    } else {
      result = type_clone(type_array_elem((const TypeArray *) def_type));
      record_expr_type(checker, (SyntaxTree *) syntax, result);
    }
  }

  mppl_free(name_syntax);
  mppl_free(index_syntax);
  return result;
}

static const Type *check_var(Checker *checker, const AnyMpplVar *syntax)
{
  switch (mppl_var__kind(syntax)) {
  case MPPL_VAR_ENTIRE:
    return check_entire_var(checker, (const MpplEntireVar *) syntax);

  case MPPL_VAR_INDEXED:
    return check_indexed_var(checker, (const MpplIndexedVar *) syntax);

  default:
    unreachable();
  }
}

static const Type *check_literal(Checker *checker, const AnyMpplLit *syntax)
{
  switch (mppl_lit__kind(syntax)) {
  case MPPL_LIT_NUMBER: {
    Type *result = type_new(TYPE_INTEGER);
    record_expr_type(checker, (SyntaxTree *) syntax, result);
    return result;
  }

  case MPPL_LIT_STRING: {
    char *text   = mppl_lit_string__to_string((const MpplStringLit *) syntax);
    Type *result = type_new(strlen(text) == 1 ? TYPE_CHAR : TYPE_STRING);
    record_expr_type(checker, (SyntaxTree *) syntax, result);
    free(text);
    record_expr_type(checker, (SyntaxTree *) syntax, result);
    return result;
  }

  case MPPL_LIT_BOOLEAN: {
    Type *result = type_new(TYPE_BOOLEAN);
    record_expr_type(checker, (SyntaxTree *) syntax, result);
    return result;
  }

  default:
    unreachable();
  }
}

static const Type *check_expr(Checker *checker, const AnyMpplExpr *syntax)
{
  switch (mppl_expr__kind(syntax)) {
  case MPPL_EXPR_BINARY:
    return check_binary_expr(checker, (const MpplBinaryExpr *) syntax);

  case MPPL_EXPR_PAREN:
    return check_paren_expr(checker, (const MpplParenExpr *) syntax);

  case MPPL_EXPR_NOT:
    return check_not_expr(checker, (const MpplNotExpr *) syntax);

  case MPPL_EXPR_CAST:
    return check_cast_expr(checker, (const MpplCastExpr *) syntax);

  case MPPL_EXPR_VAR:
    return check_var(checker, (const AnyMpplVar *) syntax);

  case MPPL_EXPR_LIT:
    return check_literal(checker, (const AnyMpplLit *) syntax);

  default:
    unreachable();
  }
}

static void visit_var_decl(const MpplAstWalker *walker, const MpplVarDecl *syntax, void *checker)
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
  mppl_ast__walk_var_decl(walker, syntax, checker);
  (void) walker;
}

static void error_proc_param_type(Checker *checker, const MpplFmlParamList *syntax)
{
  unsigned long i;
  unsigned long offset = syntax_tree_offset((SyntaxTree *) syntax);
  Report       *report = report_new(REPORT_KIND_ERROR, offset, "invalid parameter type");

  for (i = 0; i < mppl_fml_param_list__sec_count(syntax); ++i) {
    MpplFmlParamSec *param_sec_syntax = mppl_fml_param_list__sec(syntax, i);
    AnyMpplType     *type_syntax      = mppl_fml_param_sec__type(param_sec_syntax);
    Type            *type             = mppl_type__to_type(type_syntax);

    if (!type_is_std(type)) {
      unsigned long type_offset = syntax_tree_offset((SyntaxTree *) type_syntax);
      unsigned long type_length = syntax_tree_text_length((SyntaxTree *) type_syntax);
      char         *type_string = type_to_string(type);
      report_annotation(report, type_offset, type_offset + type_length,
        "parameter type should be one of `integer`, `boolean`, or `char`", type_string);
      free(type_string);
    }

    type_free(type);
    mppl_free(type_syntax);
    mppl_free(param_sec_syntax);
  }
  array_push(checker->errors, &report);
}

static void visit_proc_decl(const MpplAstWalker *walker, const MpplProcDecl *syntax, void *checker)
{
  unsigned long i, j;
  Array        *params = array_new(sizeof(Type *));
  unsigned long param_count;
  Type        **param_types;
  Type         *infer_type;
  int           needs_report = 0;

  MpplFmlParamList *param_list_syntax = mppl_proc_decl__fml_param_list(syntax);
  if (param_list_syntax) {
    for (i = 0; i < mppl_fml_param_list__sec_count(param_list_syntax); ++i) {
      MpplFmlParamSec *param_sec_syntax = mppl_fml_param_list__sec(param_list_syntax, i);
      AnyMpplType     *type_syntax      = mppl_fml_param_sec__type(param_sec_syntax);
      Type            *type             = mppl_type__to_type(type_syntax);

      if (!type_is_std(type)) {
        needs_report = 1;
      } else {
        for (j = 0; j < mppl_fml_param_sec__name_count(param_sec_syntax); ++j) {
          MpplToken *name_syntax = mppl_fml_param_sec__name(param_sec_syntax, j);
          Type      *param_type  = type_clone(type);
          Type      *infer_type  = type_clone(type);
          array_push(params, &param_type);
          record_def_type(checker, (SyntaxTree *) name_syntax, infer_type);
          mppl_free(name_syntax);
        }
      }

      type_free(type);
      mppl_free(type_syntax);
      mppl_free(param_sec_syntax);
    }
  }

  param_count = array_count(params);
  param_types = array_steal(params);
  infer_type  = type_new_proc(param_types, param_count);
  if (needs_report) {
    type_free(infer_type);
    error_proc_param_type(checker, param_list_syntax);
  } else {
    MpplToken *name_syntax = mppl_proc_decl__name(syntax);
    record_def_type(checker, (SyntaxTree *) name_syntax, infer_type);
    mppl_free(name_syntax);
  }

  mppl_free(param_list_syntax);
  mppl_ast__walk_proc_decl(walker, syntax, checker);
  (void) walker;
}

static void error_assign_impossible(const Checker *checker, const SyntaxTree *lhs, const Type *lhs_type)
{
  char         *lhs_type_string = type_to_string(lhs_type);
  unsigned long offset          = syntax_tree_offset(lhs);
  unsigned long length          = syntax_tree_text_length(lhs);

  Report *report = report_new(REPORT_KIND_ERROR, offset,
    "assignment operation cannot be applied to `%s`", lhs_type_string);
  report_annotation(report, offset, offset + length,
    "left operand of `:=` should be a variable of standard type", lhs_type_string);
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

  Report *report = report_new(REPORT_KIND_ERROR, syntax_tree_offset(node), "mismatched types");
  report_annotation(report, lhs_offset, lhs_offset + lhs_length,
    "`%s`", lhs_type_string);
  report_annotation(report, rhs_offset, rhs_offset + rhs_length,
    "expected `%s`, found `%s`", lhs_type_string, rhs_type_string);

  array_push(checker->errors, &report);
  free(lhs_type_string);
  free(rhs_type_string);
}

static void visit_assign_stmt(const MpplAstWalker *walker, const MpplAssignStmt *syntax, void *checker)
{
  AnyMpplVar  *lhs_syntax = mppl_assign_stmt__lhs(syntax);
  AnyMpplExpr *rhs_syntax = mppl_assign_stmt__rhs(syntax);
  const Type  *lhs_type   = check_var(checker, lhs_syntax);
  const Type  *rhs_type   = check_expr(checker, rhs_syntax);

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
  (void) walker;
}

static void error_conditional_stmt_invalid_condition(Checker *checker, const SyntaxTree *node, const Type *type)
{
  char         *type_string = type_to_string(type);
  unsigned long offset      = syntax_tree_offset(node);
  unsigned long length      = syntax_tree_text_length(node);

  Report *report = report_new(REPORT_KIND_ERROR, offset, "`%s` cannot be used as a condition", type_string);
  report_annotation(report, offset, offset + length, "condition should be `boolean`");
  array_push(checker->errors, &report);
  free(type_string);
}

static void visit_if_stmt(const MpplAstWalker *walker, const MpplIfStmt *syntax, void *checker)
{
  AnyMpplExpr *cond_syntax = mppl_if_stmt__cond(syntax);
  const Type  *cond_type   = check_expr(checker, cond_syntax);

  if (cond_type && type_kind(cond_type) != TYPE_BOOLEAN) {
    error_conditional_stmt_invalid_condition(checker, (SyntaxTree *) cond_syntax, cond_type);
  }

  mppl_free(cond_syntax);
  (void) walker;
}

static void visit_while_stmt(const MpplAstWalker *walker, const MpplWhileStmt *syntax, void *checker)
{
  AnyMpplExpr *cond_syntax = mppl_while_stmt__cond(syntax);
  const Type  *cond_type   = check_expr(checker, cond_syntax);

  if (cond_type && type_kind(cond_type) != TYPE_BOOLEAN) {
    error_conditional_stmt_invalid_condition(checker, (SyntaxTree *) cond_syntax, cond_type);
  }

  mppl_free(cond_syntax);
  (void) walker;
}

static void error_call_stmt_non_callable(Checker *checker, const MpplToken *syntax)
{
  const RawSyntaxToken *token  = (const RawSyntaxToken *) syntax_tree_raw((SyntaxTree *) syntax);
  unsigned long         offset = syntax_tree_offset((SyntaxTree *) syntax);
  unsigned long         length = syntax_tree_text_length((SyntaxTree *) syntax);

  Report *report = report_new(REPORT_KIND_ERROR, offset, "`%s` cannot be called", string_data(token->string));
  report_annotation(report, offset, offset + length, "a called item should be a `procedure`");
  array_push(checker->errors, &report);
}

static void error_call_stmt_mismatched_param_count(
  Checker *checker, const MpplActParamList *syntax,
  const TypeProc *proc_type, unsigned long act_param_count)
{
  unsigned long offset      = syntax_tree_offset((SyntaxTree *) syntax);
  unsigned long length      = syntax_tree_text_length((SyntaxTree *) syntax);
  unsigned long param_count = type_proc_param_count(proc_type);

  Report *report = report_new(REPORT_KIND_ERROR, offset, "mismatched the number of parameter");
  report_annotation(report, offset, offset + length,
    "expected %lu %s, found %lu", param_count, param_count > 1 ? "parameters" : "parameter", act_param_count);
  array_push(checker->errors, &report);
}

static void error_call_stmt_mismatched_param_type(
  Checker *checker, const MpplToken *name_syntax, const MpplActParamList *act_param_list_syntax,
  const TypeProc *defined_type, const TypeProc *act_type)
{
  unsigned long i;
  unsigned long offset = syntax_tree_offset((SyntaxTree *) act_param_list_syntax);

  Report *report = report_new(REPORT_KIND_ERROR, offset, "mismatched parameter type");
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
  array_push(checker->errors, &report);

  {
    const Def            *def                   = res_get_ref(checker->res, syntax_tree_raw((const SyntaxTree *) name_syntax));
    const MpplProcDecl   *proc_decl_syntax      = (const MpplProcDecl *) def->body;
    MpplToken            *id_syntax             = mppl_proc_decl__name(proc_decl_syntax);
    const RawSyntaxToken *id_token              = (const RawSyntaxToken *) syntax_tree_raw((SyntaxTree *) id_syntax);
    MpplFmlParamList     *fml_param_list_syntax = mppl_proc_decl__fml_param_list(proc_decl_syntax);
    unsigned long         name_offset           = syntax_tree_offset((SyntaxTree *) id_syntax);
    unsigned long         name_length           = syntax_tree_text_length((SyntaxTree *) id_syntax);

    Report *report = report_new(REPORT_KIND_NOTE, name_offset, "`%s` is defined here", string_data(id_token->string));
    report_annotation(report, name_offset, name_offset + name_length, NULL);
    for (i = 0; i < mppl_fml_param_list__sec_count(fml_param_list_syntax); ++i) {
      MpplFmlParamSec *fml_param_sec_syntax = mppl_fml_param_list__sec(fml_param_list_syntax, i);
      AnyMpplType     *type_syntax          = mppl_fml_param_sec__type(fml_param_sec_syntax);
      unsigned long    type_offset          = syntax_tree_offset((SyntaxTree *) type_syntax);
      unsigned long    type_length          = syntax_tree_text_length((SyntaxTree *) type_syntax);
      report_annotation(report, type_offset, type_offset + type_length, NULL);
      mppl_free(type_syntax);
      mppl_free(fml_param_sec_syntax);
    }
    array_push(checker->errors, &report);
    mppl_free(fml_param_list_syntax);
    mppl_free(id_syntax);
  }
}

static void maybe_error_call_stmt_wrong_param(
  Checker *checker, const MpplToken *name, const MpplActParamList *param_list, const TypeProc *type)
{
  Array *act_param_array = array_new(sizeof(Type *));
  int    expr_error      = 0;

  if (param_list) {
    unsigned long i;
    for (i = 0; i < mppl_act_param_list__expr_count(param_list); ++i) {
      AnyMpplExpr *act_param_syntax = mppl_act_param_list__expr(param_list, i);
      const Type  *act_param_type   = check_expr(checker, act_param_syntax);

      if (!act_param_type) {
        expr_error = 1;
      } else {
        Type *act_param_clone = type_clone(act_param_type);
        array_push(act_param_array, &act_param_clone);
      }
      mppl_free(act_param_syntax);
    }
  }

  if (!expr_error) {
    unsigned long act_param_count = array_count(act_param_array);
    Type        **act_param_types = array_steal(act_param_array);
    Type         *act_type        = type_new_proc(act_param_types, act_param_count);
    if (!type_equal((const Type *) type, act_type)) {
      error_call_stmt_mismatched_param_type(checker, name, param_list, type, (const TypeProc *) act_type);
    }
    type_free(act_type);
  }
}

static void visit_call_stmt(const MpplAstWalker *walker, const MpplCallStmt *syntax, void *checker)
{
  MpplToken  *name_syntax = mppl_call_stmt__name(syntax);
  const Type *type        = get_ref_type(checker, (SyntaxTree *) name_syntax);

  if (type) {
    MpplActParamList *act_param_list_syntax = mppl_call_stmt__act_param_list(syntax);
    unsigned long     param_count           = act_param_list_syntax ? mppl_act_param_list__expr_count(act_param_list_syntax) : 0;
    const TypeProc   *type_proc             = (const TypeProc *) type;

    if (type_kind(type) != TYPE_PROC) {
      error_call_stmt_non_callable(checker, name_syntax);
    } else if (param_count != type_proc_param_count(type_proc)) {
      error_call_stmt_mismatched_param_count(checker, act_param_list_syntax, type_proc, param_count);
    } else {
      maybe_error_call_stmt_wrong_param(checker, name_syntax, act_param_list_syntax, type_proc);
    }

    mppl_free(act_param_list_syntax);
  }

  mppl_free(name_syntax);
  (void) walker;
}

static void error_input_stmt_invalid_operand(Checker *checker, const MpplInputList *syntax)
{
  unsigned long i;
  unsigned long offset = syntax_tree_offset((SyntaxTree *) syntax);

  Report *report = report_new(REPORT_KIND_ERROR, offset, "invalid operand");
  for (i = 0; i < mppl_input_list__var_count(syntax); ++i) {
    AnyMpplVar *var_syntax = mppl_input_list__var(syntax, i);
    const Type *var_type   = check_var(checker, var_syntax);

    if (type_kind(var_type) != TYPE_INTEGER && type_kind(var_type) != TYPE_CHAR) {
      unsigned long var_offset      = syntax_tree_offset((SyntaxTree *) var_syntax);
      unsigned long var_length      = syntax_tree_text_length((SyntaxTree *) var_syntax);
      char         *var_type_string = type_to_string(var_type);
      report_annotation(report, var_offset, var_offset + var_length,
        "expected one of `integer`, or `char`, found `%s`", var_type_string);
      free(var_type_string);
    }
  }
}

static void visit_input_stmt(const MpplAstWalker *walker, const MpplInputStmt *syntax, void *checker)
{
  unsigned long  i;
  MpplInputList *input_list_syntax = mppl_input_stmt__input_list(syntax);

  if (input_list_syntax) {
    int needs_report = 0;
    for (i = 0; i < mppl_input_list__var_count(input_list_syntax); ++i) {
      AnyMpplVar *var_syntax = mppl_input_list__var(input_list_syntax, i);
      const Type *var_type   = check_var(checker, var_syntax);
      needs_report |= var_type && type_kind(var_type) != TYPE_INTEGER && type_kind(var_type) != TYPE_CHAR;
      mppl_free(var_syntax);

      if (!var_type) {
        needs_report = 0;
        break;
      }
    }

    if (needs_report) {
      error_input_stmt_invalid_operand(checker, input_list_syntax);
    }
  }

  mppl_free(input_list_syntax);
  (void) walker;
}

static void error_output_stmt_invalid_operand(Checker *checker, const MpplOutList *syntax)
{
  unsigned long i;
  unsigned long offset = syntax_tree_offset((SyntaxTree *) syntax);

  Report *report = report_new(REPORT_KIND_ERROR, offset, "invalid operand");
  for (i = 0; i < mppl_out_list__out_value_count(syntax); ++i) {
    MpplOutValue  *out_value_syntax = mppl_out_list__out_value(syntax, i);
    MpplToken     *colon_syntax     = mppl_out_value__colon_token(out_value_syntax);
    MpplNumberLit *width_syntax     = mppl_out_value__width(out_value_syntax);
    AnyMpplExpr   *expr_syntax      = mppl_out_value__expr(out_value_syntax);
    const Type    *expr_type        = check_expr(checker, expr_syntax);

    if (!type_is_std(expr_type)) {
      if (type_kind(expr_type) != TYPE_STRING) {
        unsigned long expr_offset      = syntax_tree_offset((SyntaxTree *) expr_syntax);
        unsigned long expr_length      = syntax_tree_text_length((SyntaxTree *) expr_syntax);
        char         *expr_type_string = type_to_string(expr_type);
        report_annotation(report, expr_offset, expr_offset + expr_length,
          "expected one of `integer`, `char`, or `boolean`, found `%s`", expr_type_string);
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

static void visit_output_stmt(const MpplAstWalker *walker, const MpplOutputStmt *syntax, void *checker)
{
  unsigned long i;
  MpplOutList  *out_list_syntax = mppl_output_stmt__output_list(syntax);

  if (out_list_syntax) {
    int needs_report = 0;
    for (i = 0; i < mppl_out_list__out_value_count(out_list_syntax); ++i) {
      MpplOutValue *out_value_syntax = mppl_out_list__out_value(out_list_syntax, i);
      MpplToken    *colon_syntax     = mppl_out_value__colon_token(out_value_syntax);
      AnyMpplExpr  *expr_syntax      = mppl_out_value__expr(out_value_syntax);
      const Type   *expr_type        = check_expr(checker, expr_syntax);
      needs_report |= expr_type && !type_is_std(expr_type) && (type_kind(expr_type) != TYPE_STRING || colon_syntax);

      mppl_free(expr_syntax);
      mppl_free(colon_syntax);
      mppl_free(out_value_syntax);

      if (!expr_type) {
        needs_report = 0;
        break;
      }
    }

    if (needs_report) {
      error_output_stmt_invalid_operand(checker, out_list_syntax);
    }
  }

  mppl_free(out_list_syntax);
  (void) walker;
}

static void error_array_type_invalid_size(Checker *checker, const MpplNumberLit *size_syntax)
{
  unsigned long offset = syntax_tree_offset((SyntaxTree *) size_syntax);
  unsigned long length = syntax_tree_text_length((SyntaxTree *) size_syntax);

  Report *report = report_new(REPORT_KIND_ERROR, offset, "invalid array size");
  report_annotation(report, offset, offset + length, "array size should be greater than 0");
  array_push(checker->errors, &report);
}

static void visit_array_type(const MpplAstWalker *walker, const MpplArrayType *syntax, void *checker)
{
  MpplNumberLit *size_syntax = mppl_array_type__size(syntax);
  if (mppl_lit_number__to_long(size_syntax) == 0) {
    error_array_type_invalid_size(checker, size_syntax);
  }
  mppl_free(size_syntax);
  (void) walker;
}

int mppl_check(const Source *source, const MpplProgram *syntax, const Res *resolution, Infer **inference)
{
  MpplAstWalker walker;
  Checker       checker;
  checker.res       = resolution;
  checker.inference = infer_new();
  checker.errors    = array_new(sizeof(Report *));

  mppl_ast_walker__setup(&walker);
  walker.visit_var_decl    = &visit_var_decl;
  walker.visit_proc_decl   = &visit_proc_decl;
  walker.visit_assign_stmt = &visit_assign_stmt;
  walker.visit_if_stmt     = &visit_if_stmt;
  walker.visit_while_stmt  = &visit_while_stmt;
  walker.visit_call_stmt   = &visit_call_stmt;
  walker.visit_input_stmt  = &visit_input_stmt;
  walker.visit_output_stmt = &visit_output_stmt;
  walker.visit_array_type  = &visit_array_type;
  mppl_ast_walker__travel(&walker, syntax, &checker);

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
