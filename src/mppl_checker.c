/* SPDX-License-Identifier: Apache-2.0 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "diag.h"
#include "mppl.h"
#include "mppl_semantic.h"
#include "mppl_syntax.h"
#include "report.h"
#include "syntax_tree.h"
#include "ty_ctxt.h"
#include "util.h"

typedef struct Value   Value;
typedef struct Checker Checker;

struct Value {
  const Ty *ty;
  enum ValueKind {
    VALUE_PLACE,
    VALUE_TEMP
  } kind;
};

struct Checker {
  const MpplSemantics *semantics;
  TyCtxt              *ctxt;
  Vec(Report *) diags;
  const RawSyntaxToken *proc_name;
};

static int ty_is_std(const Ty *ty)
{
  switch (ty->kind) {
  case TY_INTEGER:
  case TY_BOOLEAN:
  case TY_CHAR:
    return 1;

  default:
    return 0;
  }
}

static void set_ty_to_bind(Checker *checker, const Ty *ty, const MpplBindIdent *bind_ident)
{
  MpplBindIdentFields bind_ident_fields = mppl_bind_ident_fields_alloc(bind_ident);

  ty_ctxt_set(checker->ctxt, (const RawSyntaxNode *) bind_ident_fields.ident->raw, ty);
  mppl_bind_ident_fields_free(&bind_ident_fields);
}

static const Ty *get_ty_from_ref(Checker *checker, const MpplRefIdent *ref_ident)
{
  HashMapEntry       entry;
  const MpplBinding *binding;

  MpplRefIdentFields ref_ident_fields = mppl_ref_ident_fields_alloc(ref_ident);

  hashmap_entry(&checker->semantics->ref, &ref_ident_fields.ident->node.span.offset, &entry);
  binding = *hashmap_value(&checker->semantics->ref, &entry);

  mppl_ref_ident_fields_free(&ref_ident_fields);
  return ty_ctxt_get(checker->ctxt, (const RawSyntaxNode *) binding->binding->raw);
}

static void error_mismathced_type(Checker *checker, const AnyMpplExpr *expr, const Ty *expected, const Ty *found)
{
  if (expected->kind != TY_ERROR && found->kind != TY_ERROR) {
    const SyntaxTree *syntax = (const SyntaxTree *) expr;
    unsigned long     offset = syntax->node.span.offset;
    unsigned long     length = syntax->raw->node.span.text_length;
    Report           *report = diag_mismatched_type_error(offset, length, expected, found);
    vec_push(&checker->diags, &report, 1);
  }
}

static void error_non_standard_type(Checker *checker, const AnyMpplExpr *expr, const Ty *found)
{
  if (found->kind != TY_ERROR) {
    const SyntaxTree *syntax = (const SyntaxTree *) expr;
    unsigned long     offset = syntax->node.span.offset;
    unsigned long     length = syntax->raw->node.span.text_length;
    Report           *report = diag_non_standard_type_error(offset, length, found);
    vec_push(&checker->diags, &report, 1);
  }
}

static const Ty *check_type(Checker *checker, const AnyMpplType *type)
{
  switch (mppl_type_kind(type)) {
  case MPPL_TYPE_SYNTAX_INTEGER:
    return ty_integer();

  case MPPL_TYPE_SYNTAX_BOOLEAN:
    return ty_boolean();

  case MPPL_TYPE_SYNTAX_CHAR:
    return ty_char();

  case MPPL_TYPE_SYNTAX_ARRAY: {
    MpplArrayTypeFields array_type_fields = mppl_array_type_fields_alloc((const MpplArrayType *) type);

    const Ty     *base = check_type(checker, array_type_fields.type);
    unsigned long size = strtoul(array_type_fields.number_lit->raw->text, NULL, 10);

    if (size == 0) {
      unsigned offset = array_type_fields.number_lit->node.span.offset;
      unsigned length = array_type_fields.number_lit->raw->node.span.text_length;
      Report  *report = diag_zero_sized_array_error(offset, length);
      vec_push(&checker->diags, &report, 1);
    }

    mppl_array_type_fields_free(&array_type_fields);
    return ty_array(checker->ctxt, base, size);
  }

  default:
    unreachable();
  }
}

static Value check_expr(Checker *checker, const AnyMpplExpr *expr);

static Value check_indexed_var_expr(Checker *checker, const MpplIndexedVarExpr *indexed_var)
{
  MpplIndexedVarExprFields indexed_var_expr_fields = mppl_indexed_var_expr_fields_alloc((const MpplIndexedVarExpr *) indexed_var);

  const Ty *ty = get_ty_from_ref(checker, indexed_var_expr_fields.name);

  Value value;
  value.kind = VALUE_PLACE;

  if (ty->kind == TY_ARRAY) {
    const ArrayTy *array_ty = (const ArrayTy *) ty;
    Value          index    = check_expr(checker, indexed_var_expr_fields.index);
    if (index.ty->kind == TY_INTEGER) {
      value.ty = array_ty->base;
    } else {
      error_mismathced_type(checker, indexed_var_expr_fields.index, ty_integer(), index.ty);
      value.ty = ty_error();
    }
  } else {
    unsigned long begin = indexed_var_expr_fields.lbracket_token->node.span.offset;
    unsigned long end   = indexed_var_expr_fields.rbracket_token->node.span.offset
      + indexed_var_expr_fields.rbracket_token->raw->node.span.text_length;
    Report *report = diag_non_array_subscript_error(begin, end - begin);
    vec_push(&checker->diags, &report, 1);
    value.ty = ty_error();
  }

  mppl_indexed_var_expr_fields_free(&indexed_var_expr_fields);
  return value;
}

static Value check_binary_expr(Checker *checker, const MpplBinaryExpr *binary_expr)
{
  MpplBinaryExprFields binary_expr_fields = mppl_binary_expr_fields_alloc(binary_expr);

  Value lhs = check_expr(checker, binary_expr_fields.lhs);
  Value rhs = check_expr(checker, binary_expr_fields.rhs);

  Value value;
  value.kind = VALUE_TEMP;
  switch (binary_expr_fields.op_token->raw->node.kind) {
  case MPPL_SYNTAX_PLUS_TOKEN:
  case MPPL_SYNTAX_MINUS_TOKEN:
  case MPPL_SYNTAX_STAR_TOKEN:
  case MPPL_SYNTAX_DIV_KW: {
    if (lhs.ty->kind != TY_INTEGER) {
      error_mismathced_type(checker, binary_expr_fields.lhs, ty_integer(), lhs.ty);
    }
    if (rhs.ty->kind != TY_INTEGER) {
      error_mismathced_type(checker, binary_expr_fields.rhs, ty_integer(), rhs.ty);
    }

    if (lhs.ty->kind == TY_INTEGER && rhs.ty->kind == TY_INTEGER) {
      value.ty = ty_integer();
    } else {
      value.ty = ty_error();
    }
    break;
  }

  case MPPL_SYNTAX_AND_KW:
  case MPPL_SYNTAX_OR_KW: {
    if (lhs.ty->kind != TY_BOOLEAN) {
      error_mismathced_type(checker, binary_expr_fields.lhs, ty_boolean(), lhs.ty);
    }
    if (rhs.ty->kind != TY_BOOLEAN) {
      error_mismathced_type(checker, binary_expr_fields.rhs, ty_boolean(), rhs.ty);
    }

    if (lhs.ty->kind == TY_BOOLEAN && rhs.ty->kind == TY_BOOLEAN) {
      value.ty = ty_boolean();
    } else {
      value.ty = ty_error();
    }
    break;
  }

  case MPPL_SYNTAX_EQUAL_TOKEN:
  case MPPL_SYNTAX_NOTEQ_TOKEN:
  case MPPL_SYNTAX_LESS_TOKEN:
  case MPPL_SYNTAX_LESSEQ_TOKEN:
  case MPPL_SYNTAX_GREATER_TOKEN:
  case MPPL_SYNTAX_GREATEREQ_TOKEN: {
    if (!ty_is_std(lhs.ty)) {
      error_non_standard_type(checker, binary_expr_fields.lhs, lhs.ty);
    }
    if (!ty_is_std(rhs.ty)) {
      error_non_standard_type(checker, binary_expr_fields.rhs, rhs.ty);
    }

    if (ty_is_std(lhs.ty) && ty_is_std(rhs.ty)) {
      if (lhs.ty == rhs.ty) {
        value.ty = ty_boolean();
      } else {
        error_mismathced_type(checker, binary_expr_fields.rhs, lhs.ty, rhs.ty);
        value.ty = ty_error();
      }
    }
    break;
  }

  default:
    unreachable();
  }

  mppl_binary_expr_fields_free(&binary_expr_fields);
  return value;
}

static Value check_unary_expr(Checker *checker, const MpplUnaryExpr *unary_expr)
{
  MpplUnaryExprFields unary_expr_fields = mppl_unary_expr_fields_alloc(unary_expr);

  Value expr = check_expr(checker, unary_expr_fields.expr);

  Value value;
  value.kind = VALUE_TEMP;
  switch (unary_expr_fields.op_token->raw->node.kind) {
  case MPPL_SYNTAX_PLUS_TOKEN:
  case MPPL_SYNTAX_MINUS_TOKEN: {
    if (expr.ty->kind == TY_INTEGER) {
      value.ty = ty_integer();
    } else {
      error_mismathced_type(checker, unary_expr_fields.expr, ty_integer(), expr.ty);
      value.ty = ty_error();
    }
    break;
  }

  case MPPL_SYNTAX_NOT_KW: {
    if (expr.ty->kind == TY_BOOLEAN) {
      value.ty = ty_boolean();
    } else {
      error_mismathced_type(checker, unary_expr_fields.expr, ty_boolean(), expr.ty);
      value.ty = ty_error();
    }
    break;
  }

  default:
    unreachable();
  }

  mppl_unary_expr_fields_free(&unary_expr_fields);
  return value;
}

static Value check_expr_core(Checker *checker, const AnyMpplExpr *expr)
{
  Value value;
  switch (mppl_expr_kind(expr)) {
  case MPPL_EXPR_SYNTAX_INTEGER_LIT: {
    value.kind = VALUE_TEMP;
    value.ty   = ty_integer();
    break;
  }

  case MPPL_EXPR_SYNTAX_BOOLEAN_LIT: {
    value.kind = VALUE_TEMP;
    value.ty   = ty_boolean();
    break;
  }

  case MPPL_EXPR_SYNTAX_STRING_LIT: {
    MpplStringLitExprFields string_lit_fields = mppl_string_lit_expr_fields_alloc((const MpplStringLitExpr *) expr);
    const SyntaxToken      *lit_token         = (const SyntaxToken *) string_lit_fields.string_lit;

    value.kind = VALUE_TEMP;
    if (lit_token->raw->node.span.text_length == 3 || strcmp(lit_token->raw->text, "''''") == 0) {
      value.ty = ty_char();
    } else {
      value.ty = ty_string();
    }

    mppl_string_lit_expr_fields_free(&string_lit_fields);
    break;
  }

  case MPPL_EXPR_SYNTAX_ENTIRE_VAR: {
    MpplEntireVarExprFields entire_var_fields = mppl_entire_var_expr_fields_alloc((const MpplEntireVarExpr *) expr);

    value.kind = VALUE_PLACE;
    value.ty   = get_ty_from_ref(checker, entire_var_fields.name);

    mppl_entire_var_expr_fields_free(&entire_var_fields);
    break;
  }

  case MPPL_EXPR_SYNTAX_INDEXED_VAR:
    value = check_indexed_var_expr(checker, (const MpplIndexedVarExpr *) expr);
    break;

  case MPPL_EXPR_SYNTAX_BINARY:
    value = check_binary_expr(checker, (const MpplBinaryExpr *) expr);
    break;

  case MPPL_EXPR_SYNTAX_UNARY:
    value = check_unary_expr(checker, (const MpplUnaryExpr *) expr);
    break;

  case MPPL_EXPR_SYNTAX_CAST: {
    MpplCastExprFields cast_expr_fields = mppl_cast_expr_fields_alloc((const MpplCastExpr *) expr);

    const Ty *ty   = check_type(checker, cast_expr_fields.type);
    Value     expr = check_expr(checker, cast_expr_fields.expr);

    value.kind = VALUE_TEMP;
    if (ty_is_std(expr.ty)) {
      value.ty = ty;
    } else {
      error_non_standard_type(checker, cast_expr_fields.expr, expr.ty);
      value.ty = ty_error();
    }

    mppl_cast_expr_fields_free(&cast_expr_fields);
    break;
  }

  case MPPL_EXPR_SYNTAX_PAREN: {
    MpplParenExprFields paren_expr_fields = mppl_paren_expr_fields_alloc((const MpplParenExpr *) expr);

    value = check_expr(checker, paren_expr_fields.expr);
    mppl_paren_expr_fields_free(&paren_expr_fields);
    break;
  }

  case MPPL_EXPR_SYNTAX_BOGUS: {
    value.ty = ty_error();
    break;
  }

  default:
    unreachable();
  }
  return value;
}

static Value check_expr(Checker *checker, const AnyMpplExpr *expr)
{
  Value             value  = check_expr_core(checker, expr);
  const SyntaxTree *syntax = (SyntaxTree *) expr;
  ty_ctxt_set(checker->ctxt, (const RawSyntaxNode *) syntax->raw, value.ty);
  return value;
}

static void check_var_decl(Checker *checker, const MpplVarDecl *var_decl)
{
  MpplVarDeclFields var_decl_fields = mppl_var_decl_fields_alloc(var_decl);

  const Ty   *ty    = check_type(checker, var_decl_fields.type);
  SyntaxEvent event = syntax_event_alloc((const SyntaxTree *) var_decl);
  while (syntax_event_next(&event)) {
    if (event.kind == SYNTAX_EVENT_ENTER && event.syntax->raw->node.kind == MPPL_SYNTAX_BIND_IDENT) {
      set_ty_to_bind(checker, ty, (const MpplBindIdent *) event.syntax);
    }
  }
  syntax_event_free(&event);

  mppl_var_decl_fields_free(&var_decl_fields);
}

static void check_proc_heading(Checker *checker, const MpplProcHeading *proc_heading)
{
  MpplProcHeadingFields proc_heading_fields = mppl_proc_heading_fields_alloc(proc_heading);

  SyntaxEvent event = syntax_event_alloc((const SyntaxTree *) proc_heading_fields.fml_params);
  Vec(const Ty *) param_tys;
  vec_alloc(&param_tys, 0);

  while (syntax_event_next(&event)) {
    if (event.kind == SYNTAX_EVENT_ENTER && event.syntax->raw->node.kind == MPPL_SYNTAX_FML_PARAM_SEC) {
      MpplFmlParamSecFields fml_param_sec_fields = mppl_fml_param_sec_fields_alloc((const MpplFmlParamSec *) event.syntax);

      const Ty *ty = check_type(checker, fml_param_sec_fields.type);
      while (syntax_event_next(&event)) {
        if (event.kind == SYNTAX_EVENT_ENTER && event.syntax->raw->node.kind == MPPL_SYNTAX_BIND_IDENT) {
          set_ty_to_bind(checker, ty, (const MpplBindIdent *) event.syntax);
          vec_push(&param_tys, &ty, 1);
        } else if (event.kind == SYNTAX_EVENT_LEAVE && event.syntax->raw->node.kind == MPPL_SYNTAX_FML_PARAM_SEC) {
          break;
        }
      }
      mppl_fml_param_sec_fields_free(&fml_param_sec_fields);
    }
  }

  {
    const Ty *ty = ty_proc(checker->ctxt, param_tys.ptr, param_tys.count);
    set_ty_to_bind(checker, ty, proc_heading_fields.name);
  }

  {
    MpplBindIdentFields bind_ident_fields = mppl_bind_ident_fields_alloc(proc_heading_fields.name);
    checker->proc_name                    = bind_ident_fields.ident->raw;
    mppl_bind_ident_fields_free(&bind_ident_fields);
  }

  vec_free(&param_tys);
  syntax_event_free(&event);
  mppl_proc_heading_fields_free(&proc_heading_fields);
}

static void check_assign_stmt(Checker *checker, const MpplAssignStmt *assign_stmt)
{
  MpplAssignStmtFields assign_stmt_fields = mppl_assign_stmt_fields_alloc(assign_stmt);

  Value lhs = check_expr(checker, assign_stmt_fields.lhs);
  Value rhs = check_expr(checker, assign_stmt_fields.rhs);

  if (lhs.kind != VALUE_PLACE) {
    const SyntaxTree *lhs_syntax = (const SyntaxTree *) assign_stmt_fields.lhs;
    if (lhs.ty->kind != TY_ERROR) {
      unsigned long offset = lhs_syntax->node.span.offset;
      unsigned long length = lhs_syntax->raw->node.span.text_length;
      Report       *report = diag_non_lvalue_assignment_error(offset, length);
      vec_push(&checker->diags, &report, 1);
    }
  } else if (!ty_is_std(lhs.ty)) {
    error_non_standard_type(checker, assign_stmt_fields.lhs, lhs.ty);
  } else if (lhs.ty != rhs.ty) {
    error_mismathced_type(checker, assign_stmt_fields.rhs, lhs.ty, rhs.ty);
  }

  mppl_assign_stmt_fields_free(&assign_stmt_fields);
}

static void check_while_stmt(Checker *checker, const MpplWhileStmt *while_stmt)
{
  MpplWhileStmtFields while_stmt_fields = mppl_while_stmt_fields_alloc(while_stmt);

  Value cond = check_expr(checker, while_stmt_fields.cond);

  if (cond.ty->kind != TY_BOOLEAN) {
    error_mismathced_type(checker, while_stmt_fields.cond, ty_boolean(), cond.ty);
  }

  mppl_while_stmt_fields_free(&while_stmt_fields);
}

static void check_if_stmt(Checker *checker, const MpplIfStmt *if_stmt)
{
  MpplIfStmtFields if_stmt_fields = mppl_if_stmt_fields_alloc(if_stmt);

  Value cond = check_expr(checker, if_stmt_fields.cond);

  if (cond.ty->kind != TY_BOOLEAN) {
    error_mismathced_type(checker, if_stmt_fields.cond, ty_boolean(), cond.ty);
  }

  mppl_if_stmt_fields_free(&if_stmt_fields);
}

static void check_call_stmt(Checker *checker, const MpplCallStmt *call_stmt)
{
  MpplCallStmtFields call_stmt_fields = mppl_call_stmt_fields_alloc(call_stmt);

  const Ty *ty = get_ty_from_ref(checker, call_stmt_fields.name);

  if (ty->kind == TY_PROC) {
    unsigned long      i;
    HashMapEntry       entry;
    const MpplBinding *binding;

    const ProcTy *proc_ty = (const ProcTy *) ty;

    MpplRefIdentFields  ref_ident_fields  = mppl_ref_ident_fields_alloc(call_stmt_fields.name);
    MpplActParamsFields act_params_fields = mppl_act_params_fields_alloc(call_stmt_fields.act_params);
    MpplExprListFields  expr_list_fields  = mppl_expr_list_fields_alloc(act_params_fields.expr_list);

    hashmap_entry(&checker->semantics->ref, &ref_ident_fields.ident->node.span.offset, &entry);
    binding = *hashmap_value(&checker->semantics->ref, &entry);

    if (checker->proc_name == binding->binding->raw) {
      unsigned long offset = ref_ident_fields.ident->node.span.offset;
      unsigned long length = ref_ident_fields.ident->raw->node.span.text_length;
      Report       *report = diag_recursive_call_error(offset, length, ref_ident_fields.ident->raw->text);
      vec_push(&checker->diags, &report, 1);
    } else if (proc_ty->params.count != expr_list_fields.count) {
      unsigned long offset = call_stmt_fields.act_params->syntax.node.span.offset;
      unsigned long length = call_stmt_fields.act_params->syntax.raw->node.span.text_length;
      Report       *report = diag_mismatched_arguments_count_error(offset, length, proc_ty->params.count, expr_list_fields.count);
      vec_push(&checker->diags, &report, 1);
    } else {
      for (i = 0; i < expr_list_fields.count; ++i) {
        MpplExprListElemFields elem_fields = mppl_expr_list_elem_fields_alloc(expr_list_fields.ptr[i]);

        Value arg = check_expr(checker, elem_fields.expr);
        if (arg.ty != proc_ty->params.ptr[i]) {
          error_mismathced_type(checker, elem_fields.expr, proc_ty->params.ptr[i], arg.ty);
        }
        mppl_expr_list_elem_fields_free(&elem_fields);
      }
    }

    mppl_ref_ident_fields_free(&ref_ident_fields);
    mppl_expr_list_fields_free(&expr_list_fields);
    mppl_act_params_fields_free(&act_params_fields);
  } else {
    unsigned long offset = call_stmt_fields.name->syntax.node.span.offset;
    unsigned long length = call_stmt_fields.name->syntax.raw->node.span.text_length;
    Report       *report = diag_non_procedure_invocation_error(offset, length);
    vec_push(&checker->diags, &report, 1);
  }

  mppl_call_stmt_fields_free(&call_stmt_fields);
}

static void check_inputs(Checker *checker, const MpplInputs *inputs)
{
  unsigned long i;

  MpplInputsFields   inputs_fields    = mppl_inputs_fields_alloc(inputs);
  MpplExprListFields expr_list_fields = mppl_expr_list_fields_alloc(inputs_fields.expr_list);

  for (i = 0; i < expr_list_fields.count; ++i) {
    MpplExprListElemFields elem_fields = mppl_expr_list_elem_fields_alloc(expr_list_fields.ptr[i]);

    Value arg = check_expr(checker, elem_fields.expr);
    if (arg.kind != VALUE_PLACE) {
      const SyntaxTree *syntax = (const SyntaxTree *) elem_fields.expr;
      if (arg.ty->kind != TY_ERROR) {
        unsigned long offset = syntax->node.span.offset;
        unsigned long length = syntax->raw->node.span.text_length;
        Report       *report = diag_invalid_input_error(offset, length);
        vec_push(&checker->diags, &report, 1);
      }
    }

    mppl_expr_list_elem_fields_free(&elem_fields);
  }

  mppl_expr_list_fields_free(&expr_list_fields);
  mppl_inputs_fields_free(&inputs_fields);
}

static void check_outputs(Checker *checker, const MpplOutputs *outputs)
{
  unsigned long i;

  MpplOutputsFields    outputs_fields     = mppl_outputs_fields_alloc(outputs);
  MpplOutputListFields output_list_fields = mppl_output_list_fields_alloc(outputs_fields.output_list);

  for (i = 0; i < output_list_fields.count; ++i) {
    MpplOutputListElemFields elem_fields = mppl_output_list_elem_fields_alloc(output_list_fields.ptr[i]);
    switch (mppl_output_value_kind(elem_fields.output_value)) {
    case MPPL_OUTPUT_VALUE_SYNTAX: {
      MpplOutputValueFields output_value_fields = mppl_output_value_fields_alloc((const MpplOutputValue *) elem_fields.output_value);

      Value value = check_expr(checker, output_value_fields.expr);
      if (!ty_is_std(value.ty)) {
        if (value.ty->kind != TY_STRING) {
          error_non_standard_type(checker, output_value_fields.expr, value.ty);
        } else if (output_value_fields.field_width) {
          unsigned long offset = output_value_fields.field_width->syntax.node.span.offset;
          unsigned long length = output_value_fields.field_width->syntax.raw->node.span.text_length;
          Report       *report = diag_invalid_output_value_error(offset, length);
          vec_push(&checker->diags, &report, 1);
        }
      }

      mppl_output_value_fields_free(&output_value_fields);
    }

    case MPPL_OUTPUT_VALUE_SYNTAX_BOGUS:
      /* do nothing */
      break;

    default:
      unreachable();
    }
    mppl_output_list_elem_fields_free(&elem_fields);
  }

  mppl_output_list_fields_free(&output_list_fields);
  mppl_outputs_fields_free(&outputs_fields);
}

static void check_syntax(Checker *checker, const SyntaxTree *syntax)
{
  SyntaxEvent event = syntax_event_alloc(syntax);
  while (syntax_event_next(&event)) {
    if (event.kind == SYNTAX_EVENT_ENTER) {
      switch (event.syntax->raw->node.kind) {
      case MPPL_SYNTAX_VAR_DECL:
        check_var_decl(checker, (const MpplVarDecl *) event.syntax);
        break;

      case MPPL_SYNTAX_PROC_HEADING:
        check_proc_heading(checker, (const MpplProcHeading *) event.syntax);
        break;

      case MPPL_SYNTAX_ASSIGN_STMT:
        check_assign_stmt(checker, (const MpplAssignStmt *) event.syntax);
        break;

      case MPPL_SYNTAX_WHILE_STMT:
        check_while_stmt(checker, (const MpplWhileStmt *) event.syntax);
        break;

      case MPPL_SYNTAX_IF_STMT:
        check_if_stmt(checker, (const MpplIfStmt *) event.syntax);
        break;

      case MPPL_SYNTAX_CALL_STMT:
        check_call_stmt(checker, (const MpplCallStmt *) event.syntax);
        break;

      case MPPL_SYNTAX_INPUTS:
        check_inputs(checker, (const MpplInputs *) event.syntax);
        break;

      case MPPL_SYNTAX_OUTPUTS:
        check_outputs(checker, (const MpplOutputs *) event.syntax);
        break;

      default:
        /* do nothing */
        break;
      }
    } else {
      switch (event.syntax->raw->node.kind) {
      case MPPL_SYNTAX_PROC_BODY:
        checker->proc_name = NULL;
        break;

      default:
        /* do nothing */
        break;
      }
    }
  }
  syntax_event_free(&event);
}

MpplCheckResult mppl_check(const MpplRoot *syntax, const MpplSemantics *semantics)
{
  MpplCheckResult result;
  Checker         checker;
  checker.semantics = semantics;
  checker.ctxt      = ty_ctxt_alloc();
  vec_alloc(&checker.diags, 0);

  check_syntax(&checker, &syntax->syntax);

  result.ctxt        = checker.ctxt;
  result.diags.ptr   = checker.diags.ptr;
  result.diags.count = checker.diags.count;

  return result;
}
