#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "ast.h"
#include "ir.h"
#include "message.h"
#include "utility.h"

typedef struct {
  const source_t *source;
  ir_factory_t   *factory;
  ir_block_t     *break_dest;
} analyzer_t;

void maybe_error_conflict(analyzer_t *analyzer, const symbol_t *symbol, region_t region)
{
  ir_item_t *item = ir_item_lookup_scope(analyzer->factory->scope, symbol);
  if (item) {
    msg_t *msg = new_msg(analyzer->source, region, MSG_ERROR, "conflicting names");
    msg_add_inline_entry(msg, item->name_region, "first used here");
    msg_add_inline_entry(msg, region, "second used here");
    msg_emit(msg);
    exit(EXIT_FAILURE);
  }
}

void maybe_error_undeclared(analyzer_t *analyzer, const symbol_t *symbol, region_t region)
{
  if (!ir_item_lookup(analyzer->factory->scope, symbol)) {
    msg_t *msg = new_msg(analyzer->source, region,
      MSG_ERROR, "`%.*s` is not declared", (int) symbol->len, symbol->ptr);
    msg_emit(msg);
    exit(EXIT_FAILURE);
  }
}

const ir_type_t *analyze_type(analyzer_t *analyzer, ast_type_t *type)
{
  assert(analyzer && type);

  switch (type->kind) {
  case AST_TYPE_KIND_BOOLEAN:
    return ir_type_boolean(analyzer->factory);
  case AST_TYPE_KIND_CHAR:
    return ir_type_char(analyzer->factory);
  case AST_TYPE_KIND_INTEGER:
    return ir_type_integer(analyzer->factory);

  case AST_TYPE_KIND_ARRAY: {
    const ir_type_t *base_type     = analyze_type(analyzer, type->type.array.base);
    ir_type_t       *base_type_ref = ir_type_ref(base_type);
    long             size          = type->type.array.size->lit.number.value;
    if (size == 0) {
      msg_t *msg = new_msg(analyzer->source, type->type.array.size->region,
        MSG_ERROR, "size of array needs to be greater than 0");
      msg_emit(msg);
      exit(EXIT_FAILURE);
    }
    return ir_type_array(analyzer->factory, base_type_ref, size);
  }
  }

  unreachable();
}

ir_operand_t *analyze_expr(analyzer_t *analyzer, ir_block_t **block, ast_expr_t *expr);

ir_place_t *analyze_lvalue(analyzer_t *analyzer, ir_block_t **block, ast_expr_t *expr)
{
  assert(analyzer && expr);

  switch (expr->kind) {
  case AST_EXPR_KIND_DECL_REF: {
    ast_ident_t *ident  = expr->expr.decl_ref.decl;
    ir_item_t   *lookup = ir_item_lookup(analyzer->factory->scope, ident->symbol);
    maybe_error_undeclared(analyzer, ident->symbol, ident->region);
    return new_ir_place(ir_local_for(analyzer->factory, lookup, ident->region.pos));
  }
  case AST_EXPR_KIND_ARRAY_SUBSCRIPT: {
    ir_operand_t    *index      = analyze_expr(analyzer, block, expr->expr.array_subscript.subscript);
    ast_ident_t     *ident      = expr->expr.array_subscript.decl;
    ir_item_t       *lookup     = ir_item_lookup(analyzer->factory->scope, ident->symbol);
    const ir_type_t *index_type = ir_operand_type(index);

    maybe_error_undeclared(analyzer, ident->symbol, ident->region);
    if (!ir_type_is_kind(lookup->type, IR_TYPE_ARRAY)) {
      const symbol_t *symbol = expr->expr.decl_ref.decl->symbol;
      msg_t          *msg    = new_msg(analyzer->source, ident->region,
                    MSG_ERROR, "`%.*s` is not an array", (int) symbol->len, symbol->ptr);
      msg_emit(msg);
      exit(EXIT_FAILURE);
    }
    if (!ir_type_is_kind(index_type, IR_TYPE_INTEGER)) {
      region_t region = expr->expr.array_subscript.subscript->region;
      msg_t   *msg    = new_msg(analyzer->source, region,
             MSG_ERROR, "arrays cannot be indexed by `%s`", ir_type_str(index_type));
      msg_add_inline_entry(msg, region, "array indices are of type integer");
      msg_emit(msg);
      exit(EXIT_FAILURE);
    }
    return new_ir_index_place(ir_local_for(analyzer->factory, lookup, ident->region.pos), index);
  }
  default:
    unreachable();
  }
}

void error_invalid_binary_expr(
  analyzer_t *analyzer, ast_expr_binary_t *expr,
  const ir_type_t *lhs_type, const ir_type_t *rhs_type, const char *expected)
{
  msg_t *msg;
  assert(analyzer && expr);
  msg = new_msg(analyzer->source, expr->op_region,
    MSG_ERROR, "invalid operands for `%s`", ast_binary_kind_expr_to_str(expr->kind));
  msg_add_inline_entry(msg, expr->lhs->region, "%s", ir_type_str(lhs_type));
  msg_add_inline_entry(msg, expr->op_region,
    "operator `%s` takes two operands of %s", ast_binary_kind_expr_to_str(expr->kind), expected);
  msg_add_inline_entry(msg, expr->rhs->region, "%s", ir_type_str(rhs_type));
  msg_emit(msg);
}

ir_operand_t *analyze_binary_expr(analyzer_t *analyzer, ir_block_t **block, ast_expr_binary_t *expr)
{
  assert(analyzer && expr);

  if (expr->lhs->kind == AST_EXPR_KIND_EMPTY) {
    ir_operand_t    *rhs   = analyze_expr(analyzer, block, expr->rhs);
    const ir_type_t *rtype = ir_operand_type(rhs);

    if (!ir_type_is_kind(rtype, IR_TYPE_INTEGER)) {
      msg_t *msg = new_msg(analyzer->source, expr->op_region,
        MSG_ERROR, "`%s` cannot be prefixed by `%s`", ir_type_str(rtype), ast_binary_kind_expr_to_str(expr->kind));
      msg_emit(msg);
      exit(EXIT_FAILURE);
    }

    switch (expr->kind) {
    case AST_EXPR_BINARY_KIND_PLUS:
      return rhs;
    case AST_EXPR_BINARY_KIND_MINUS: {
      ir_operand_t     *lhs    = new_ir_constant_operand(ir_number_constant(analyzer->factory, 0));
      const ir_local_t *result = ir_local_temp(analyzer->factory, ir_type_integer(analyzer->factory));
      ir_block_push_assign(*block, new_ir_place(result), new_ir_binary_op_rvalue(expr->kind, lhs, rhs));
      return new_ir_place_operand(new_ir_place(result));
    }
    default:
      unreachable();
    }
  } else {
    ir_operand_t    *lhs   = analyze_expr(analyzer, block, expr->lhs);
    const ir_type_t *ltype = ir_operand_type(lhs);

    switch (expr->kind) {
    case AST_EXPR_BINARY_KIND_EQUAL:
    case AST_EXPR_BINARY_KIND_NOTEQ:
    case AST_EXPR_BINARY_KIND_LE:
    case AST_EXPR_BINARY_KIND_LEEQ:
    case AST_EXPR_BINARY_KIND_GR:
    case AST_EXPR_BINARY_KIND_GREQ: {
      ir_operand_t     *rhs    = analyze_expr(analyzer, block, expr->rhs);
      const ir_type_t  *rtype  = ir_operand_type(rhs);
      const ir_local_t *result = ir_local_temp(analyzer->factory, ir_type_boolean(analyzer->factory));
      ir_block_push_assign(*block, new_ir_place(result), new_ir_binary_op_rvalue(expr->kind, lhs, rhs));
      if (ltype != rtype || !ir_type_is_std(ltype) || !ir_type_is_std(rtype)) {
        error_invalid_binary_expr(analyzer, expr, ltype, rtype, "the same standard type");
        exit(EXIT_FAILURE);
      }
      return new_ir_place_operand(new_ir_place(result));
    }
    case AST_EXPR_BINARY_KIND_PLUS:
    case AST_EXPR_BINARY_KIND_MINUS:
    case AST_EXPR_BINARY_KIND_STAR:
    case AST_EXPR_BINARY_KIND_DIV: {
      ir_operand_t     *rhs    = analyze_expr(analyzer, block, expr->rhs);
      const ir_type_t  *rtype  = ir_operand_type(rhs);
      const ir_local_t *result = ir_local_temp(analyzer->factory, ir_type_integer(analyzer->factory));
      ir_block_push_assign(*block, new_ir_place(result), new_ir_binary_op_rvalue(expr->kind, lhs, rhs));
      if (!ir_type_is_kind(ltype, IR_TYPE_INTEGER) || !ir_type_is_kind(rtype, IR_TYPE_INTEGER)) {
        error_invalid_binary_expr(analyzer, expr, ltype, rtype, "type integer");
        exit(EXIT_FAILURE);
      }
      return new_ir_place_operand(new_ir_place(result));
    }
    case AST_EXPR_BINARY_KIND_OR:
    case AST_EXPR_BINARY_KIND_AND: {
      ir_block_t       *shortcircuit = ir_block(analyzer->factory);
      ir_block_t       *els_begin    = ir_block(analyzer->factory);
      ir_block_t       *els_end      = els_begin;
      ir_operand_t     *rhs          = analyze_expr(analyzer, &els_end, expr->rhs);
      const ir_type_t  *rtype        = ir_operand_type(rhs);
      const ir_local_t *result       = ir_local_temp(analyzer->factory, ir_type_boolean(analyzer->factory));
      if (!ir_type_is_kind(ltype, IR_TYPE_BOOLEAN) || !ir_type_is_kind(rtype, IR_TYPE_BOOLEAN)) {
        error_invalid_binary_expr(analyzer, expr, ltype, rtype, "type boolean");
        exit(EXIT_FAILURE);
      }
      switch (expr->kind) {
      case AST_EXPR_BINARY_KIND_OR:
        ir_block_terminate_if(*block, lhs, shortcircuit, els_begin);
        lhs = new_ir_constant_operand(ir_boolean_constant(analyzer->factory, 1));
        break;
      case AST_EXPR_BINARY_KIND_AND:
        ir_block_terminate_if(*block, lhs, els_begin, shortcircuit);
        lhs = new_ir_constant_operand(ir_boolean_constant(analyzer->factory, 0));
        break;
      default:
        unreachable();
        break;
      }
      *block = ir_block(analyzer->factory);
      ir_block_push_assign(shortcircuit, new_ir_place(result), new_ir_use_rvalue(lhs));
      ir_block_terminate_goto(shortcircuit, *block);
      ir_block_push_assign(els_end, new_ir_place(result), new_ir_use_rvalue(rhs));
      ir_block_terminate_goto(els_end, *block);
      return new_ir_place_operand(new_ir_place(result));
    }
    default:
      unreachable();
    }
  }
}

ir_operand_t *analyze_not_expr(analyzer_t *analyzer, ir_block_t **block, ast_expr_not_t *expr)
{
  assert(analyzer && expr);

  {
    ir_operand_t     *operand = analyze_expr(analyzer, block, expr->expr);
    const ir_type_t  *type    = ir_operand_type(operand);
    const ir_local_t *result  = ir_local_temp(analyzer->factory, ir_type_boolean(analyzer->factory));
    ir_block_push_assign(*block, new_ir_place(result), new_ir_not_rvalue(operand));
    if (!ir_type_is_kind(type, IR_TYPE_BOOLEAN)) {
      msg_t *msg = new_msg(analyzer->source, expr->op_region,
        MSG_ERROR, "invalid operands for `not`");
      msg_add_inline_entry(msg, expr->op_region, "operator `not` takes one operand of type boolean");
      msg_add_inline_entry(msg, expr->expr->region, "%s", ir_type_str(type));
      msg_emit(msg);
      exit(EXIT_FAILURE);
    }
    return new_ir_place_operand(new_ir_place(result));
  }
}

ir_operand_t *analyze_cast_expr(analyzer_t *analyzer, ir_block_t **block, ast_expr_cast_t *expr)
{
  assert(analyzer && block && expr);

  {
    ir_operand_t     *operand      = analyze_expr(analyzer, block, expr->cast);
    const ir_type_t  *operand_type = ir_operand_type(operand);
    const ir_type_t  *cast_type    = analyze_type(analyzer, expr->type);
    const ir_local_t *result       = ir_local_temp(analyzer->factory, cast_type);
    ir_block_push_assign(*block, new_ir_place(result), new_ir_cast_rvalue(cast_type, operand));
    if (!ir_type_is_std(operand_type)) {
      msg_t *msg = new_msg(analyzer->source, expr->cast->region,
        MSG_ERROR, "expression of type `%s` cannot be cast", ir_type_str(operand_type));
      msg_add_inline_entry(msg, expr->cast->region, "expressions to be cast are of standard types");
      msg_emit(msg);
      exit(EXIT_FAILURE);
    }
    if (!ir_type_is_std(cast_type)) {
      msg_t *msg = new_msg(analyzer->source, expr->cast->region,
        MSG_ERROR, "expression cannot be cast to `%s`", ir_type_str(cast_type));
      msg_add_inline_entry(msg, expr->type->region, "expressions can be cast to standard types");
      msg_emit(msg);
      exit(EXIT_FAILURE);
    }
    return new_ir_place_operand(new_ir_place(result));
  }
}

ir_operand_t *analyze_constant_expr(analyzer_t *analyzer, ir_block_t **block, ast_expr_constant_t *expr)
{
  assert(analyzer && block && expr);

  switch (expr->lit->kind) {
  case AST_LIT_KIND_NUMBER:
    return new_ir_constant_operand(ir_number_constant(analyzer->factory, expr->lit->lit.number.value));
  case AST_LIT_KIND_BOOLEAN:
    return new_ir_constant_operand(ir_boolean_constant(analyzer->factory, expr->lit->lit.boolean.value));
  case AST_LIT_KIND_STRING: {
    const symbol_t *symbol = expr->lit->lit.string.symbol;
    if (expr->lit->lit.string.str_len != 1) {
      msg_t *msg = new_msg(analyzer->source, expr->lit->region,
        MSG_ERROR, "string is not a valid expression");
      msg_emit(msg);
      exit(EXIT_FAILURE);
    }
    return new_ir_constant_operand(ir_char_constant(analyzer->factory, symbol->ptr[0]));
  }
  }

  unreachable();
}

ir_operand_t *analyze_expr(analyzer_t *analyzer, ir_block_t **block, ast_expr_t *expr)
{
  assert(analyzer && block && expr);

  switch (expr->kind) {
  case AST_EXPR_KIND_DECL_REF:
  case AST_EXPR_KIND_ARRAY_SUBSCRIPT: {
    ir_place_t *place = analyze_lvalue(analyzer, block, expr);
    return new_ir_place_operand(place);
  }
  case AST_EXPR_KIND_BINARY:
    return analyze_binary_expr(analyzer, block, &expr->expr.binary);
  case AST_EXPR_KIND_NOT:
    return analyze_not_expr(analyzer, block, &expr->expr.not );
  case AST_EXPR_KIND_PAREN:
    return analyze_expr(analyzer, block, expr->expr.paren.inner);
  case AST_EXPR_KIND_CAST:
    return analyze_cast_expr(analyzer, block, &expr->expr.cast);
  case AST_EXPR_KIND_CONSTANT:
    return analyze_constant_expr(analyzer, block, &expr->expr.constant);
  default:
    unreachable();
  }
}

ir_operand_t *analyze_stmt_call_param(analyzer_t *analyzer, ir_block_t **block, ast_expr_t *args, ir_type_t *param_type)
{
  assert(analyzer && block);

  if (args && param_type) {
    ir_block_t      *blk  = *block;
    ir_operand_t    *next = analyze_stmt_call_param(analyzer, &blk, args->next, param_type->next);
    ir_operand_t    *ret  = analyze_expr(analyzer, &blk, args);
    const ir_type_t *type = ir_operand_type(ret);
    ret->next             = next;
    if (param_type->u.ref != type) {
      msg_t *msg = new_msg(analyzer->source, args->region, MSG_ERROR, "mismatching argument type");
      char   expected[1024], found[1024];
      strcpy(expected, ir_type_str(param_type->u.ref));
      strcpy(found, ir_type_str(type));
      msg_add_inline_entry(msg, args->region, "expected `%s`, found `%s`", expected, found);
      msg_emit(msg);
      exit(EXIT_FAILURE);
    }
    *block = ir_block(analyzer->factory);
    ir_block_terminate_arg(blk, ret, *block);
    return ret;
  } else {
    return NULL;
  }
}

void analyze_stmt(analyzer_t *analyzer, ir_block_t **block, ast_stmt_t *stmt)
{
  assert(analyzer && block && stmt);

  while (stmt) {
    switch (stmt->kind) {
    case AST_STMT_KIND_ASSIGN: {
      ast_stmt_assign_t *stmt_assign = (ast_stmt_assign_t *) stmt;
      ir_place_t        *lhs         = analyze_lvalue(analyzer, block, stmt_assign->lhs);
      ir_operand_t      *rhs         = analyze_expr(analyzer, block, stmt_assign->rhs);
      const ir_type_t   *ltype       = ir_place_type(lhs);
      const ir_type_t   *rtype       = ir_operand_type(rhs);

      if (ltype != rtype || !ir_type_is_std(ltype) || !ir_type_is_std(rtype)) {
        msg_t *msg = new_msg(analyzer->source, stmt_assign->op_region,
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
      ir_operand_t    *cond    = analyze_expr(analyzer, block, stmt_if->cond);
      const ir_type_t *type    = ir_operand_type(cond);

      if (!ir_type_is_kind(type, IR_TYPE_BOOLEAN)) {
        msg_t *msg = new_msg(analyzer->source, stmt_if->cond->region,
          MSG_ERROR, "expression of type `%s` cannot be condition", ir_type_str(type));
        msg_add_inline_entry(msg, stmt_if->cond->region, "condition expressions are of type boolean");
        msg_emit(msg);
        exit(EXIT_FAILURE);
      }

      {
        ir_block_t *then_begin = ir_block(analyzer->factory);
        ir_block_t *then_end   = then_begin;
        ir_block_t *join_block = ir_block(analyzer->factory);
        analyze_stmt(analyzer, &then_end, stmt_if->then_stmt);

        if (stmt_if->else_stmt) {
          ir_block_t *else_begin = ir_block(analyzer->factory);
          ir_block_t *else_end   = else_begin;
          analyze_stmt(analyzer, &else_end, stmt_if->else_stmt);
          ir_block_terminate_if(*block, cond, then_begin, else_begin);
          ir_block_terminate_goto(then_end, join_block);
          ir_block_terminate_goto(else_end, join_block);
        } else {
          ir_block_terminate_if(*block, cond, then_begin, join_block);
          ir_block_terminate_goto(then_end, join_block);
        }
        *block = join_block;
      }
      break;
    }
    case AST_STMT_KIND_WHILE: {
      ir_block_t       *cond_begin = ir_block(analyzer->factory);
      ir_block_t       *cond_end   = cond_begin;
      ir_block_t       *join_block = ir_block(analyzer->factory);
      ast_stmt_while_t *stmt_while = (ast_stmt_while_t *) stmt;
      ir_operand_t     *cond       = analyze_expr(analyzer, &cond_end, stmt_while->cond);
      const ir_type_t  *type       = ir_operand_type(cond);

      if (!ir_type_is_kind(type, IR_TYPE_BOOLEAN)) {
        msg_t *msg = new_msg(analyzer->source, stmt_while->cond->region,
          MSG_ERROR, "expression of type `%s` cannot be condition", ir_type_str(type));
        msg_add_inline_entry(msg, stmt_while->cond->region, "condition expressions are of type boolean");
        msg_emit(msg);
        exit(EXIT_FAILURE);
      }

      {
        ir_block_t *pre_break_dest = analyzer->break_dest;
        analyzer->break_dest       = join_block;
        {
          ir_block_t *do_begin = ir_block(analyzer->factory);
          ir_block_t *do_end   = do_begin;
          analyze_stmt(analyzer, &do_end, stmt_while->do_stmt);
          ir_block_terminate_goto(*block, cond_begin);
          ir_block_terminate_if(cond_end, cond, do_begin, join_block);
          ir_block_terminate_goto(do_end, cond_begin);
        }
        analyzer->break_dest = pre_break_dest;
        *block               = join_block;
      }
      break;
    }
    case AST_STMT_KIND_BREAK: {
      ir_block_terminate_goto(*block, analyzer->break_dest);
      *block = ir_block(analyzer->factory);
      break;
    }
    case AST_STMT_KIND_CALL: {
      ast_stmt_call_t *stmt_call = (ast_stmt_call_t *) stmt;
      ast_ident_t     *ident     = stmt_call->name;
      ir_item_t       *item      = ir_item_lookup(analyzer->factory->scope, ident->symbol);

      maybe_error_undeclared(analyzer, ident->symbol, ident->region);
      if (item->kind != IR_ITEM_PROCEDURE) {
        const symbol_t *symbol = stmt_call->name->symbol;
        msg_t          *msg    = new_msg(analyzer->source, ident->region,
                      MSG_ERROR, "`%.*s` is not a procedure", (int) symbol->len, symbol->ptr);
        msg_emit(msg);
        exit(EXIT_FAILURE);
      }

      {
        ir_scope_t *scope = analyzer->factory->scope;
        while (scope) {
          if (scope->owner->symbol == item->symbol && scope->owner->kind == IR_ITEM_PROCEDURE) {
            msg_t *msg = new_msg(analyzer->source, ident->region,
              MSG_ERROR, "recursive call of procedure is not allowed");
            msg_emit(msg);
            exit(EXIT_FAILURE);
          }
          scope = scope->next;
        }
      }

      {
        const ir_local_t *func       = ir_local_for(analyzer->factory, item, ident->region.pos);
        ir_type_t        *param_type = ir_local_type(func)->u.procedure_type.param_types;
        ast_expr_t       *expr_cur   = stmt_call->args;
        ir_type_t        *type_cur   = param_type;
        long              expr_cnt = 0, type_cnt = 0;
        ir_operand_t     *arg = analyze_stmt_call_param(analyzer, block, stmt_call->args, param_type);
        while (expr_cur) {
          expr_cnt++;
          expr_cur = expr_cur->next;
        }
        while (type_cur) {
          type_cnt++;
          type_cur = type_cur->next;
        }
        if (expr_cnt != type_cnt) {
          msg_t *msg = new_msg(analyzer->source, ident->region, MSG_ERROR, "wrong number of arguments");
          msg_add_inline_entry(msg, ident->region, "expected %ld arguments, supplied %ld arguments", type_cnt, expr_cnt);
          msg_emit(msg);
          exit(EXIT_FAILURE);
        }
        ir_block_push_call(*block, new_ir_place(func), arg);
      }
      break;
    }
    case AST_STMT_KIND_RETURN: {
      ir_block_terminate_return(*block);
      *block = ir_block(analyzer->factory);
      break;
    }
    case AST_STMT_KIND_READ: {
      ast_stmt_read_t *stmt_read = (ast_stmt_read_t *) stmt;
      ast_expr_t      *args      = stmt_read->args;

      while (args) {
        if (args->kind != AST_EXPR_KIND_DECL_REF && args->kind != AST_EXPR_KIND_ARRAY_SUBSCRIPT) {
          msg_t *msg = new_msg(analyzer->source, args->region,
            MSG_ERROR, "cannot read value for expression");
          msg_add_inline_entry(msg, args->region,
            "arguments for read statements are of reference to integer or char");
          msg_emit(msg);
          exit(EXIT_FAILURE);
        }

        {
          ir_place_t      *ref  = analyze_lvalue(analyzer, block, args);
          const ir_type_t *type = ir_place_type(ref);
          if (!ir_type_is_kind(type, IR_TYPE_INTEGER) && !ir_type_is_kind(type, IR_TYPE_CHAR)) {
            msg_t *msg = new_msg(analyzer->source, args->region,
              MSG_ERROR, "cannot read value for reference to `%s`", ir_type_str(type));
            msg_add_inline_entry(msg, args->region,
              "arguments for read statements are of reference to integer or char");
            msg_emit(msg);
            exit(EXIT_FAILURE);
          }

          ir_block_push_read(*block, ref);
        }
        args = args->next;
      }

      if (stmt_read->newline) {
        ir_block_push_readln(*block);
      }
      break;
    }
    case AST_STMT_KIND_WRITE: {
      ast_stmt_write_t *stmt_write = (ast_stmt_write_t *) stmt;
      ast_out_fmt_t    *formats    = stmt_write->formats;

      while (formats) {
        ast_expr_constant_t *constant = &formats->expr->expr.constant;
        ast_lit_string_t    *string   = &constant->lit->lit.string;
        if (formats->expr->kind == AST_EXPR_KIND_CONSTANT && constant->lit->kind == AST_LIT_KIND_STRING && string->str_len != 1) {
          const ir_constant_t *constant = ir_string_constant(analyzer->factory, string->symbol, string->str_len);
          ir_block_push_write(*block, new_ir_constant_operand(constant), NULL);
        } else {
          ir_operand_t    *value = analyze_expr(analyzer, block, formats->expr);
          const ir_type_t *type  = ir_operand_type(value);
          if (!ir_type_is_std(type)) {
            msg_t *msg = new_msg(analyzer->source, formats->expr->region,
              MSG_ERROR, "cannot write value of type `%s`", ir_type_str(type));
            msg_add_inline_entry(msg, formats->expr->region,
              "arguments for write statements are of standard types");
            msg_emit(msg);
            exit(EXIT_FAILURE);
          }

          if (formats->len) {
            ir_block_push_write(*block, value, ir_number_constant(analyzer->factory, formats->len->lit.number.value));
          } else {
            ir_block_push_write(*block, value, NULL);
          }
        }
        formats = formats->next;
      }

      if (stmt_write->newline) {
        ir_block_push_writeln(*block);
      }
      break;
    }
    case AST_STMT_KIND_COMPOUND: {
      ast_stmt_compound_t *stmt_compound = (ast_stmt_compound_t *) stmt;
      analyze_stmt(analyzer, block, stmt_compound->stmts);
      break;
    }
    case AST_STMT_KIND_EMPTY:
      break;
    }
    stmt = stmt->next;
  }
}

ir_type_t *analyze_param_types(analyzer_t *analyzer, ast_decl_param_t *decl)
{
  ir_type_t *ret, **last;
  assert(analyzer);

  ret  = NULL;
  last = &ret;
  while (decl) {
    ast_ident_t     *ident = decl->names;
    const ir_type_t *type  = analyze_type(analyzer, decl->type);
    if (!ir_type_is_std(type)) {
      msg_t *msg = new_msg(analyzer->source, decl->type->region,
        MSG_ERROR, "invalid parameter of type `%s`", ir_type_str(type));
      msg_add_inline_entry(msg, decl->type->region,
        "parameters are of standard types");
      msg_emit(msg);
      exit(EXIT_FAILURE);
    }
    while (ident) {
      *last = ir_type_ref(type);
      last  = &(*last)->next;
      ident = ident->next;
    }
    decl = decl->next;
  }
  return ret;
}

void analyze_variable_decl(analyzer_t *analyzer, ast_decl_variable_t *decl, int local)
{
  assert(analyzer && decl);

  while (decl) {
    ast_ident_t     *ident = decl->names;
    const ir_type_t *type  = analyze_type(analyzer, decl->type);
    while (ident) {
      ir_item_kind_t kind = local ? IR_ITEM_LOCAL_VAR : IR_ITEM_VAR;
      maybe_error_conflict(analyzer, ident->symbol, ident->region);
      ir_item(analyzer->factory, kind, ident->symbol, ident->region, type);
      ident = ident->next;
    }
    decl = decl->next;
  }
}

void analyze_param_decl(analyzer_t *analyzer, ast_decl_param_t *decl)
{
  assert(analyzer);

  while (decl) {
    ast_ident_t     *ident = decl->names;
    const ir_type_t *type  = analyze_type(analyzer, decl->type);
    if (!ir_type_is_std(type)) {
      msg_t *msg = new_msg(analyzer->source, decl->type->region,
        MSG_ERROR, "invalid parameter of type `%s`", ir_type_str(type));
      msg_add_inline_entry(msg, decl->type->region,
        "parameters are of standard types");
      msg_emit(msg);
      exit(EXIT_FAILURE);
    }
    while (ident) {
      maybe_error_conflict(analyzer, ident->symbol, ident->region);
      ir_item(analyzer->factory, IR_ITEM_ARG_VAR, ident->symbol, ident->region, type);
      ident = ident->next;
    }
    decl = decl->next;
  }
}

void analyze_decl_part(analyzer_t *analyzer, ast_decl_part_t *decl_part)
{
  assert(analyzer);

  while (decl_part) {
    switch (decl_part->kind) {
    case AST_DECL_PART_VARIABLE: {
      ast_decl_part_variable_t *decl = (ast_decl_part_variable_t *) decl_part;
      analyze_variable_decl(analyzer, decl->decls, 0);
      break;
    }
    case AST_DECL_PART_PROCEDURE: {
      ir_item_t                 *item;
      ir_block_t                *block_begin = ir_block(analyzer->factory);
      ast_decl_part_procedure_t *decl        = (ast_decl_part_procedure_t *) decl_part;
      ir_type_t                 *param_types = analyze_param_types(analyzer, decl->params);
      const ir_type_t           *proc_type   = ir_type_procedure(analyzer->factory, param_types);

      maybe_error_conflict(analyzer, decl->name->symbol, decl->name->region);
      item = ir_item(analyzer->factory, IR_ITEM_PROCEDURE, decl->name->symbol, decl->name->region, proc_type);
      ir_scope_start(analyzer->factory, item);
      {
        ast_decl_part_variable_t *decl_part = (ast_decl_part_variable_t *) decl->variables;
        ir_block_t               *block_end = block_begin;
        analyze_param_decl(analyzer, decl->params);
        if (decl_part) {
          analyze_variable_decl(analyzer, decl_part->decls, 1);
        }
        analyze_stmt(analyzer, &block_end, decl->stmt);
        ir_block_terminate_return(block_end);
      }
      ir_scope_end(analyzer->factory, block_begin);
      break;
    }
    }
    decl_part = decl_part->next;
  }
}

ir_item_t *analyze_program(analyzer_t *analyzer, ast_program_t *program)
{
  ir_item_t  *ret;
  ir_block_t *block_begin;
  assert(analyzer && program);

  ret         = ir_item(analyzer->factory, IR_ITEM_PROGRAM, program->name->symbol, program->name->region, ir_type_program(analyzer->factory));
  block_begin = ir_block(analyzer->factory);
  ir_scope_start(analyzer->factory, ret);
  {
    ir_block_t *block_end = block_begin;
    analyze_decl_part(analyzer, program->decl_part);
    analyze_stmt(analyzer, &block_end, program->stmt);
    ir_block_terminate_return(block_end);
  }
  ir_scope_end(analyzer->factory, block_begin);
  return ret;
}

ir_t *analyze_ast(ast_t *ast)
{
  ir_item_t *items;
  analyzer_t analyzer;
  assert(ast);

  analyzer.source  = ast->source;
  analyzer.factory = new_ir_factory();
  items            = analyze_program(&analyzer, ast->program);
  return new_ir(analyzer.source, items, analyzer.factory);
}
