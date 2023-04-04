#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "ast.h"
#include "ir.h"
#include "message.h"
#include "pretty_printer.h"
#include "utility.h"

typedef struct {
  const source_t *source;
  ir_factory_t   *factory;
  ir_block_t     *break_dest;
} lowerer_t;

void maybe_error_conflict(lowerer_t *lowerer, const symbol_t *symbol, region_t region)
{
  ir_item_t *item = ir_item_lookup_scope(lowerer->factory->scope, symbol);
  if (item) {
    msg_t *msg = new_msg(lowerer->source, region, MSG_ERROR, "conflicting names");
    msg_add_inline_entry(msg, item->name_region, "first used here");
    msg_add_inline_entry(msg, region, "second used here");
    msg_emit(msg);
    exit(EXIT_FAILURE);
  }
}

void maybe_error_undeclared(lowerer_t *lowerer, const symbol_t *symbol, region_t region)
{
  if (!ir_item_lookup(lowerer->factory->scope, symbol)) {
    msg_t *msg = new_msg(lowerer->source, region,
      MSG_ERROR, "`%.*s` is not declared", (int) symbol->len, symbol->ptr);
    msg_emit(msg);
    exit(EXIT_FAILURE);
  }
}

const ir_type_t *lower_type(lowerer_t *lowerer, ast_type_t *type)
{
  switch (type->kind) {
  case AST_TYPE_KIND_BOOLEAN:
    return ir_type_boolean(lowerer->factory);
  case AST_TYPE_KIND_CHAR:
    return ir_type_char(lowerer->factory);
  case AST_TYPE_KIND_INTEGER:
    return ir_type_integer(lowerer->factory);
  case AST_TYPE_KIND_ARRAY: {
    const ir_type_t *base_type     = lower_type(lowerer, type->type.array.base);
    ir_type_t       *base_type_ref = ir_type_ref(base_type);
    long             size          = type->type.array.size->lit.number.value;
    if (size == 0) {
      msg_t *msg = new_msg(lowerer->source, type->type.array.size->region,
        MSG_ERROR, "size of array needs to be greater than 0");
      msg_emit(msg);
      exit(EXIT_FAILURE);
    }
    return ir_type_array(lowerer->factory, base_type_ref, size);
  }
  }
  unreachable();
}

ir_operand_t *lower_expr(lowerer_t *lowerer, ir_block_t **block, ast_expr_t *expr);

ir_place_t *lower_lvalue(lowerer_t *lowerer, ir_block_t **block, ast_expr_t *expr)
{
  switch (expr->kind) {
  case AST_EXPR_KIND_DECL_REF: {
    ast_ident_t *ident  = expr->expr.decl_ref.decl;
    ir_item_t   *lookup = ir_item_lookup(lowerer->factory->scope, ident->symbol);
    maybe_error_undeclared(lowerer, ident->symbol, ident->region);
    return new_ir_plain_place(ir_local_for(lowerer->factory, lookup, ident->region.pos));
  }
  case AST_EXPR_KIND_ARRAY_SUBSCRIPT: {
    ir_operand_t    *index      = lower_expr(lowerer, block, expr->expr.array_subscript.subscript);
    ast_ident_t     *ident      = expr->expr.array_subscript.decl;
    ir_item_t       *lookup     = ir_item_lookup(lowerer->factory->scope, ident->symbol);
    const ir_type_t *index_type = ir_operand_type(index);

    maybe_error_undeclared(lowerer, ident->symbol, ident->region);
    if (!ir_type_is_kind(lookup->type, IR_TYPE_ARRAY)) {
      const symbol_t *symbol = expr->expr.decl_ref.decl->symbol;
      msg_t          *msg    = new_msg(lowerer->source, ident->region,
                    MSG_ERROR, "`%.*s` is not an array", (int) symbol->len, symbol->ptr);
      msg_emit(msg);
      exit(EXIT_FAILURE);
    }
    if (!ir_type_is_kind(index_type, IR_TYPE_INTEGER)) {
      region_t region = expr->expr.array_subscript.subscript->region;
      msg_t   *msg    = new_msg(lowerer->source, region,
             MSG_ERROR, "arrays cannot be indexed by `%s`", ir_type_str(index_type));
      msg_add_inline_entry(msg, region, "array indices are of type integer");
      msg_emit(msg);
      exit(EXIT_FAILURE);
    }
    return new_ir_index_place(ir_local_for(lowerer->factory, lookup, ident->region.pos), index);
  }
  default:
    unreachable();
  }
}

void error_invalid_binary_expr(
  lowerer_t *lowerer, ast_expr_binary_t *expr,
  const ir_type_t *lhs_type, const ir_type_t *rhs_type, const char *expected)
{
  msg_t *msg;
  assert(lowerer && expr);
  msg = new_msg(lowerer->source, expr->op_region,
    MSG_ERROR, "invalid operands for `%s`", pp_binary_operator_str(expr->kind));
  msg_add_inline_entry(msg, expr->lhs->region, "%s", ir_type_str(lhs_type));
  msg_add_inline_entry(msg, expr->op_region,
    "operator `%s` takes two operands of %s", pp_binary_operator_str(expr->kind), expected);
  msg_add_inline_entry(msg, expr->rhs->region, "%s", ir_type_str(rhs_type));
  msg_emit(msg);
}

ir_operand_t *lower_binary_expr(lowerer_t *lowerer, ir_block_t **block, ast_expr_binary_t *expr)
{
  if (expr->lhs->kind == AST_EXPR_KIND_EMPTY) {
    ir_operand_t    *rhs   = lower_expr(lowerer, block, expr->rhs);
    const ir_type_t *rtype = ir_operand_type(rhs);

    if (!ir_type_is_kind(rtype, IR_TYPE_INTEGER)) {
      msg_t *msg = new_msg(lowerer->source, expr->op_region,
        MSG_ERROR, "`%s` cannot be prefixed by `%s`", ir_type_str(rtype), pp_binary_operator_str(expr->kind));
      msg_emit(msg);
      exit(EXIT_FAILURE);
    }

    switch (expr->kind) {
    case AST_EXPR_BINARY_KIND_PLUS:
      return rhs;
    case AST_EXPR_BINARY_KIND_MINUS: {
      ir_operand_t     *lhs    = new_ir_constant_operand(ir_number_constant(lowerer->factory, 0));
      const ir_local_t *result = ir_local_temp(lowerer->factory, ir_type_integer(lowerer->factory));
      ir_block_push_assign(*block, new_ir_plain_place(result), new_ir_binary_op_rvalue(expr->kind, lhs, rhs));
      return new_ir_place_operand(new_ir_plain_place(result));
    }
    default:
      unreachable();
    }
  } else {
    ir_operand_t    *lhs   = lower_expr(lowerer, block, expr->lhs);
    const ir_type_t *ltype = ir_operand_type(lhs);

    switch (expr->kind) {
    case AST_EXPR_BINARY_KIND_EQUAL:
    case AST_EXPR_BINARY_KIND_NOTEQ:
    case AST_EXPR_BINARY_KIND_LE:
    case AST_EXPR_BINARY_KIND_LEEQ:
    case AST_EXPR_BINARY_KIND_GR:
    case AST_EXPR_BINARY_KIND_GREQ: {
      ir_operand_t     *rhs    = lower_expr(lowerer, block, expr->rhs);
      const ir_type_t  *rtype  = ir_operand_type(rhs);
      const ir_local_t *result = ir_local_temp(lowerer->factory, ir_type_boolean(lowerer->factory));
      ir_block_push_assign(*block, new_ir_plain_place(result), new_ir_binary_op_rvalue(expr->kind, lhs, rhs));
      if (ltype != rtype || !ir_type_is_std(ltype) || !ir_type_is_std(rtype)) {
        error_invalid_binary_expr(lowerer, expr, ltype, rtype, "the same standard type");
        exit(EXIT_FAILURE);
      }
      return new_ir_place_operand(new_ir_plain_place(result));
    }
    case AST_EXPR_BINARY_KIND_PLUS:
    case AST_EXPR_BINARY_KIND_MINUS:
    case AST_EXPR_BINARY_KIND_STAR:
    case AST_EXPR_BINARY_KIND_DIV: {
      ir_operand_t     *rhs    = lower_expr(lowerer, block, expr->rhs);
      const ir_type_t  *rtype  = ir_operand_type(rhs);
      const ir_local_t *result = ir_local_temp(lowerer->factory, ir_type_integer(lowerer->factory));
      ir_block_push_assign(*block, new_ir_plain_place(result), new_ir_binary_op_rvalue(expr->kind, lhs, rhs));
      if (!ir_type_is_kind(ltype, IR_TYPE_INTEGER) || !ir_type_is_kind(rtype, IR_TYPE_INTEGER)) {
        error_invalid_binary_expr(lowerer, expr, ltype, rtype, "type integer");
        exit(EXIT_FAILURE);
      }
      return new_ir_place_operand(new_ir_plain_place(result));
    }
    case AST_EXPR_BINARY_KIND_OR:
    case AST_EXPR_BINARY_KIND_AND: {
      ir_block_t       *els          = ir_block(lowerer->factory);
      ir_block_t       *shortcircuit = ir_block(lowerer->factory);
      const ir_local_t *result       = ir_local_temp(lowerer->factory, ir_type_boolean(lowerer->factory));
      ir_block_push_assign(*block, new_ir_plain_place(result), new_ir_use_rvalue(lhs));
      switch (expr->kind) {
      case AST_EXPR_BINARY_KIND_OR:
        ir_block_terminate_if(*block, new_ir_place_operand(new_ir_plain_place(result)), shortcircuit, els);
        break;
      case AST_EXPR_BINARY_KIND_AND:
        ir_block_terminate_if(*block, new_ir_place_operand(new_ir_plain_place(result)), els, shortcircuit);
        break;
      default:
        unreachable();
        break;
      }
      {
        ir_operand_t    *rhs   = lower_expr(lowerer, &els, expr->rhs);
        const ir_type_t *rtype = ir_operand_type(rhs);
        if (!ir_type_is_kind(ltype, IR_TYPE_BOOLEAN) || !ir_type_is_kind(rtype, IR_TYPE_BOOLEAN)) {
          error_invalid_binary_expr(lowerer, expr, ltype, rtype, "type boolean");
          exit(EXIT_FAILURE);
        }
        ir_block_push_assign(els, new_ir_plain_place(result), new_ir_use_rvalue(rhs));
        ir_block_terminate_goto(els, shortcircuit);
        *block = shortcircuit;
        return new_ir_place_operand(new_ir_plain_place(result));
      }
    }
    default:
      unreachable();
    }
  }
}

ir_operand_t *lower_not_expr(lowerer_t *lowerer, ir_block_t **block, ast_expr_not_t *expr)
{
  ir_operand_t     *operand = lower_expr(lowerer, block, expr->expr);
  const ir_type_t  *type    = ir_operand_type(operand);
  const ir_local_t *result  = ir_local_temp(lowerer->factory, ir_type_boolean(lowerer->factory));
  ir_block_push_assign(*block, new_ir_plain_place(result), new_ir_not_rvalue(operand));
  if (!ir_type_is_kind(type, IR_TYPE_BOOLEAN)) {
    msg_t *msg = new_msg(lowerer->source, expr->op_region,
      MSG_ERROR, "invalid operands for `not`");
    msg_add_inline_entry(msg, expr->op_region, "operator `not` takes one operand of type boolean");
    msg_add_inline_entry(msg, expr->expr->region, "%s", ir_type_str(type));
    msg_emit(msg);
    exit(EXIT_FAILURE);
  }
  return new_ir_place_operand(new_ir_plain_place(result));
}

ir_operand_t *lower_cast_expr(lowerer_t *lowerer, ir_block_t **block, ast_expr_cast_t *expr)
{
  ir_operand_t     *operand      = lower_expr(lowerer, block, expr->cast);
  const ir_type_t  *operand_type = ir_operand_type(operand);
  const ir_type_t  *cast_type    = lower_type(lowerer, expr->type);
  const ir_local_t *result       = ir_local_temp(lowerer->factory, cast_type);
  ir_block_push_assign(*block, new_ir_plain_place(result), new_ir_cast_rvalue(cast_type, operand));
  if (!ir_type_is_std(operand_type)) {
    msg_t *msg = new_msg(lowerer->source, expr->cast->region,
      MSG_ERROR, "expression of type `%s` cannot be cast", ir_type_str(operand_type));
    msg_add_inline_entry(msg, expr->cast->region, "expressions to be cast are of standard types");
    msg_emit(msg);
    exit(EXIT_FAILURE);
  }
  if (!ir_type_is_std(cast_type)) {
    msg_t *msg = new_msg(lowerer->source, expr->cast->region,
      MSG_ERROR, "expression cannot be cast to `%s`", ir_type_str(cast_type));
    msg_add_inline_entry(msg, expr->type->region, "expressions can be cast to standard types");
    msg_emit(msg);
    exit(EXIT_FAILURE);
  }
  return new_ir_place_operand(new_ir_plain_place(result));
}

ir_operand_t *lower_constant_expr(lowerer_t *lowerer, ir_block_t **block, ast_expr_constant_t *expr)
{
  assert(lowerer && block && expr);

  switch (expr->lit->kind) {
  case AST_LIT_KIND_NUMBER:
    return new_ir_constant_operand(ir_number_constant(lowerer->factory, expr->lit->lit.number.value));
  case AST_LIT_KIND_BOOLEAN:
    return new_ir_constant_operand(ir_boolean_constant(lowerer->factory, expr->lit->lit.boolean.value));
  case AST_LIT_KIND_STRING: {
    const symbol_t *symbol = expr->lit->lit.string.symbol;
    if (expr->lit->lit.string.str_len != 1) {
      msg_t *msg = new_msg(lowerer->source, expr->lit->region,
        MSG_ERROR, "string is not a valid expression");
      msg_emit(msg);
      exit(EXIT_FAILURE);
    }
    return new_ir_constant_operand(ir_char_constant(lowerer->factory, symbol->ptr[0]));
  }
  }
  unreachable();
}

ir_operand_t *lower_expr(lowerer_t *lowerer, ir_block_t **block, ast_expr_t *expr)
{
  switch (expr->kind) {
  case AST_EXPR_KIND_DECL_REF:
  case AST_EXPR_KIND_ARRAY_SUBSCRIPT: {
    ir_place_t *place = lower_lvalue(lowerer, block, expr);
    return new_ir_place_operand(place);
  }
  case AST_EXPR_KIND_BINARY:
    return lower_binary_expr(lowerer, block, &expr->expr.binary);
  case AST_EXPR_KIND_NOT:
    return lower_not_expr(lowerer, block, &expr->expr.not_);
  case AST_EXPR_KIND_PAREN:
    return lower_expr(lowerer, block, expr->expr.paren.inner);
  case AST_EXPR_KIND_CAST:
    return lower_cast_expr(lowerer, block, &expr->expr.cast);
  case AST_EXPR_KIND_CONSTANT:
    return lower_constant_expr(lowerer, block, &expr->expr.constant);
  default:
    unreachable();
  }
}

ir_operand_t *lower_stmt_call_param(lowerer_t *lowerer, ir_block_t **block, ast_expr_t *args, ir_type_t *param_type)
{
  if (args && param_type) {
    ir_block_t      *blk  = *block;
    ir_operand_t    *next = lower_stmt_call_param(lowerer, &blk, args->next, param_type->next);
    ir_operand_t    *ret  = lower_expr(lowerer, &blk, args);
    const ir_type_t *type = ir_operand_type(ret);
    ret->next             = next;
    if (param_type->type.ref != type) {
      msg_t *msg = new_msg(lowerer->source, args->region, MSG_ERROR, "mismatching argument type");
      char   expected[1024], found[1024];
      strcpy(expected, ir_type_str(param_type->type.ref));
      strcpy(found, ir_type_str(type));
      msg_add_inline_entry(msg, args->region, "expected `%s`, found `%s`", expected, found);
      msg_emit(msg);
      exit(EXIT_FAILURE);
    }
    *block = ir_block(lowerer->factory);
    ir_block_terminate_arg(blk, ret, *block);
    return ret;
  } else {
    return NULL;
  }
}

void lower_stmt(lowerer_t *lowerer, ir_block_t **block, ast_stmt_t *stmt)
{
  for (; stmt; stmt = stmt->next) {
    switch (stmt->kind) {
    case AST_STMT_KIND_ASSIGN: {
      ast_stmt_assign_t *stmt_assign = (ast_stmt_assign_t *) stmt;
      ir_place_t        *lhs         = lower_lvalue(lowerer, block, stmt_assign->lhs);
      ir_operand_t      *rhs         = lower_expr(lowerer, block, stmt_assign->rhs);
      const ir_type_t   *ltype       = ir_place_type(lhs);
      const ir_type_t   *rtype       = ir_operand_type(rhs);

      if (ltype != rtype || !ir_type_is_std(ltype) || !ir_type_is_std(rtype)) {
        msg_t *msg = new_msg(lowerer->source, stmt_assign->op_region,
          MSG_ERROR, "invalid operands for `:=`");
        msg_add_inline_entry(msg, stmt_assign->lhs->region, "%s", ir_type_str(ltype));
        msg_add_inline_entry(msg, stmt_assign->op_region,
          "operator `:=` takes two operands of the same standard type");
        msg_add_inline_entry(msg, stmt_assign->rhs->region, "%s", ir_type_str(rtype));
        msg_emit(msg);
        exit(EXIT_FAILURE);
      }

      ir_block_push_assign(*block, lhs, new_ir_use_rvalue(rhs));
      break;
    }
    case AST_STMT_KIND_IF: {
      ast_stmt_if_t   *stmt_if = (ast_stmt_if_t *) stmt;
      ir_operand_t    *cond    = lower_expr(lowerer, block, stmt_if->cond);
      ir_block_t      *then    = ir_block(lowerer->factory);
      ir_block_t      *els     = ir_block(lowerer->factory);
      const ir_type_t *type    = ir_operand_type(cond);

      if (!ir_type_is_kind(type, IR_TYPE_BOOLEAN)) {
        msg_t *msg = new_msg(lowerer->source, stmt_if->cond->region,
          MSG_ERROR, "expression of type `%s` cannot be condition", ir_type_str(type));
        msg_add_inline_entry(msg, stmt_if->cond->region, "condition expressions are of type boolean");
        msg_emit(msg);
        exit(EXIT_FAILURE);
      }

      ir_block_terminate_if(*block, cond, then, els);
      lower_stmt(lowerer, &then, stmt_if->then_stmt);
      if (stmt_if->else_stmt) {
        ir_block_t *join = ir_block(lowerer->factory);
        lower_stmt(lowerer, &els, stmt_if->else_stmt);
        ir_block_terminate_goto(els, join);
        els = join;
      }
      ir_block_terminate_goto(then, els);
      *block = els;
      break;
    }
    case AST_STMT_KIND_WHILE: {
      ast_stmt_while_t *stmt_while     = (ast_stmt_while_t *) stmt;
      ir_block_t       *cond_begin     = ir_block(lowerer->factory);
      ir_block_t       *cond_end       = cond_begin;
      ir_operand_t     *cond           = lower_expr(lowerer, &cond_end, stmt_while->cond);
      ir_block_t       *pre_break_dest = lowerer->break_dest;
      ir_block_t       *do_            = ir_block(lowerer->factory);
      const ir_type_t  *type           = ir_operand_type(cond);

      if (!ir_type_is_kind(type, IR_TYPE_BOOLEAN)) {
        msg_t *msg = new_msg(lowerer->source, stmt_while->cond->region,
          MSG_ERROR, "expression of type `%s` cannot be condition", ir_type_str(type));
        msg_add_inline_entry(msg, stmt_while->cond->region, "condition expressions are of type boolean");
        msg_emit(msg);
        exit(EXIT_FAILURE);
      }

      lowerer->break_dest = ir_block(lowerer->factory);
      ir_block_terminate_if(cond_end, cond, do_, lowerer->break_dest);
      lower_stmt(lowerer, &do_, stmt_while->do_stmt);
      ir_block_terminate_goto(*block, cond_begin);
      ir_block_terminate_goto(do_, cond_begin);
      *block              = lowerer->break_dest;
      lowerer->break_dest = pre_break_dest;
      break;
    }
    case AST_STMT_KIND_BREAK: {
      ir_block_terminate_goto(*block, lowerer->break_dest);
      *block = ir_block(lowerer->factory);
      break;
    }
    case AST_STMT_KIND_CALL: {
      ast_stmt_call_t *stmt_call = (ast_stmt_call_t *) stmt;
      ast_ident_t     *ident     = stmt_call->name;
      ir_item_t       *item      = ir_item_lookup(lowerer->factory->scope, ident->symbol);

      maybe_error_undeclared(lowerer, ident->symbol, ident->region);
      if (item->kind != IR_ITEM_PROCEDURE) {
        const symbol_t *symbol = stmt_call->name->symbol;
        msg_t          *msg    = new_msg(lowerer->source, ident->region,
                      MSG_ERROR, "`%.*s` is not a procedure", (int) symbol->len, symbol->ptr);
        msg_emit(msg);
        exit(EXIT_FAILURE);
      }

      {
        ir_scope_t *scope = lowerer->factory->scope;
        for (; scope; scope = scope->next) {
          if (scope->owner->symbol == item->symbol && scope->owner->kind == IR_ITEM_PROCEDURE) {
            msg_t *msg = new_msg(lowerer->source, ident->region,
              MSG_ERROR, "recursive call of procedure is not allowed");
            msg_emit(msg);
            exit(EXIT_FAILURE);
          }
        }
      }

      {
        const ir_local_t *func       = ir_local_for(lowerer->factory, item, ident->region.pos);
        ir_type_t        *param_type = ir_local_type(func)->type.procedure.param_types;
        ast_expr_t       *args       = stmt_call->args;
        ir_type_t        *types      = param_type;
        ir_operand_t     *arg        = lower_stmt_call_param(lowerer, block, stmt_call->args, param_type);
        long              arg_cnt    = 0;
        long              type_cnt   = 0;
        for (; args; args = args->next) {
          ++arg_cnt;
        }
        for (; types; types = types->next) {
          ++type_cnt;
        }
        if (arg_cnt != type_cnt) {
          msg_t *msg = new_msg(lowerer->source, ident->region, MSG_ERROR, "wrong number of arguments");
          msg_add_inline_entry(msg, ident->region, "expected %ld arguments, supplied %ld arguments", type_cnt, arg_cnt);
          msg_emit(msg);
          exit(EXIT_FAILURE);
        }
        ir_block_push_call(*block, new_ir_plain_place(func), arg);
      }
      break;
    }
    case AST_STMT_KIND_RETURN: {
      ir_block_terminate_return(*block);
      *block = ir_block(lowerer->factory);
      break;
    }
    case AST_STMT_KIND_READ: {
      ast_stmt_read_t *stmt_read = (ast_stmt_read_t *) stmt;
      ast_expr_t      *args      = stmt_read->args;

      for (; args; args = args->next) {
        if (args->kind != AST_EXPR_KIND_DECL_REF && args->kind != AST_EXPR_KIND_ARRAY_SUBSCRIPT) {
          msg_t *msg = new_msg(lowerer->source, args->region,
            MSG_ERROR, "cannot read value for expression");
          msg_add_inline_entry(msg, args->region,
            "arguments for read statements are of reference to integer or char");
          msg_emit(msg);
          exit(EXIT_FAILURE);
        }

        {
          ir_place_t      *ref  = lower_lvalue(lowerer, block, args);
          const ir_type_t *type = ir_place_type(ref);
          if (!ir_type_is_kind(type, IR_TYPE_INTEGER) && !ir_type_is_kind(type, IR_TYPE_CHAR)) {
            msg_t *msg = new_msg(lowerer->source, args->region,
              MSG_ERROR, "cannot read value for reference to `%s`", ir_type_str(type));
            msg_add_inline_entry(msg, args->region,
              "arguments for read statements are of reference to integer or char");
            msg_emit(msg);
            exit(EXIT_FAILURE);
          }

          ir_block_push_read(*block, ref);
        }
      }

      if (stmt_read->newline) {
        ir_block_push_readln(*block);
      }
      break;
    }
    case AST_STMT_KIND_WRITE: {
      ast_stmt_write_t *stmt_write = (ast_stmt_write_t *) stmt;
      ast_out_fmt_t    *formats    = stmt_write->formats;

      for (; formats; formats = formats->next) {
        ast_expr_constant_t *constant = &formats->expr->expr.constant;
        ast_lit_string_t    *string   = &constant->lit->lit.string;
        if (formats->expr->kind == AST_EXPR_KIND_CONSTANT && constant->lit->kind == AST_LIT_KIND_STRING && string->str_len != 1) {
          const ir_constant_t *constant = ir_string_constant(lowerer->factory, string->symbol, string->str_len);
          ir_block_push_write(*block, new_ir_constant_operand(constant), NULL);
        } else {
          ir_operand_t    *value = lower_expr(lowerer, block, formats->expr);
          const ir_type_t *type  = ir_operand_type(value);
          if (!ir_type_is_std(type)) {
            msg_t *msg = new_msg(lowerer->source, formats->expr->region,
              MSG_ERROR, "cannot write value of type `%s`", ir_type_str(type));
            msg_add_inline_entry(msg, formats->expr->region,
              "arguments for write statements are of standard types");
            msg_emit(msg);
            exit(EXIT_FAILURE);
          }

          if (formats->len) {
            ir_block_push_write(*block, value, ir_number_constant(lowerer->factory, formats->len->lit.number.value));
          } else {
            ir_block_push_write(*block, value, NULL);
          }
        }
      }

      if (stmt_write->newline) {
        ir_block_push_writeln(*block);
      }
      break;
    }
    case AST_STMT_KIND_COMPOUND: {
      ast_stmt_compound_t *stmt_compound = (ast_stmt_compound_t *) stmt;
      lower_stmt(lowerer, block, stmt_compound->stmts);
      break;
    }
    case AST_STMT_KIND_EMPTY:
      break;
    }
  }
}

ir_type_t *lower_param_types(lowerer_t *lowerer, ast_decl_param_t *decl)
{
  ir_type_t  *ret  = NULL;
  ir_type_t **last = &ret;
  for (; decl; decl = decl->next) {
    ast_ident_t     *ident = decl->names;
    const ir_type_t *type  = lower_type(lowerer, decl->type);
    if (!ir_type_is_std(type)) {
      msg_t *msg = new_msg(lowerer->source, decl->type->region,
        MSG_ERROR, "invalid parameter of type `%s`", ir_type_str(type));
      msg_add_inline_entry(msg, decl->type->region,
        "parameters are of standard types");
      msg_emit(msg);
      exit(EXIT_FAILURE);
    }
    for (; ident; ident = ident->next) {
      *last = ir_type_ref(type);
      last  = &(*last)->next;
    }
  }
  return ret;
}

void lower_variable_decl(lowerer_t *lowerer, ast_decl_variable_t *decl, int local)
{
  for (; decl; decl = decl->next) {
    ast_ident_t     *ident = decl->names;
    const ir_type_t *type  = lower_type(lowerer, decl->type);
    for (; ident; ident = ident->next) {
      ir_item_kind_t kind = local ? IR_ITEM_LOCAL_VAR : IR_ITEM_VAR;
      maybe_error_conflict(lowerer, ident->symbol, ident->region);
      ir_item(lowerer->factory, kind, ident->symbol, ident->region, type);
    }
  }
}

void lower_param_decl(lowerer_t *lowerer, ast_decl_param_t *decl)
{
  for (; decl; decl = decl->next) {
    ast_ident_t     *ident = decl->names;
    const ir_type_t *type  = lower_type(lowerer, decl->type);
    if (!ir_type_is_std(type)) {
      msg_t *msg = new_msg(lowerer->source, decl->type->region,
        MSG_ERROR, "invalid parameter of type `%s`", ir_type_str(type));
      msg_add_inline_entry(msg, decl->type->region,
        "parameters are of standard types");
      msg_emit(msg);
      exit(EXIT_FAILURE);
    }
    for (; ident; ident = ident->next) {
      maybe_error_conflict(lowerer, ident->symbol, ident->region);
      ir_item(lowerer->factory, IR_ITEM_ARG_VAR, ident->symbol, ident->region, type);
    }
  }
}

void lower_decl_part(lowerer_t *lowerer, ast_decl_part_t *decl_part)
{
  for (; decl_part; decl_part = decl_part->next) {
    switch (decl_part->kind) {
    case AST_DECL_PART_VARIABLE: {
      ast_decl_part_variable_t *decl = (ast_decl_part_variable_t *) decl_part;
      lower_variable_decl(lowerer, decl->decls, 0);
      break;
    }
    case AST_DECL_PART_PROCEDURE: {
      ir_item_t                 *item;
      ir_block_t                *block_begin = ir_block(lowerer->factory);
      ast_decl_part_procedure_t *decl        = (ast_decl_part_procedure_t *) decl_part;
      ir_type_t                 *param_types = lower_param_types(lowerer, decl->params);
      const ir_type_t           *proc_type   = ir_type_procedure(lowerer->factory, param_types);

      maybe_error_conflict(lowerer, decl->name->symbol, decl->name->region);
      item = ir_item(lowerer->factory, IR_ITEM_PROCEDURE, decl->name->symbol, decl->name->region, proc_type);
      ir_scope_start(lowerer->factory, item);
      {
        ast_decl_part_variable_t *decl_part = (ast_decl_part_variable_t *) decl->variables;
        ir_block_t               *block_end = block_begin;
        lower_param_decl(lowerer, decl->params);
        if (decl_part) {
          lower_variable_decl(lowerer, decl_part->decls, 1);
        }
        lower_stmt(lowerer, &block_end, decl->stmt);
        ir_block_terminate_return(block_end);
      }
      ir_scope_end(lowerer->factory, block_begin);
      break;
    }
    }
  }
}

ir_item_t *lower_program(lowerer_t *lowerer, ast_program_t *program)
{
  ir_item_t  *item        = ir_item(lowerer->factory, IR_ITEM_PROGRAM, program->name->symbol, program->name->region, ir_type_program(lowerer->factory));
  ir_block_t *block_begin = ir_block(lowerer->factory);
  ir_scope_start(lowerer->factory, item);
  {
    ir_block_t *block_end = block_begin;
    lower_decl_part(lowerer, program->decl_part);
    lower_stmt(lowerer, &block_end, program->stmt);
    ir_block_terminate_return(block_end);
  }
  ir_scope_end(lowerer->factory, block_begin);
  return item;
}

ir_t *lower_ast(ast_t *ast)
{
  ir_item_t *items;
  lowerer_t  lowerer;
  assert(ast);

  lowerer.source  = ast->source;
  lowerer.factory = new_ir_factory();
  items           = lower_program(&lowerer, ast->program);
  return new_ir(lowerer.source, items, lowerer.factory);
}
