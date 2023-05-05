#include <assert.h>

#include "ast.h"
#include "context.h"
#include "mpplc.h"
#include "types.h"
#include "utility.h"

typedef struct checker__s checker_t;

struct checker__s {
  ast_visitor_t visitor;
  context_t    *ctx;
};

static int is_std_type(const type_t *type)
{
  return type->kind == TYPE_BOOLEAN || type->kind == TYPE_CHAR || type->kind == TYPE_INTEGER;
}

static const type_t *check_lit(checker_t *checker, const ast_lit_t *lit)
{
  switch (lit->kind) {
  case AST_LIT_KIND_BOOLEAN: {
    return ctx_mk_type_boolean(checker->ctx);
  }
  case AST_LIT_KIND_NUMBER: {
    return ctx_mk_type_integer(checker->ctx);
  }
  case AST_LIT_KIND_STRING: {
    const ast_lit_string_t *string = (ast_lit_string_t *) lit;
    if (string->str_len == 1) {
      return ctx_mk_type_char(checker->ctx);
    } else {
      return ctx_mk_type_string(checker->ctx);
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
    return ctx_mk_type_boolean(checker->ctx);
  }
  case AST_TYPE_KIND_CHAR: {
    return ctx_mk_type_char(checker->ctx);
  }
  case AST_TYPE_KIND_INTEGER: {
    return ctx_mk_type_integer(checker->ctx);
  }
  case AST_TYPE_KIND_ARRAY: {
    const ast_type_array_t *array = (ast_type_array_t *) type;
    const type_t           *base  = check_type(checker, array->base);
    subst_t                *subst = ctx_loan_subst(checker->ctx, base);

    assert(array->size->kind == AST_LIT_KIND_NUMBER);
    return ctx_mk_type_array(checker->ctx, subst, ((ast_lit_number_t *) array->size)->value);
  }
  default:
    unreachable();
  }
}

const type_t *check_def(checker_t *checker, const def_t *def)
{
  const hash_entry_t *entry = hash_find(checker->ctx->types, def->ast);
  if (entry) {
    return entry->value;
  } else {
    const type_t *type = NULL;
    switch (def->kind) {
    case DEF_PROGRAM: {
      type = ctx_mk_type_program(checker->ctx);
      break;
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
          subst_t *subst = ctx_loan_subst(checker->ctx, param);
          *tail          = subst;
          tail           = &subst->next;
        }
      }
      type = ctx_mk_type_procedure(checker->ctx, types);
      break;
    }
    case DEF_VAR: {
      const ast_decl_variable_t *var = def->ast;
      type                           = check_type(checker, var->type);
      break;
    }
    case DEF_PARAM: {
      const ast_decl_param_t *param = def->ast;
      type                          = check_type(checker, param->type);
      break;
    }
    }
    hash_insert_unchecked(checker->ctx->types, (void *) def->ast, (void *) type);
    return type;
  }
}

const type_t *check_expr(checker_t *checker, const ast_expr_t *expr)
{
  switch (expr->kind) {
  case AST_EXPR_KIND_DECL_REF: {
    const ast_expr_decl_ref_t *ref = (ast_expr_decl_ref_t *) expr;
    const def_t               *def = hash_find(checker->ctx->resolution, ref->decl)->value;
    return check_def(checker, def);
  }
  case AST_EXPR_KIND_ARRAY_SUBSCRIPT: {
    const ast_expr_array_subscript_t *subscript = (ast_expr_array_subscript_t *) expr;
    const def_t                      *def       = hash_find(checker->ctx->resolution, subscript->decl)->value;
    const type_t                     *type      = check_def(checker, def);
    if (type->kind != TYPE_ARRAY) {
      /* TODO: emit error message for nonarray */
      return NULL;
    }
    return ((type_array_t *) type)->base->type;
  }
  case AST_EXPR_KIND_BINARY: {
    const ast_expr_binary_t *binary = (ast_expr_binary_t *) expr;
    if (binary->lhs->kind == AST_EXPR_KIND_EMPTY) {
      const type_t *rhs = check_expr(checker, binary->rhs);
      assert(binary->kind == AST_EXPR_BINARY_KIND_PLUS || binary->kind == AST_EXPR_BINARY_KIND_MINUS);
      if (rhs->kind != TYPE_INTEGER) {
        /* TODO: emit error message for invalid unary */
        return NULL;
      }
      return ctx_mk_type_integer(checker->ctx);
    } else {
      const type_t *lhs = check_expr(checker, binary->lhs);
      const type_t *rhs = check_expr(checker, binary->rhs);
      switch (binary->kind) {
      case AST_EXPR_BINARY_KIND_EQUAL:
      case AST_EXPR_BINARY_KIND_NOTEQ:
      case AST_EXPR_BINARY_KIND_LE:
      case AST_EXPR_BINARY_KIND_LEEQ:
      case AST_EXPR_BINARY_KIND_GR:
      case AST_EXPR_BINARY_KIND_GREQ: {
        if (lhs != rhs || !is_std_type(lhs) || !is_std_type(rhs)) {
          /* TODO: emit error message for invalid comparison */
          return NULL;
        }
        return ctx_mk_type_boolean(checker->ctx);
      }
      case AST_EXPR_BINARY_KIND_PLUS:
      case AST_EXPR_BINARY_KIND_MINUS:
      case AST_EXPR_BINARY_KIND_STAR:
      case AST_EXPR_BINARY_KIND_DIV: {
        if (lhs->kind != TYPE_INTEGER || rhs->kind != TYPE_INTEGER) {
          /* TODO: emit error message for invalid arithmetic */
          return NULL;
        }
        return ctx_mk_type_integer(checker->ctx);
      }
      case AST_EXPR_BINARY_KIND_OR:
      case AST_EXPR_BINARY_KIND_AND: {
        if (lhs->kind != TYPE_BOOLEAN || rhs->kind != TYPE_BOOLEAN) {
          /* TODO: emit error message for invalid logical */
          return NULL;
        }
        return ctx_mk_type_boolean(checker->ctx);
      }
      default:
        unreachable();
      }
    }
  }
  case AST_EXPR_KIND_NOT: {
    const ast_expr_not_t *not_ = (ast_expr_not_t *) expr;
    const type_t         *type = check_expr(checker, not_->expr);
    if (type->kind != TYPE_BOOLEAN) {
      /* TODO: emit error message for invalid not */
      return NULL;
    }
    return ctx_mk_type_boolean(checker->ctx);
  }
  case AST_EXPR_KIND_PAREN: {
    const ast_expr_paren_t *paren = (ast_expr_paren_t *) expr;
    return check_expr(checker, paren->inner);
  }
  case AST_EXPR_KIND_CAST: {
    const ast_expr_cast_t *cast       = (ast_expr_cast_t *) expr;
    const type_t          *value_type = check_expr(checker, cast->cast);
    const type_t          *cast_type  = check_type(checker, cast->type);
    if (!is_std_type(value_type)) {
      /* TODO: emit error message for invalid value */
      return NULL;
    }
    if (!is_std_type(cast_type)) {
      /* TODO: emit error message for invalid cast */
      return NULL;
    }
    return cast_type;
  }
  case AST_EXPR_KIND_CONSTANT: {
    const ast_expr_constant_t *constant = (ast_expr_constant_t *) expr;
    return check_lit(checker, constant->lit);
  }
  default:
    return NULL;
  }
  unreachable();
}

static void visit_out_fmt(ast_visitor_t *visitor, const ast_out_fmt_t *fmt)
{
  checker_t    *checker = (checker_t *) visitor;
  const type_t *type    = check_expr(checker, fmt->expr);
  if (!is_std_type(type) || type->kind != TYPE_STRING) {
    /* TODO: emit error message for invalid expression */
    return;
  }
  if (type->kind == TYPE_STRING && fmt->len != NULL) {
    /* TODO: emit error message for invalid format */
    return;
  }
}

static void visit_stmt(ast_visitor_t *visitor, const ast_stmt_t *stmt)
{
  checker_t *checker = (checker_t *) visitor;
  switch (stmt->kind) {
  case AST_STMT_KIND_ASSIGN: {
    const ast_stmt_assign_t *assign = (ast_stmt_assign_t *) stmt;
    const type_t            *lhs    = check_expr(checker, assign->lhs);
    const type_t            *rhs    = check_expr(checker, assign->rhs);
    if (lhs != rhs) {
      /* TODO: emit error message for invalid assign */
    }
    break;
  }
  case AST_STMT_KIND_IF: {
    const ast_stmt_if_t *if_  = (ast_stmt_if_t *) stmt;
    const type_t        *cond = check_expr(checker, if_->cond);
    if (cond->kind != TYPE_BOOLEAN) {
      /* TODO: emit error message for invalid condition */
    }
    break;
  }
  case AST_STMT_KIND_WHILE: {
    const ast_stmt_while_t *while_ = (ast_stmt_while_t *) stmt;
    const type_t           *cond   = check_expr(checker, while_->cond);
    if (cond->kind != TYPE_BOOLEAN) {
      /* TODO: emit error message for invalid condition */
    }
    break;
  }
  case AST_STMT_KIND_CALL: {
    const ast_stmt_call_t *call = (ast_stmt_call_t *) stmt;
    const def_t           *def  = hash_find(checker->ctx->resolution, call->name)->value;
    const type_t          *type = check_def(checker, def);
    if (type->kind != TYPE_PROCEDURE) {
      /* TODO: emit error message for calling nonprocedure */
    }

    {
      const type_procedure_t *proc = (type_procedure_t *) type;
      const ast_expr_t       *args;
      const subst_t          *params;
      long                    arg_cnt   = 0;
      long                    param_cnt = 0;
      for (args = call->args; args; args = args->next) {
        ++arg_cnt;
      }
      for (params = proc->params; params; params = params->next) {
        ++param_cnt;
      }
      if (arg_cnt != param_cnt) {
        /* TODO: emit error message for a wrong number of arguments */
      }
      for (args = call->args, params = proc->params; args; args = args->next, params = params->next) {
        const type_t *type = check_expr(checker, args);
        if (proc->params->type != type) {
          /* TODO: emit error message for a mismatching argument */
        }
      }
    }
    break;
  }
  case AST_STMT_KIND_READ: {
    const ast_stmt_read_t *read = (ast_stmt_read_t *) stmt;
    const ast_expr_t      *expr = read->args;
    assert(expr->kind == AST_EXPR_KIND_DECL_REF || expr->kind == AST_EXPR_KIND_ARRAY_SUBSCRIPT);
    for (; expr; expr = expr->next) {
      const type_t *type = check_expr(checker, expr);
      if (type->kind != TYPE_CHAR && type->kind != TYPE_INTEGER) {
        /* TODO: emit error message for invalid read argument */
      }
    }
    break;
  }
  default:
    /* do nothing */
    break;
  }
  ast_walk_stmt(visitor, stmt);
}

void mpplc_check(context_t *ctx)
{
  checker_t      checker;
  ast_visitor_t *visitor = (ast_visitor_t *) &checker;

  ctx->types  = hash_new(&hash_default_comp, &hash_default_hasher);
  checker.ctx = ctx;

  ast_init_visitor(visitor);
  visitor->visit_out_fmt = &visit_out_fmt;
  visitor->visit_stmt    = &visit_stmt;
  ast_walk(visitor, ctx->ast);
}
