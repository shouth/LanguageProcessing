#include <assert.h>
#include <string.h>

#include "ast.h"
#include "context.h"
#include "message.h"
#include "mpplc.h"
#include "pretty.h"
#include "types.h"
#include "utility.h"

typedef struct checker__s checker_t;

struct checker__s {
  ast_visitor_t visitor;
  context_t    *ctx;
  const void   *enclosure;
};

static const type_t *record_type(checker_t *checker, const void *ast, const type_t *type)
{
  if (!type) {
    return NULL;
  }
  hash_map_update(checker->ctx->infer_result, (void *) ast, (void *) type);
  return type;
}

static int is_std_type(const type_t *type)
{
  return type->kind == TYPE_BOOLEAN || type->kind == TYPE_CHAR || type->kind == TYPE_INTEGER;
}

static const type_t *check_lit(checker_t *checker, const ast_lit_t *lit)
{
  switch (lit->kind) {
  case AST_LIT_KIND_BOOLEAN: {
    return checker->ctx->types.boolean;
  }
  case AST_LIT_KIND_NUMBER: {
    return checker->ctx->types.integer;
  }
  case AST_LIT_KIND_STRING: {
    const ast_lit_string_t *string = (ast_lit_string_t *) lit;

    if (string->str_len == 1) {
      return checker->ctx->types.char_;
    } else {
      return checker->ctx->types.string;
    }
  }
  default:
    unreachable();
  }
}

static const type_t *check_type(checker_t *checker, const ast_type_t *type)
{
  switch (type->kind) {
  case AST_TYPE_KIND_BOOLEAN: {
    return checker->ctx->types.boolean;
  }
  case AST_TYPE_KIND_CHAR: {
    return checker->ctx->types.char_;
  }
  case AST_TYPE_KIND_INTEGER: {
    return checker->ctx->types.integer;
  }
  case AST_TYPE_KIND_ARRAY: {
    const ast_type_array_t *array = (ast_type_array_t *) type;
    const ast_lit_number_t *size  = (ast_lit_number_t *) array->size;

    assert(array->size->kind == AST_LIT_KIND_NUMBER);
    if (size->value == 0) {
      msg_t *msg = msg_new(checker->ctx->src, array->size->region,
        MSG_ERROR, "size of array needs to be greater than 0");
      msg_emit(msg);
      return NULL;
    }

    {
      const type_t *base = check_type(checker, array->base);
      if (!base) {
        return NULL;
      }
      return ctx_mk_type_array(checker->ctx, ctx_mk_subst(checker->ctx, base), ((ast_lit_number_t *) array->size)->value);
    }
  }
  default:
    unreachable();
  }
}

static const type_t *check_def(checker_t *checker, const def_t *def)
{
  const hash_map_entry_t *entry = hash_map_find(checker->ctx->infer_result, def->ast);
  if (entry) {
    return entry->value;
  } else {
    switch (def->kind) {
    case DEF_PROGRAM: {
      return record_type(checker, def->ast, checker->ctx->types.program);
    }
    case DEF_PROCEDURE: {
      const ast_decl_part_procedure_t *proc   = def->ast;
      const ast_decl_param_t          *params = proc->params;
      subst_t                         *types  = NULL;
      subst_t                        **tail   = &types;

      for (; params; params = params->next) {
        const type_t      *param = check_type(checker, params->type);
        const ast_ident_t *names = params->names;
        for (; names; names = names->next) {
          subst_t *subst = ctx_mk_subst(checker->ctx, param);
          *tail          = subst;
          tail           = &subst->next;
        }
      }
      return record_type(checker, def->ast, ctx_mk_type_procedure(checker->ctx, types));
    }
    case DEF_VAR: {
      const ast_decl_variable_t *var = def->ast;
      return record_type(checker, def->ast, check_type(checker, var->type));
    }
    case DEF_PARAM: {
      const ast_decl_param_t *param = def->ast;
      return record_type(checker, def->ast, check_type(checker, param->type));
    }
    }
    unreachable();
  }
}

static const type_t *check_expr(checker_t *checker, const ast_expr_t *expr)
{
  switch (expr->kind) {
  case AST_EXPR_KIND_DECL_REF: {
    const ast_expr_decl_ref_t *ref = (ast_expr_decl_ref_t *) expr;

    const def_t *def = hash_map_find(checker->ctx->resolution, ref->decl)->value;
    return record_type(checker, expr, check_def(checker, def));
  }
  case AST_EXPR_KIND_ARRAY_SUBSCRIPT: {
    const ast_expr_array_subscript_t *subscript = (ast_expr_array_subscript_t *) expr;

    const def_t  *def   = hash_map_find(checker->ctx->resolution, subscript->decl)->value;
    const type_t *type  = check_def(checker, def);
    const type_t *index = check_expr(checker, subscript->subscript);

    if (!type || !index) {
      return NULL;
    }

    if (type->kind != TYPE_ARRAY) {
      msg_t *msg = msg_new(checker->ctx->src, subscript->decl->region,
        MSG_ERROR, "`%s` is not an array", subscript->decl->symbol->ptr);
      msg_emit(msg);
      return NULL;
    }

    if (index->kind != TYPE_INTEGER) {
      msg_t *msg = msg_new(checker->ctx->src, subscript->subscript->region,
        MSG_ERROR, "arrays cannot be indexed by `%s`", str_type(index));
      msg_add_inline(msg, subscript->subscript->region, "array indices are of type integer");
      msg_emit(msg);
      return NULL;
    }

    {
      const type_array_t *array = (type_array_t *) type;
      return record_type(checker, expr, array->base->type);
    }
  }
  case AST_EXPR_KIND_BINARY: {
    const ast_expr_binary_t *binary = (ast_expr_binary_t *) expr;

    if (binary->lhs->kind == AST_EXPR_KIND_EMPTY) {
      const type_t *rhs = check_expr(checker, binary->rhs);

      if (!rhs) {
        return NULL;
      }

      assert(binary->kind == AST_EXPR_BINARY_KIND_PLUS || binary->kind == AST_EXPR_BINARY_KIND_MINUS);
      if (rhs->kind != TYPE_INTEGER) {
        msg_t *msg = msg_new(checker->ctx->src, binary->op_region,
          MSG_ERROR, "`%s` cannot be prefixed by `%s`", str_type(rhs), pp_binary_operator_str(binary->kind));
        msg_emit(msg);
        return NULL;
      }

      return record_type(checker, expr, checker->ctx->types.integer);
    } else {
      const type_t *lhs = check_expr(checker, binary->lhs);
      const type_t *rhs = check_expr(checker, binary->rhs);

      if (!lhs || !rhs) {
        return NULL;
      }

      switch (binary->kind) {
      case AST_EXPR_BINARY_KIND_EQUAL:
      case AST_EXPR_BINARY_KIND_NOTEQ:
      case AST_EXPR_BINARY_KIND_LE:
      case AST_EXPR_BINARY_KIND_LEEQ:
      case AST_EXPR_BINARY_KIND_GR:
      case AST_EXPR_BINARY_KIND_GREQ: {
        if (lhs != rhs || !is_std_type(lhs) || !is_std_type(rhs)) {
          msg_t *msg = msg_new(checker->ctx->src, binary->op_region,
            MSG_ERROR, "invalid operands for `%s`", pp_binary_operator_str(binary->kind));
          msg_add_inline(msg, binary->lhs->region, "%s", str_type(lhs));
          msg_add_inline(msg, binary->op_region,
            "operator `%s` takes two operands of same standard type", pp_binary_operator_str(binary->kind));
          msg_add_inline(msg, binary->rhs->region, "%s", str_type(rhs));
          msg_emit(msg);
          return NULL;
        }

        return record_type(checker, expr, checker->ctx->types.boolean);
      }
      case AST_EXPR_BINARY_KIND_PLUS:
      case AST_EXPR_BINARY_KIND_MINUS:
      case AST_EXPR_BINARY_KIND_STAR:
      case AST_EXPR_BINARY_KIND_DIV: {
        if (lhs->kind != TYPE_INTEGER || rhs->kind != TYPE_INTEGER) {
          msg_t *msg = msg_new(checker->ctx->src, binary->op_region,
            MSG_ERROR, "invalid operands for `%s`", pp_binary_operator_str(binary->kind));
          msg_add_inline(msg, binary->lhs->region, "%s", str_type(lhs));
          msg_add_inline(msg, binary->op_region,
            "operator `%s` takes two operands of type integer", pp_binary_operator_str(binary->kind));
          msg_add_inline(msg, binary->rhs->region, "%s", str_type(rhs));
          msg_emit(msg);
          return NULL;
        }

        return record_type(checker, expr, checker->ctx->types.integer);
      }
      case AST_EXPR_BINARY_KIND_OR:
      case AST_EXPR_BINARY_KIND_AND: {
        if (lhs->kind != TYPE_BOOLEAN || rhs->kind != TYPE_BOOLEAN) {
          msg_t *msg = msg_new(checker->ctx->src, binary->op_region,
            MSG_ERROR, "invalid operands for `%s`", pp_binary_operator_str(binary->kind));
          msg_add_inline(msg, binary->lhs->region, "%s", str_type(lhs));
          msg_add_inline(msg, binary->op_region,
            "operator `%s` takes two operands of type boolean", pp_binary_operator_str(binary->kind));
          msg_add_inline(msg, binary->rhs->region, "%s", str_type(rhs));
          msg_emit(msg);
          return NULL;
        }

        return record_type(checker, expr, checker->ctx->types.boolean);
      }
      default:
        unreachable();
      }
    }
  }
  case AST_EXPR_KIND_NOT: {
    const ast_expr_not_t *not_ = (ast_expr_not_t *) expr;

    const type_t *type = check_expr(checker, not_->expr);

    if (!type) {
      return NULL;
    }

    if (type->kind != TYPE_BOOLEAN) {
      msg_t *msg = msg_new(checker->ctx->src, not_->op_region,
        MSG_ERROR, "invalid operands for `not`");
      msg_add_inline(msg, not_->op_region, "operator `not` takes one operand of type boolean");
      msg_add_inline(msg, not_->expr->region, "%s", str_type(type));
      msg_emit(msg);
      return NULL;
    }

    return record_type(checker, expr, checker->ctx->types.boolean);
  }
  case AST_EXPR_KIND_PAREN: {
    const ast_expr_paren_t *paren = (ast_expr_paren_t *) expr;
    return record_type(checker, expr, check_expr(checker, paren->inner));
  }
  case AST_EXPR_KIND_CAST: {
    const ast_expr_cast_t *cast = (ast_expr_cast_t *) expr;

    const type_t *value_type = check_expr(checker, cast->cast);
    const type_t *cast_type  = check_type(checker, cast->type);

    if (!value_type || !cast_type) {
      return NULL;
    }

    if (!is_std_type(value_type)) {
      msg_t *msg = msg_new(checker->ctx->src, cast->cast->region,
        MSG_ERROR, "expression of type `%s` cannot be cast", str_type(value_type));
      msg_add_inline(msg, cast->cast->region, "expressions to be cast are of standard types");
      msg_emit(msg);
      return NULL;
    }

    if (!is_std_type(cast_type)) {
      msg_t *msg = msg_new(checker->ctx->src, cast->cast->region,
        MSG_ERROR, "expression cannot be cast to `%s`", str_type(cast_type));
      msg_add_inline(msg, cast->type->region, "expressions can be cast to standard types");
      msg_emit(msg);
      return NULL;
    }

    return record_type(checker, expr, cast_type);
  }
  case AST_EXPR_KIND_CONSTANT: {
    const ast_expr_constant_t *constant = (ast_expr_constant_t *) expr;
    return record_type(checker, expr, check_lit(checker, constant->lit));
  }
  default:
    return NULL;
  }
  unreachable();
}

static void check_stmt(checker_t *checker, const ast_stmt_t *stmt)
{
  switch (stmt->kind) {
  case AST_STMT_KIND_ASSIGN: {
    const ast_stmt_assign_t *assign = (ast_stmt_assign_t *) stmt;

    const type_t *lhs = check_expr(checker, assign->lhs);
    const type_t *rhs = check_expr(checker, assign->rhs);

    if (!lhs || !rhs) {
      return;
    }

    if (lhs != rhs) {
      msg_t *msg = msg_new(checker->ctx->src, assign->op_region,
        MSG_ERROR, "invalid operands for `:=`");
      msg_add_inline(msg, assign->lhs->region, "%s", str_type(lhs));
      msg_add_inline(msg, assign->op_region,
        "operator `:=` takes two operands of the same standard type");
      msg_add_inline(msg, assign->rhs->region, "%s", str_type(rhs));
      msg_emit(msg);
      return;
    }

    return;
  }
  case AST_STMT_KIND_IF: {
    const ast_stmt_if_t *if_ = (ast_stmt_if_t *) stmt;

    const type_t *cond = check_expr(checker, if_->cond);

    if (!cond) {
      return;
    }

    if (cond->kind != TYPE_BOOLEAN) {
      msg_t *msg = msg_new(checker->ctx->src, if_->cond->region,
        MSG_ERROR, "expression of type `%s` cannot be condition", str_type(cond));
      msg_add_inline(msg, if_->cond->region, "condition expressions are of type boolean");
      msg_emit(msg);
      return;
    }

    return;
  }
  case AST_STMT_KIND_WHILE: {
    const ast_stmt_while_t *while_ = (ast_stmt_while_t *) stmt;

    const type_t *cond = check_expr(checker, while_->cond);

    if (!cond) {
      return;
    }

    if (cond->kind != TYPE_BOOLEAN) {
      msg_t *msg = msg_new(checker->ctx->src, while_->cond->region,
        MSG_ERROR, "expression of type `%s` cannot be condition", str_type(cond));
      msg_add_inline(msg, while_->cond->region, "condition expressions are of type boolean");
      msg_emit(msg);
      return;
    }

    return;
  }
  case AST_STMT_KIND_CALL: {
    const ast_stmt_call_t *call = (ast_stmt_call_t *) stmt;

    const def_t  *def  = hash_map_find(checker->ctx->resolution, call->name)->value;
    const type_t *type = check_def(checker, def);

    if (!type) {
      return;
    }

    if (type->kind != TYPE_PROCEDURE) {
      msg_t *msg = msg_new(checker->ctx->src, call->name->region,
        MSG_ERROR, "`%s` is not a procedure", call->name->symbol->ptr);
      msg_emit(msg);
      return;
    }

    if (def->ast == checker->enclosure) {
      msg_t *msg = msg_new(checker->ctx->src, call->name->region,
        MSG_ERROR, "recursive call of procedure is not allowed");
      msg_emit(msg);
      return;
    }

    {
      const type_procedure_t *proc = (type_procedure_t *) type;

      const ast_expr_t *args;
      const subst_t    *params;

      long arg_cnt   = 0;
      long param_cnt = 0;
      for (args = call->args; args; args = args->next) {
        ++arg_cnt;
      }
      for (params = proc->params; params; params = params->next) {
        ++param_cnt;
      }
      if (arg_cnt != param_cnt) {
        msg_t *msg = msg_new(checker->ctx->src, call->name->region, MSG_ERROR, "wrong number of arguments");
        msg_add_inline(msg, call->name->region, "expected %ld arguments, supplied %ld arguments", param_cnt, arg_cnt);
        msg_emit(msg);
        return;
      }

      for (args = call->args, params = proc->params; args; args = args->next, params = params->next) {
        const type_t *type = check_expr(checker, args);
        if (proc->params->type != type) {
          msg_t *msg = msg_new(checker->ctx->src, args->region, MSG_ERROR, "mismatching argument type");
          char   expected[1024], found[1024];
          strcpy(expected, str_type(params->type));
          strcpy(found, str_type(type));
          msg_add_inline(msg, args->region, "expected `%s`, found `%s`", expected, found);
          msg_emit(msg);
        }
      }
    }
    return;
  }
  case AST_STMT_KIND_READ: {
    const ast_stmt_read_t *read = (ast_stmt_read_t *) stmt;
    const ast_expr_t      *expr = read->args;
    assert(expr->kind == AST_EXPR_KIND_DECL_REF || expr->kind == AST_EXPR_KIND_ARRAY_SUBSCRIPT);
    for (; expr; expr = expr->next) {
      const type_t *type = check_expr(checker, expr);
      if (type->kind != TYPE_CHAR && type->kind != TYPE_INTEGER) {
        msg_t *msg = msg_new(checker->ctx->src, expr->region,
          MSG_ERROR, "cannot read value for reference to `%s`", str_type(type));
        msg_add_inline(msg, expr->region,
          "arguments for read statements are of reference to integer or char");
        msg_emit(msg);
      }
    }
    return;
  }
  default:
    /* do nothing */
    return;
  }
}

static void visit_out_fmt(ast_visitor_t *visitor, const ast_out_fmt_t *fmt)
{
  checker_t    *checker = (checker_t *) visitor;
  const type_t *type    = check_expr(checker, fmt->expr);

  if (!type) {
    return;
  }

  if (!is_std_type(type) && type->kind != TYPE_STRING) {
    msg_t *msg = msg_new(checker->ctx->src, fmt->expr->region,
      MSG_ERROR, "cannot write value of type `%s`", str_type(type));
    msg_add_inline(msg, fmt->expr->region,
      "arguments for write statements are of standard types");
    msg_emit(msg);
  }
}

static void visit_stmt(ast_visitor_t *visitor, const ast_stmt_t *stmt)
{
  check_stmt((checker_t *) visitor, stmt);
  ast_walk_stmt(visitor, stmt);
}

static void visit_decl_part(ast_visitor_t *visitor, const ast_decl_part_t *decl_part)
{
  checker_t *checker = (checker_t *) visitor;
  switch (decl_part->kind) {
  case AST_DECL_PART_PROCEDURE: {
    const void *outer  = checker->enclosure;
    checker->enclosure = decl_part;
    ast_walk_decl_part(visitor, decl_part);
    checker->enclosure = outer;
    break;
  }
  default:
    ast_walk_decl_part(visitor, decl_part);
    break;
  }
}

static void visit_program(ast_visitor_t *visitor, const ast_program_t *program)
{
  checker_t  *checker = (checker_t *) visitor;
  const void *outer   = checker->enclosure;
  checker->enclosure  = program;
  ast_walk_program(visitor, program);
  checker->enclosure = outer;
}

void mpplc_check(context_t *ctx)
{
  checker_t      checker;
  ast_visitor_t *visitor = (ast_visitor_t *) &checker;

  ctx->infer_result = hash_map_new(NULL, NULL);
  checker.ctx       = ctx;

  ast_init_visitor(visitor);
  visitor->visit_out_fmt   = &visit_out_fmt;
  visitor->visit_stmt      = &visit_stmt;
  visitor->visit_decl_part = &visit_decl_part;
  visitor->visit_program   = &visit_program;
  ast_walk(visitor, ctx->ast);
}
