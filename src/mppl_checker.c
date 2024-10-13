/* SPDX-License-Identifier: Apache-2.0 */

#include "mppl_passes.h"
#include "mppl_semantic.h"
#include "mppl_syntax.h"
#include "mppl_ty_ctxt.h"
#include "stdlib.h"
#include "syntax_tree.h"
#include "util.h"

typedef struct Value   Value;
typedef struct Checker Checker;

struct Value {
  const MpplTy *ty;
  enum ValueKind {
    VALUE_ERROR,
    VALUE_LVALUE,
    VALUE_RVALUE
  } kind;
};

struct Checker {
  const MpplSemantics *semantics;
  MpplTyCtxt          *ctxt;
  Vec(Report *) diags;
};

static int ty_is_std(const MpplTy *ty)
{
  if (!ty) {
    return 0;
  }

  switch (ty->kind) {
  case MPPL_TY_INTEGER:
  case MPPL_TY_BOOLEAN:
  case MPPL_TY_CHAR:
    return 1;

  default:
    return 0;
  }
}

static void set_ty_to_bind(Checker *checker, const MpplTy *ty, const MpplBindIdent *bind_ident)
{
  MpplBindIdentFields bind_ident_fields = mppl_bind_ident_fields_alloc(bind_ident);
  mppl_ty_ctxt_type_of(checker->ctxt, (const RawSyntaxNode *) bind_ident_fields.ident->raw, ty);
  mppl_bind_ident_fields_free(&bind_ident_fields);
}

static const MpplTy *get_ty_from_ref(Checker *checker, const MpplRefIdent *ref_ident)
{
  HashMapEntry       entry;
  const MpplBinding *binding;
  const MpplTy      *ty;

  MpplRefIdentFields ref_ident_fields = mppl_ref_ident_fields_alloc(ref_ident);

  hashmap_entry(&checker->semantics->ref, &ref_ident_fields.ident->node.span.offset, &entry);
  binding = *hashmap_value(&checker->semantics->ref, &entry);
  ty      = mppl_ty_ctxt_type_of(checker->ctxt, (const RawSyntaxNode *) binding->binding->raw, NULL);

  mppl_ref_ident_fields_free(&ref_ident_fields);
  return ty;
}

static const MpplTy *check_type(Checker *checker, const AnyMpplType *type)
{
  switch (mppl_type_kind(type)) {
  case MPPL_TYPE_SYNTAX_INTEGER:
    return mppl_ty_integer();

  case MPPL_TYPE_SYNTAX_BOOLEAN:
    return mppl_ty_boolean();

  case MPPL_TYPE_SYNTAX_CHAR:
    return mppl_ty_char();

  case MPPL_TYPE_SYNTAX_ARRAY: {
    MpplArrayTypeFields array_type_fields = mppl_array_type_fields_alloc((const MpplArrayType *) type);

    const MpplTy *base = check_type(checker, array_type_fields.type);
    unsigned long size = strtoul(array_type_fields.number_lit->raw->text, NULL, 10);

    return mppl_ty_array(checker->ctxt, base, size);
  }

  default:
    unreachable();
  }
}

static Value check_expr(Checker *checker, const AnyMpplExpr *expr);

static Value check_var(Checker *checker, const AnyMpplVar *var)
{
  switch (mppl_var_kind(var)) {
  case MPPL_VAR_SYNTAX_ENTIRE: {
    MpplEntireVarFields entire_var_fields = mppl_entire_var_fields_alloc((const MpplEntireVar *) var);

    Value value;
    value.kind = VALUE_LVALUE;
    value.ty   = get_ty_from_ref(checker, entire_var_fields.name);
    return value;
  }

  case MPPL_VAR_SYNTAX_INDEXED: {
    MpplIndexedVarFields indexed_var_fields = mppl_indexed_var_fields_alloc((const MpplIndexedVar *) var);

    const MpplTy *ty    = get_ty_from_ref(checker, indexed_var_fields.name);
    Value         index = check_expr(checker, indexed_var_fields.index);

    if (ty->kind != MPPL_TY_ARRAY) {
      /* TODO: Report error */
    }

    if (index.kind != VALUE_ERROR && index.ty->kind != MPPL_TY_INTEGER) {
      /* TODO: Report error */
    }

    if (ty->kind == MPPL_TY_ARRAY) {
      MpplArrayTy *array_ty = (MpplArrayTy *) ty;

      Value value;
      value.kind = VALUE_LVALUE;
      value.ty   = array_ty->base;
      return value;
    } else {
      Value value;
      value.kind = VALUE_ERROR;
      value.ty   = NULL;
      return value;
    }
  }
  }
}

static Value check_binary_expr(Checker *checker, const MpplBinaryExpr *binary_expr)
{
  MpplBinaryExprFields binary_expr_fields = mppl_binary_expr_fields_alloc(binary_expr);

  Value          lhs = check_expr(checker, binary_expr_fields.lhs);
  Value          rhs = check_expr(checker, binary_expr_fields.rhs);
  MpplSyntaxKind op  = binary_expr_fields.op_token->raw->node.kind;
  mppl_binary_expr_fields_free(&binary_expr_fields);

  switch (op) {
  case MPPL_SYNTAX_PLUS_TOKEN:
  case MPPL_SYNTAX_MINUS_TOKEN:
  case MPPL_SYNTAX_STAR_TOKEN:
  case MPPL_SYNTAX_DIV_KW: {
    Value value;
    value.kind = VALUE_RVALUE;
    value.ty   = mppl_ty_integer();

    if (lhs.kind != VALUE_ERROR && lhs.ty->kind != MPPL_TY_INTEGER) {
      /* TODO: Report error */
    }

    if (rhs.kind != VALUE_ERROR && rhs.ty->kind != MPPL_TY_INTEGER) {
      /* TODO: Report error */
    }

    return value;
  }

  case MPPL_SYNTAX_AND_KW:
  case MPPL_SYNTAX_OR_KW: {
    Value value;
    value.kind = VALUE_RVALUE;
    value.ty   = mppl_ty_boolean();

    if (lhs.kind != VALUE_ERROR && lhs.ty->kind != MPPL_TY_BOOLEAN) {
      /* TODO: Report error */
    }

    if (rhs.kind != VALUE_ERROR && rhs.ty->kind != MPPL_TY_BOOLEAN) {
      /* TODO: Report error */
    }

    return value;
  }

  case MPPL_SYNTAX_EQUAL_TOKEN:
  case MPPL_SYNTAX_NOTEQ_TOKEN:
  case MPPL_SYNTAX_LESS_TOKEN:
  case MPPL_SYNTAX_LESSEQ_TOKEN:
  case MPPL_SYNTAX_GREATER_TOKEN:
  case MPPL_SYNTAX_GREATEREQ_TOKEN: {
    Value value;
    value.kind = VALUE_RVALUE;
    value.ty   = mppl_ty_boolean();

    if (lhs.kind != VALUE_ERROR && !ty_is_std(lhs.ty)) {
      /* TODO: Report error */
    }

    if (rhs.kind != VALUE_ERROR && !ty_is_std(rhs.ty)) {
      /* TODO: Report error */
    }

    if (ty_is_std(lhs.ty) && ty_is_std(rhs.ty) && lhs.ty != rhs.ty) {
      /* TODO: Report error */
    }

    return value;
  }

  default:
    unreachable();
  }
}

static Value check_unary_expr(Checker *checker, const MpplUnaryExpr *unary_expr)
{
  MpplUnaryExprFields unary_expr_fields = mppl_unary_expr_fields_alloc(unary_expr);

  Value          expr = check_expr(checker, unary_expr_fields.expr);
  MpplSyntaxKind op   = unary_expr_fields.op_token->raw->node.kind;
  mppl_unary_expr_fields_free(&unary_expr_fields);

  switch (op) {
  case MPPL_SYNTAX_PLUS_TOKEN:
  case MPPL_SYNTAX_MINUS_TOKEN: {
    Value value;
    value.kind = VALUE_RVALUE;
    value.ty   = mppl_ty_integer();

    if (expr.kind != VALUE_ERROR && expr.ty->kind != MPPL_TY_INTEGER) {
      /* TODO: Report error */
    }

    return value;
  }

  case MPPL_SYNTAX_NOT_KW: {
    Value value;
    value.kind = VALUE_RVALUE;
    value.ty   = mppl_ty_boolean();

    if (expr.kind != VALUE_ERROR && expr.ty->kind != MPPL_TY_BOOLEAN) {
      /* TODO: Report error */
    }

    return value;
  }

  default:
    unreachable();
  }
}

static Value check_expr_core(Checker *checker, const AnyMpplExpr *expr)
{
  switch (mppl_expr_kind(expr)) {
  case MPPL_EXPR_SYNTAX_VAR:
    return check_var(checker, (const AnyMpplVar *) expr);

  case MPPL_EXPR_SYNTAX_BINARY:
    return check_binary_expr(checker, (const MpplBinaryExpr *) expr);

  case MPPL_EXPR_SYNTAX_UNARY:
    return check_unary_expr(checker, (const MpplUnaryExpr *) expr);

  case MPPL_EXPR_SYNTAX_CAST: {
    MpplCastExprFields cast_expr_fields = mppl_cast_expr_fields_alloc((const MpplCastExpr *) expr);

    const MpplTy *ty   = check_type(checker, cast_expr_fields.type);
    Value         expr = check_expr(checker, cast_expr_fields.expr);

    Value value;
    value.kind = VALUE_RVALUE;
    value.ty   = ty;

    if (expr.kind != VALUE_ERROR && !ty_is_std(expr.ty)) {
      /* TODO: Report error */
    }

    mppl_cast_expr_fields_free(&cast_expr_fields);
    return value;
  }

  case MPPL_EXPR_SYNTAX_PAREN: {
    MpplParenExprFields paren_expr_fields = mppl_paren_expr_fields_alloc((const MpplParenExpr *) expr);

    Value value = check_expr(checker, paren_expr_fields.expr);
    mppl_paren_expr_fields_free(&paren_expr_fields);
    return value;
  }

  case MPPL_EXPR_SYNTAX_BOGUS: {
    Value value;
    value.kind = VALUE_ERROR;
    value.ty   = NULL;
    return value;
  }

  default:
    unreachable();
  }
}

static Value check_expr(Checker *checker, const AnyMpplExpr *expr)
{
  Value             value  = check_expr_core(checker, expr);
  const SyntaxTree *syntax = (SyntaxTree *) expr;
  mppl_ty_ctxt_type_of(checker->ctxt, (const RawSyntaxNode *) syntax->raw, value.ty);
  return value;
}

static void check_output_value(Checker *checker, const AnyMpplOutputValue *output_value)
{
  switch (mppl_output_value_kind(output_value)) {
  case MPPL_OUTPUT_VALUE_SYNTAX: {
    MpplOutputValueFields output_value_fields = mppl_output_value_fields_alloc((const MpplOutputValue *) output_value);

    Value value = check_expr(checker, output_value_fields.expr);
    if (value.kind != VALUE_ERROR && !ty_is_std(value.ty)) {
      /* TODO: Report error */
    }
  }

  case MPPL_OUTPUT_VALUE_SYNTAX_BOGUS:
    /* do nothing */
    break;

  default:
    unreachable();
  }
}

static void check_var_decl(Checker *checker, const MpplVarDecl *var_decl)
{
  MpplVarDeclFields var_decl_fields = mppl_var_decl_fields_alloc(var_decl);

  const MpplTy *ty    = check_type(checker, var_decl_fields.type);
  SyntaxEvent   event = syntax_event_alloc((const SyntaxTree *) var_decl);
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
  Vec(const MpplTy *) param_tys;
  vec_alloc(&param_tys, 0);

  while (syntax_event_next(&event)) {
    if (event.kind == SYNTAX_EVENT_ENTER && event.syntax->raw->node.kind == MPPL_SYNTAX_FML_PARAM_SEC) {
      MpplFmlParamSecFields fml_param_sec_fields = mppl_fml_param_sec_fields_alloc((const MpplFmlParamSec *) event.syntax);

      const MpplTy *ty = check_type(checker, fml_param_sec_fields.type);
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
    const MpplTy *ty = mppl_ty_proc(checker->ctxt, param_tys.ptr, param_tys.count);
    set_ty_to_bind(checker, ty, proc_heading_fields.name);
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

  if (lhs.kind != VALUE_LVALUE) {
    /* TODO: Report error */
  }

  if (lhs.ty != rhs.ty) {
    /* TODO: Report error */
  }

  mppl_assign_stmt_fields_free(&assign_stmt_fields);
}

static void check_while_stmt(Checker *checker, const MpplWhileStmt *while_stmt)
{
  MpplWhileStmtFields while_stmt_fields = mppl_while_stmt_fields_alloc(while_stmt);

  Value cond = check_expr(checker, while_stmt_fields.cond);

  if (cond.ty->kind != MPPL_TY_BOOLEAN) {
    /* TODO: Report error */
  }

  mppl_while_stmt_fields_free(&while_stmt_fields);
}

static void check_if_stmt(Checker *checker, const MpplIfStmt *if_stmt)
{
  MpplIfStmtFields if_stmt_fields = mppl_if_stmt_fields_alloc(if_stmt);

  Value cond = check_expr(checker, if_stmt_fields.cond);

  if (cond.ty->kind != MPPL_TY_BOOLEAN) {
    /* TODO: Report error */
  }

  mppl_if_stmt_fields_free(&if_stmt_fields);
}

static void check_call_stmt(Checker *checker, const MpplCallStmt *call_stmt)
{
  MpplCallStmtFields call_stmt_fields = mppl_call_stmt_fields_alloc(call_stmt);

  const MpplTy *ty = get_ty_from_ref(checker, call_stmt_fields.name);

  if (ty->kind == MPPL_TY_PROC) {
    unsigned long     i;
    const MpplProcTy *proc_ty = (const MpplProcTy *) ty;

    MpplActParamsFields act_params_fields = mppl_act_params_fields_alloc(call_stmt_fields.act_params);
    MpplExprListFields  expr_list_fields  = mppl_expr_list_fields_alloc(act_params_fields.expr_list);

    if (proc_ty->params.count != expr_list_fields.count) {
      /* TODO: Report error */
    }

    for (i = 0; i < expr_list_fields.count; ++i) {
      MpplExprListElemFields elem_fields = mppl_expr_list_elem_fields_alloc(expr_list_fields.ptr[i]);

      Value arg = check_expr(checker, elem_fields.expr);
      if (proc_ty->params.count == expr_list_fields.count && arg.kind != VALUE_ERROR && arg.ty != proc_ty->params.ptr[i]) {
        /* TODO: Report error */
      }
      mppl_expr_list_elem_fields_free(&elem_fields);
    }

    mppl_expr_list_fields_free(&expr_list_fields);
    mppl_act_params_fields_free(&act_params_fields);
  } else {
    /* TODO: Report error */
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
    if (arg.kind != VALUE_ERROR && arg.kind != VALUE_LVALUE) {
      /* TODO: Report error */
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

    switch (mppl_output_kind(elem_fields.output)) {
    case MPPL_OUTPUT_SYNTAX_EXPR: {
      Value value = check_expr(checker, &elem_fields.output->expr);
      if (value.kind != VALUE_ERROR && !ty_is_std(value.ty) && value.ty->kind != MPPL_TY_STRING) {
        /* TODO: Report error */
      }
      break;
    }

    case MPPL_OUTPUT_SYNTAX_OUTPUT_VALUE:
      check_output_value(checker, &elem_fields.output->output_value);
      break;
    }
  }

  mppl_output_list_fields_free(&output_list_fields);
  mppl_outputs_fields_free(&outputs_fields);
}

static void check_syntax(Checker *checker, const SyntaxTree *syntax)
{
  SyntaxEvent event = syntax_event_alloc(syntax);
  while (syntax_event_next(&event)) {
    if (event.kind == SYNTAX_EVENT_ENTER) {
      switch (syntax->raw->node.kind) {
      case MPPL_SYNTAX_VAR_DECL:
        check_var_decl(checker, (const MpplVarDecl *) syntax);
        break;

      case MPPL_SYNTAX_PROC_HEADING:
        check_proc_heading(checker, (const MpplProcHeading *) syntax);
        break;

      case MPPL_SYNTAX_ASSIGN_STMT:
        check_assign_stmt(checker, (const MpplAssignStmt *) syntax);
        break;

      case MPPL_SYNTAX_WHILE_STMT:
        check_while_stmt(checker, (const MpplWhileStmt *) syntax);
        break;

      case MPPL_SYNTAX_IF_STMT:
        check_if_stmt(checker, (const MpplIfStmt *) syntax);
        break;

      case MPPL_SYNTAX_CALL_STMT:
        check_call_stmt(checker, (const MpplCallStmt *) syntax);
        break;

      case MPPL_SYNTAX_INPUTS:
        check_inputs(checker, (const MpplInputs *) syntax);
        break;

      case MPPL_SYNTAX_OUTPUTS:
        check_outputs(checker, (const MpplOutputs *) syntax);
        break;

      default:
        /* do nothing */
        break;
      }
    }
  }
}

MpplCheckResult mppl_check(const MpplRoot *syntax, const MpplSemantics *semantics)
{
  MpplCheckResult result;
  Checker         checker;
  checker.semantics = semantics;
  checker.ctxt      = mppl_ty_ctxt_alloc();
  vec_alloc(&checker.diags, 0);

  check_syntax(&checker, &syntax->syntax);

  result.ctxt        = checker.ctxt;
  result.diags.ptr   = checker.diags.ptr;
  result.diags.count = checker.diags.count;

  return result;
}
