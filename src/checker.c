#include <stddef.h>
#include <stdio.h>

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

static void error_assign_impossible(const Checker *checker, const SyntaxTree *lhs, const Type *lhs_type)
{
  char         *lhs_type_string = type_to_string(lhs_type);
  unsigned long offset          = syntax_tree_offset(lhs);
  unsigned long length          = syntax_tree_text_length(lhs);
  Report       *report          = report_new(REPORT_KIND_ERROR, offset, "`%s` is not assignable", lhs_type_string);
  report_annotation(report, offset, offset + length,
    "left hand side of assignment statement should be of standard type", lhs_type_string);
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
    "variable of type `%s`", lhs_type_string);
  report_annotation(report, rhs_offset, rhs_offset + rhs_length,
    "expected type `%s`, but found type `%s`", lhs_type_string, rhs_type_string);

  array_push(checker->errors, &report);
  free(lhs_type_string);
  free(rhs_type_string);
}

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

static void record_def_type(Checker *checker, const SyntaxTree *tree, Type *type)
{
  const Def *def = res_get_def(checker->res, syntax_tree_raw(tree));
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
    Type *type = mppl_type__to_type(type_syntax);
    printf("recording var\n");
    record_def_type(checker, (const SyntaxTree *) syntax, type);
  }
  mppl_free(type_syntax);
}

static void infer_param(Checker *checker, const MpplFmlParamSec *syntax)
{
  unsigned long i;
  AnyMpplType  *type_syntax = mppl_fml_param_sec__type(syntax);
  for (i = 0; i < mppl_fml_param_sec__name_count(syntax); ++i) {
    Type *type = mppl_type__to_type(type_syntax);
    printf("recording param\n");
    record_def_type(checker, (SyntaxTree *) syntax, type);
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
        const Type *param_type = get_def_type(checker, (SyntaxTree *) param_sec_syntax);
        Type       *infer_type = type_clone(param_type);
        array_push(params, &infer_type);
      }
      mppl_free(param_sec_syntax);
    }
    mppl_free(param_list_syntax);
  }

  {
    unsigned long param_count = array_count(params);
    Type        **param_types = array_steal(params);
    Type         *infer_type  = type_new_proc(param_types, param_count);
    printf("recording proc\n");
    record_def_type(checker, (SyntaxTree *) syntax, infer_type);
  }
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
      error_assign_type_mismatch(checker, (SyntaxTree *) syntax, (SyntaxTree *) lhs_syntax, lhs_type, (SyntaxTree *) rhs_syntax, rhs_type);
    }
  }
  mppl_free(lhs_syntax);
  mppl_free(rhs_syntax);
}

static void check_binary_expr(Checker *checker, const MpplBinaryExpr *syntax)
{
  AnyMpplExpr *lhs_syntax = mppl_binary_expr__lhs(syntax);
  AnyMpplExpr *rhs_syntax = mppl_binary_expr__rhs(syntax);
  MpplToken   *op_syntax  = mppl_binary_expr__op_token(syntax);
  const Type  *lhs_type   = get_expr_type(checker, (SyntaxTree *) lhs_syntax);
  const Type  *rhs_type   = get_expr_type(checker, (SyntaxTree *) rhs_syntax);

  if (lhs_type && rhs_type) {
    switch (syntax_tree_kind((SyntaxTree *) op_syntax)) {
    case SYNTAX_EQUAL_TOKEN:
    case SYNTAX_NOTEQ_TOKEN:
    case SYNTAX_LESS_TOKEN:
    case SYNTAX_LESSEQ_TOKEN:
    case SYNTAX_GREATER_TOKEN:
    case SYNTAX_GREATEREQ_TOKEN:
      if (!type_is_std(lhs_type) || !type_is_std(rhs_type)) {

      } else if (!type_equal(lhs_type, rhs_type)) {

      } else {
        Type *infer_type = type_new(TYPE_BOOLEAN);
        record_expr_type(checker, (SyntaxTree *) syntax, infer_type);
      }
      break;

    case SYNTAX_PLUS_TOKEN:
    case SYNTAX_MINUS_TOKEN:
    case SYNTAX_STAR_TOKEN:
    case SYNTAX_DIV_KW:
      if (type_kind(lhs_type) != TYPE_INTEGER || type_kind(rhs_type) != TYPE_INTEGER) {

      } else {
        Type *infer_type = type_new(TYPE_INTEGER);
        record_expr_type(checker, (SyntaxTree *) syntax, infer_type);
      }
      break;

    case SYNTAX_AND_KW:
    case SYNTAX_OR_KW:
      if (type_kind(lhs_type) != TYPE_BOOLEAN || type_kind(rhs_type) != TYPE_BOOLEAN) {

      } else {
        Type *infer_type = type_new(TYPE_BOOLEAN);
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

static void check_not_expr(Checker *checker, const MpplNotExpr *syntax)
{
  AnyMpplExpr *expr_syntax = mppl_not_expr__expr(syntax);
  const Type  *type        = get_expr_type(checker, (SyntaxTree *) expr_syntax);

  if (type) {
    if (type_kind(type) != TYPE_BOOLEAN) {

    } else {
      Type *infer_type = type_new(TYPE_BOOLEAN);
      record_expr_type(checker, (SyntaxTree *) syntax, infer_type);
    }
  }
  mppl_free(expr_syntax);
}

static void check_cast_expr(Checker *checker, const MpplCastExpr *syntax)
{
  AnyMpplExpr    *expr_syntax = mppl_cast_expr__expr(syntax);
  AnyMpplStdType *type_syntax = mppl_cast_expr__type(syntax);
  const Type     *type        = get_expr_type(checker, (SyntaxTree *) expr_syntax);
  Type           *infer_type  = mppl_std_type__to_type(type_syntax);

  if (type) {
    if (!type_is_std(type)) {

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

  if (type) {
    if (!type_is_std(type)) {

    } else {
      Type *infer_type = type_new(type_kind(type));
      record_expr_type(checker, (SyntaxTree *) syntax, infer_type);
    }
  }
  mppl_free(name_syntax);
}

static void check_indexed_var(Checker *checker, const MpplIndexedVar *syntax)
{
  MpplToken   *name_syntax  = mppl_indexed_var__name(syntax);
  AnyMpplExpr *index_syntax = mppl_indexed_var__expr(syntax);
  const Type  *def_type     = get_ref_type(checker, (SyntaxTree *) name_syntax);
  const Type  *index_type   = get_expr_type(checker, (SyntaxTree *) index_syntax);

  if (def_type && index_type) {
    if (type_kind(def_type) != TYPE_ARRAY) {

    } else if (type_kind(index_type) != TYPE_INTEGER) {

    } else {
      const Type *elem_type  = type_array_elem((const TypeArray *) def_type);
      Type       *infer_type = type_clone(elem_type);
      record_expr_type(checker, (SyntaxTree *) syntax, infer_type);
    }
  }
  mppl_free(name_syntax);
  mppl_free(index_syntax);
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

    case SYNTAX_BINARY_EXPR:
      check_binary_expr(checker, (const MpplBinaryExpr *) syntax);
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
    array_free(checker.errors);
    infer_free(checker.inference);
    *inference = NULL;
  } else {
    *inference = checker.inference;
  }
  array_free(checker.errors);
  return !!*inference;
}
