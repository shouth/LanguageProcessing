#include <assert.h>

#include "mppl.h"

typedef struct {
    const source_t *source;
    ir_factory_t *factory;
    ir_block_t *break_dest;
} analyzer_t;

void maybe_error_conflict(analyzer_t *analyzer, symbol_t symbol, region_t region)
{
    ir_item_t *item;
    if (item = ir_item_lookup(analyzer->factory, symbol)) {
        msg_t *msg = new_msg(analyzer->source, region, MSG_ERROR, "conflicting names");
        msg_add_inline_entry(msg, item->name_region, "first used here");
        msg_add_inline_entry(msg, region, "second used here");
        msg_emit(msg);
        exit(1);
    }
}

void maybe_error_undeclared(analyzer_t *analyzer, symbol_t symbol, region_t region)
{
    if (!ir_item_lookup(analyzer->factory, symbol)) {
        const symbol_instance_t *instance = symbol_get_instance(symbol);
        msg_t *msg = new_msg(analyzer->source, region,
            MSG_ERROR, "`%.*s` is not declared", (int) instance->len, instance->ptr);
        msg_emit(msg);
        exit(1);
    }
}

const ir_type_t *analyze_type(analyzer_t *analyzer, ast_type_t *type)
{
    assert(analyzer && type);

    switch (type->kind) {
    case AST_TYPE_BOOLEAN:
        return ir_type_boolean(analyzer->factory);
    case AST_TYPE_CHAR:
        return ir_type_char(analyzer->factory);
    case AST_TYPE_INTEGER:
        return ir_type_integer(analyzer->factory);
    case AST_TYPE_ARRAY: {
        const ir_type_t *base_type = analyze_type(analyzer, type->u.array_type.base);
        ir_type_t *base_type_ref = ir_type_ref(base_type);
        size_t size = type->u.array_type.size->u.number_lit.value;
        if (size == 0) {
            msg_t *msg = new_msg(analyzer->source, type->u.array_type.size->region,
                MSG_ERROR, "size of array needs to be greater than 0");
            msg_emit(msg);
            exit(1);
        }
        return ir_type_array(analyzer->factory, base_type_ref, size);
    }
    }

    unreachable();
}

ir_operand_t *analyze_expr(analyzer_t *analyzer, ir_block_t *block, ast_expr_t *expr);

ir_place_t *analyze_lvalue(analyzer_t *analyzer, ir_block_t *block, ast_expr_t *expr)
{
    assert(analyzer && expr);
    assert(expr->kind == AST_EXPR_DECL_REF || expr->kind == AST_EXPR_ARRAY_SUBSCRIPT);

    switch (expr->kind) {
    case AST_EXPR_DECL_REF: {
        ast_ident_t *ident = expr->u.decl_ref_expr.decl;
        ir_item_t *lookup = ir_item_lookup(analyzer->factory, ident->symbol);
        maybe_error_undeclared(analyzer, ident->symbol, ident->region);
        return new_ir_place(ir_local_for(analyzer->factory, lookup, ident->region.pos));
    }
    case AST_EXPR_ARRAY_SUBSCRIPT: {
        ir_operand_t *index = analyze_expr(analyzer, block, expr->u.array_subscript_expr.expr);
        ast_ident_t *ident = expr->u.array_subscript_expr.decl;
        ir_item_t *lookup = ir_item_lookup(analyzer->factory, ident->symbol);
        const ir_type_t *operand_type;

        maybe_error_undeclared(analyzer, ident->symbol, ident->region);
        if (!ir_type_is_kind(lookup->type, IR_TYPE_ARRAY)) {
            const symbol_instance_t *instance = symbol_get_instance(expr->u.decl_ref_expr.decl->symbol);
            msg_t *msg = new_msg(analyzer->source, ident->region,
                MSG_ERROR, "`%.*s` is not an array.", (int) instance->len, instance->ptr);
            msg_emit(msg);
            exit(1);
        }
        operand_type = ir_operand_type(index);
        if (!ir_type_is_kind(operand_type, IR_TYPE_INTEGER)) {
            region_t region = expr->u.array_subscript_expr.expr->region;
            msg_t *msg = new_msg(analyzer->source, region,
                MSG_ERROR, "arrays cannot be indexed by `%s`", ir_type_str(operand_type));
            msg_add_inline_entry(msg, region, "array indices are of type integer");
            msg_emit(msg);
            exit(1);
        }
        return new_ir_index_place(ir_local_for(analyzer->factory, lookup, ident->region.pos), index);
    }
    }
}

void error_invalid_binary_expr(
    analyzer_t *analyzer, ast_binary_expr_t *expr,
    const ir_type_t *lhs_type, const ir_type_t *rhs_type, const char *expected)
{
    msg_t *msg;
    assert(analyzer && expr);
    msg = new_msg(analyzer->source, expr->op_region,
        MSG_ERROR, "invalid operands for `%s`", ast_binop_str(expr->kind));
    msg_add_inline_entry(msg, expr->lhs->region, "%s", ir_type_str(lhs_type));
    msg_add_inline_entry(msg, expr->op_region,
        "operator `%s` takes two operands of %s", ast_binop_str(expr->kind), expected);
    msg_add_inline_entry(msg, expr->rhs->region, "%s", ir_type_str(rhs_type));
    msg_emit(msg);
}

ir_operand_t *analyze_binary_expr(analyzer_t *analyzer, ir_block_t *block, ast_binary_expr_t *expr)
{
    ir_operand_t *lhs, *rhs;
    const ir_type_t *ltype, *rtype;
    const ir_local_t *result;
    const ir_type_t *type;
    assert(analyzer && expr);

    rhs = analyze_expr(analyzer, block, expr->rhs);
    rtype = ir_operand_type(rhs);

    if (expr->lhs->kind == AST_EXPR_EMPTY) {
        if (!ir_type_is_kind(rtype, IR_TYPE_INTEGER)) {
            msg_t *msg = new_msg(analyzer->source, expr->op_region,
                MSG_ERROR, "`%s` cannot be prefixed by `%s`", ir_type_str(rtype), ast_binop_str(expr->kind));
            msg_emit(msg);
            exit(1);
        }

        type = ir_type_integer(analyzer->factory);
        lhs = new_ir_constant_operand(ir_number_constant(analyzer->factory, 0));
    } else {
        lhs = analyze_expr(analyzer, block, expr->lhs);
        ltype = ir_operand_type(lhs);

        switch (expr->kind) {
        case AST_BINARY_OP_EQUAL:
        case AST_BINARY_OP_NOTEQ:
        case AST_BINARY_OP_LE:
        case AST_BINARY_OP_LEEQ:
        case AST_BINARY_OP_GR:
        case AST_BINARY_OP_GREQ:
            if (ltype != rtype || !ir_type_is_std(ltype) || !ir_type_is_std(rtype)) {
                error_invalid_binary_expr(analyzer, expr, ltype, rtype, "the same standard type");
                exit(1);
            }
            type = ir_type_boolean(analyzer->factory);
            break;
        case AST_BINARY_OP_PLUS:
        case AST_BINARY_OP_MINUS:
        case AST_BINARY_OP_STAR:
        case AST_BINARY_OP_DIV:
            if (!ir_type_is_kind(ltype, IR_TYPE_INTEGER) || !ir_type_is_kind(rtype, IR_TYPE_INTEGER)) {
                error_invalid_binary_expr(analyzer, expr, ltype, rtype, "type integer");
                exit(1);
            }
            type = ir_type_integer(analyzer->factory);
            break;
        case AST_BINARY_OP_OR:
        case AST_BINARY_OP_AND:
            if (!ir_type_is_kind(ltype, IR_TYPE_BOOLEAN) || !ir_type_is_kind(rtype, IR_TYPE_BOOLEAN)) {
                error_invalid_binary_expr(analyzer, expr, ltype, rtype, "type boolean");
                exit(1);
            }
            type = ir_type_boolean(analyzer->factory);
            break;
        }
    }

    result = ir_local_temp(analyzer->factory, type);
    ir_block_push_assign(block, new_ir_place(result), new_ir_binary_op_rvalue(expr->kind, lhs, rhs));
    return new_ir_place_operand(new_ir_place(result));
}

ir_operand_t *analyze_unary_expr(analyzer_t *analyzer, ir_block_t *block, ast_unary_expr_t *expr)
{
    ir_operand_t *operand;
    const ir_type_t *operand_type;
    const ir_local_t *result;
    const ir_type_t *type;
    assert(analyzer && expr);

    operand = analyze_expr(analyzer, block, expr->expr);
    operand_type = ir_operand_type(operand);

    switch (expr->kind) {
    case AST_UNARY_OP_NOT:
        if (!ir_type_is_kind(operand_type, IR_TYPE_BOOLEAN)) {
            msg_t *msg = new_msg(analyzer->source, expr->op_region,
                MSG_ERROR, "invalid operands for `not`");
            msg_add_inline_entry(msg, expr->op_region, "operator `not` takes one operand of type boolean");
            msg_add_inline_entry(msg, expr->expr->region, "%s", ir_type_str(operand_type));
            msg_emit(msg);
            exit(1);
        }
        type = ir_type_boolean(analyzer->factory);
        break;
    }

    result = ir_local_temp(analyzer->factory, type);
    ir_block_push_assign(block, new_ir_place(result), new_ir_unary_op_rvalue(expr->kind, operand));
    return new_ir_place_operand(new_ir_place(result));
}

ir_operand_t *analyze_cast_expr(analyzer_t *analyzer, ir_block_t *block, ast_cast_expr_t *expr)
{
    ir_operand_t *operand;
    const ir_type_t *operand_type;
    const ir_type_t *cast_type;
    const ir_local_t *result;

    operand = analyze_expr(analyzer, block, expr->expr);
    operand_type = ir_operand_type(operand);
    cast_type = analyze_type(analyzer, expr->type);

    if (!ir_type_is_std(operand_type)) {
        msg_t *msg = new_msg(analyzer->source, expr->expr->region,
            MSG_ERROR, "expression of type `%s` cannot be cast", ir_type_str(operand_type));
        msg_add_inline_entry(msg, expr->expr->region, "expressions to be cast are of standard types");
        msg_emit(msg);
        exit(1);
    }
    if (!ir_type_is_std(cast_type)) {
        msg_t *msg = new_msg(analyzer->source, expr->expr->region,
            MSG_ERROR, "expression cannot be cast to `%s`", ir_type_str(cast_type));
        msg_add_inline_entry(msg, expr->type->region, "expressions can be cast to standard types");
        msg_emit(msg);
        exit(1);
    }

    result = ir_local_temp(analyzer->factory, cast_type);
    ir_block_push_assign(block, new_ir_place(result), new_ir_cast_rvalue(cast_type, operand));
    return new_ir_place_operand(new_ir_place(result));
}

ir_operand_t *analyze_constant_expr(analyzer_t *analyzer, ir_block_t *block, ast_constant_expr_t *expr)
{
    assert(analyzer && expr);

    switch (expr->lit->kind) {
    case AST_LIT_NUMBER: {
        const ir_constant_t *constant = ir_number_constant(analyzer->factory, expr->lit->u.number_lit.value);
        return new_ir_constant_operand(constant);
    }
    case AST_LIT_BOOLEAN: {
        const ir_constant_t *constant = ir_boolean_constant(analyzer->factory, expr->lit->u.boolean_lit.value);
        return new_ir_constant_operand(constant);
    }
    case AST_LIT_STRING: {
        const symbol_instance_t *instance;
        const ir_constant_t *constant;
        if (expr->lit->u.string_lit.str_len != 1) {
            msg_t *msg = new_msg(analyzer->source, expr->lit->region,
                MSG_ERROR, "string is not a valid expression");
            msg_emit(msg);
            exit(1);
        }
        instance = symbol_get_instance(expr->lit->u.string_lit.symbol);
        constant = ir_char_constant(analyzer->factory, instance->ptr[0]);
        return new_ir_constant_operand(constant);
    }
    }

    unreachable();
}

ir_operand_t *analyze_expr(analyzer_t *analyzer, ir_block_t *block, ast_expr_t *expr)
{
    assert(analyzer && expr);

    switch (expr->kind) {
    case AST_EXPR_DECL_REF:
    case AST_EXPR_ARRAY_SUBSCRIPT: {
        ir_place_t *place = analyze_lvalue(analyzer, block, expr);
        return new_ir_place_operand(place);
    }
    case AST_EXPR_BINARY_OP:
        return analyze_binary_expr(analyzer, block, &expr->u.binary_expr);
    case AST_EXPR_UNARY_OP:
        return analyze_unary_expr(analyzer, block, &expr->u.unary_expr);
    case AST_EXPR_PAREN:
        return analyze_expr(analyzer, block, expr->u.paren_expr.expr);
    case AST_EXPR_CAST:
        return analyze_cast_expr(analyzer, block, &expr->u.cast_expr);
    case AST_EXPR_CONSTANT:
        return analyze_constant_expr(analyzer, block, &expr->u.constant_expr);
    }

    unreachable();
}

ir_block_t *analyze_stmt(analyzer_t *analyzer, ir_block_t *block, ast_stmt_t *stmt)
{
    assert(analyzer && stmt);

    while (stmt) {
        switch (stmt->kind) {
        case AST_STMT_ASSIGN: {
            ast_assign_stmt_t *assign_stmt = &stmt->u.assign_stmt;
            ir_place_t *lhs = analyze_lvalue(analyzer, block, assign_stmt->lhs);
            ir_operand_t *rhs_operand = analyze_expr(analyzer, block, assign_stmt->rhs);
            ir_rvalue_t *rhs = new_ir_use_rvalue(rhs_operand);
            const ir_type_t *ltype = ir_place_type(lhs);
            const ir_type_t *rtype = ir_operand_type(rhs_operand);

            if (ltype != rtype || !ir_type_is_std(ltype) || !ir_type_is_std(rtype)) {
                msg_t *msg = new_msg(analyzer->source, stmt->u.assign_stmt.op_region,
                    MSG_ERROR, "invalid operands for `:=`");
                msg_add_inline_entry(msg, assign_stmt->lhs->region, "%s", ir_type_str(ltype));
                msg_add_inline_entry(msg, assign_stmt->op_region,
                    "operator `:=` takes two operands of the same standard type");
                msg_add_inline_entry(msg, assign_stmt->rhs->region, "%s", ir_type_str(rtype));
                msg_emit(msg);
                exit(1);
            }

            ir_block_push_assign(block, lhs, rhs);
            break;
        }
        case AST_STMT_IF: {
            ast_if_stmt_t *if_stmt = &stmt->u.if_stmt;
            ir_operand_t *cond = analyze_expr(analyzer, block, stmt->u.if_stmt.cond);
            const ir_type_t *type = ir_operand_type(cond);

            if (!ir_type_is_kind(type, IR_TYPE_BOOLEAN)) {
                msg_t *msg = new_msg(analyzer->source, if_stmt->cond->region,
                    MSG_ERROR, "expression of type `%s` cannot be condition", ir_type_str(type));
                msg_add_inline_entry(msg, if_stmt->cond->region, "condition expressions are of type boolean");
                msg_emit(msg);
                exit(1);
            }

            {
                ir_block_t *then_begin = ir_block(analyzer->factory);
                ir_block_t *then_end = analyze_stmt(analyzer, then_begin, stmt->u.if_stmt.then_stmt);
                ir_block_t *join_block = ir_block(analyzer->factory);

                if (stmt->u.if_stmt.else_stmt) {
                    ir_block_t *else_begin = ir_block(analyzer->factory);
                    ir_block_t *else_end = analyze_stmt(analyzer, else_begin, stmt->u.if_stmt.else_stmt);
                    ir_block_terminate_if(block, cond, then_begin, else_begin);
                    ir_block_terminate_goto(then_end, join_block);
                    ir_block_terminate_goto(else_end, join_block);
                } else {
                    ir_block_terminate_if(block, cond, then_begin, join_block);
                    ir_block_terminate_goto(then_end, join_block);
                }

                block = join_block;
            }
            break;
        }
        case AST_STMT_WHILE: {
            ir_block_t *cond_block = ir_block(analyzer->factory);
            ir_block_t *join_block = ir_block(analyzer->factory);
            ast_while_stmt_t *while_stmt = &stmt->u.while_stmt;
            ir_operand_t *cond = analyze_expr(analyzer, cond_block, while_stmt->cond);
            const ir_type_t *type = ir_operand_type(cond);

            if (!ir_type_is_kind(type, IR_TYPE_BOOLEAN)) {
                msg_t *msg = new_msg(analyzer->source, while_stmt->cond->region,
                    MSG_ERROR, "expression of type `%s` cannot be condition", ir_type_str(type));
                msg_add_inline_entry(msg, while_stmt->cond->region, "condition expressions are of type boolean");
                msg_emit(msg);
                exit(1);
            }

            {
                ir_block_t *pre_break_dest = analyzer->break_dest;
                analyzer->break_dest = join_block;
                {
                    ir_block_t *do_begin = ir_block(analyzer->factory);
                    ir_block_t *do_end = analyze_stmt(analyzer, do_begin, while_stmt->do_stmt);
                    ir_block_terminate_goto(block, cond_block);
                    ir_block_terminate_if(cond_block, cond, do_begin, join_block);
                    ir_block_terminate_goto(do_end, cond_block);
                }
                analyzer->break_dest = pre_break_dest;
                block = join_block;
            }
            break;
        }
        case AST_STMT_BREAK: {
            ir_block_terminate_goto(block, analyzer->break_dest);
            block = ir_block(analyzer->factory);
            break;
        }
        case AST_STMT_CALL: {
            ast_ident_t *ident = stmt->u.call_stmt.name;
            ir_item_t *item = ir_item_lookup(analyzer->factory, ident->symbol);

            maybe_error_undeclared(analyzer, ident->symbol, ident->region);
            if (item->kind != IR_ITEM_PROCEDURE) {
                const symbol_instance_t *instance = symbol_get_instance(stmt->u.call_stmt.name->symbol);
                msg_t *msg = new_msg(analyzer->source, ident->region,
                    MSG_ERROR, "`%.*s` is not a procedure", (int) instance->len, instance->ptr);
                msg_emit(msg);
                exit(1);
            }

            {
                ir_scope_t *scope = analyzer->factory->scope;
                while (scope) {
                    if (scope->owner->symbol == item->symbol && scope->owner->kind == IR_ITEM_PROCEDURE) {
                        msg_t *msg = new_msg(analyzer->source, ident->region,
                            MSG_ERROR, "recursive call of procedure is not allowed");
                        msg_emit(msg);
                        exit(1);
                    }
                    scope = scope->next;
                }
            }

            {
                const ir_local_t *func = ir_local_for(analyzer->factory, item, ident->region.pos);
                ast_expr_t *args = stmt->u.call_stmt.args;
                ir_type_t *type = NULL, **type_back = &type;
                ir_operand_t *arg = NULL, **arg_back = &arg;
                while (args) {
                    *arg_back = analyze_expr(analyzer, block, args);
                    *type_back = ir_type_ref(ir_operand_type(*arg_back));
                    arg_back = &(*arg_back)->next;
                    type_back = &(*type_back)->next;
                    args = args->next;
                }

                if (ir_local_type(func) != ir_type_procedure(analyzer->factory, type)) {
                    const symbol_instance_t *instance = symbol_get_instance(item->symbol);
                    msg_t *msg = new_msg(analyzer->source, ident->region,
                        MSG_ERROR, "mismatching arguments for procedure `%.*s`", (int) instance->len, instance->ptr);
                    msg_emit(msg);
                    exit(1);
                }

                ir_block_push_call(block, new_ir_place(func), arg);
            }
            break;
        }
        case AST_STMT_RETURN: {
            ir_block_terminate_return(block);
            block = ir_block(analyzer->factory);
            break;
        }
        case AST_STMT_READ: {
            ast_expr_t *args = stmt->u.read_stmt.args;

            while (args) {
                if (args->kind != AST_EXPR_DECL_REF && args->kind != AST_EXPR_ARRAY_SUBSCRIPT) {
                    msg_t *msg = new_msg(analyzer->source, args->region,
                        MSG_ERROR, "cannot read value for expression");
                    msg_add_inline_entry(msg, args->region,
                        "arguments for read statements are of reference and of type integer or char");
                    msg_emit(msg);
                    exit(1);
                }

                {
                    ir_place_t *ref = analyze_lvalue(analyzer, block, args);
                    const ir_type_t *type = ir_place_type(ref);
                    if (!ir_type_is_kind(type, IR_TYPE_INTEGER) && !ir_type_is_kind(type, IR_TYPE_CHAR)) {
                        msg_t *msg = new_msg(analyzer->source, args->region,
                            MSG_ERROR, "cannot read value for reference of type `%s`", ir_type_str(type));
                        msg_add_inline_entry(msg, args->region,
                            "arguments for read statements are of reference and of type integer or char");
                        msg_emit(msg);
                        exit(1);
                    }

                    ir_block_push_read(block, ref);
                }
                args = args->next;
            }
            break;
        }
        case AST_STMT_WRITE: {
            ast_output_format_t *formats = stmt->u.write_stmt.formats;

            while (formats) {
                ast_expr_t *expr = formats->expr;
                ast_constant_expr_t *constant = &expr->u.constant_expr;
                ast_string_lit_t *string = &constant->lit->u.string_lit;
                if (expr->kind == AST_EXPR_CONSTANT && constant->lit->kind == AST_LIT_STRING && string->str_len > 1) {
                    const ir_constant_t *constant = ir_string_constant(analyzer->factory, string->symbol, string->str_len);
                    ir_block_push_write(block, new_ir_constant_operand(constant), SIZE_MAX);
                } else {
                    ir_operand_t *value = analyze_expr(analyzer, block, formats->expr);
                    const ir_type_t *type = ir_operand_type(value);
                    if (!ir_type_is_std(type)) {
                        msg_t *msg = new_msg(analyzer->source, formats->expr->region,
                            MSG_ERROR, "cannot write value of type `%s`", ir_type_str(type));
                        msg_add_inline_entry(msg, formats->expr->region,
                            "arguments for write statements are of standard types");
                        msg_emit(msg);
                        exit(1);
                    }

                    if (formats->len) {
                        ir_block_push_write(block, value, formats->len->u.number_lit.value);
                    } else {
                        ir_block_push_write(block, value, SIZE_MAX);
                    }
                }
                formats = formats->next;
            }
            break;
        }
        case AST_STMT_COMPOUND: {
            block = analyze_stmt(analyzer, block, stmt->u.compound_stmt.stmts);
            break;
        }
        case AST_STMT_EMPTY:
            break;
        }
        stmt = stmt->next;
    }

    return block;
}

ir_type_t *analyze_param_types(analyzer_t *analyzer, ast_param_decl_t *decl)
{
    ir_type_t *ret, **last;
    assert(analyzer);

    ret = NULL;
    last = &ret;
    while (decl) {
        ast_ident_t *ident = decl->names;
        const ir_type_t *type = analyze_type(analyzer, decl->type);
        if (!ir_type_is_std(type)) {
            msg_t *msg = new_msg(analyzer->source, decl->type->region,
                MSG_ERROR, "invalid parameter of type `%s`", ir_type_str(type));
            msg_add_inline_entry(msg, decl->type->region,
                "parameters are of standard types");
            msg_emit(msg);
            exit(1);
        }
        while (ident) {
            *last = ir_type_ref(type);
            last = &(*last)->next;
            ident = ident->next;
        }
        decl = decl->next;
    }
    return ret;
}

void analyze_variable_decl(analyzer_t *analyzer, ast_variable_decl_t *decl, int local)
{
    assert(analyzer && decl);

    while (decl) {
        ast_ident_t *ident = decl->names;
        const ir_type_t *type = analyze_type(analyzer, decl->type);
        while (ident) {
            maybe_error_conflict(analyzer, ident->symbol, ident->region);
            if (local) {
                ir_item(analyzer->factory, IR_ITEM_LOCAL_VAR, ident->symbol, ident->region, type);
            } else {
                ir_item(analyzer->factory, IR_ITEM_VAR, ident->symbol, ident->region, type);
            }
            ident = ident->next;
        }
        decl = decl->next;
    }
}

void analyze_param_decl(analyzer_t *analyzer, ast_param_decl_t *decl)
{
    assert(analyzer);

    while (decl) {
        ast_ident_t *ident = decl->names;
        const ir_type_t *type = analyze_type(analyzer, decl->type);
        if (!ir_type_is_std(type)) {
            msg_t *msg = new_msg(analyzer->source, decl->type->region,
                MSG_ERROR, "invalid parameter of type `%s`", ir_type_str(type));
            msg_add_inline_entry(msg, decl->type->region,
                "parameters are of standard types");
            msg_emit(msg);
            exit(1);
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
    assert(analyzer && decl_part);

    while (decl_part) {
        switch (decl_part->kind) {
        case AST_DECL_PART_VARIABLE: {
            ast_variable_decl_part_t *decl = &decl_part->u.variable_decl_part;
            analyze_variable_decl(analyzer, decl->decls, 0);
            break;
        }
        case AST_DECL_PART_PROCEDURE: {
            ir_item_t *item;
            ir_block_t *inner = ir_block(analyzer->factory);
            ast_procedure_decl_part_t *decl = &decl_part->u.procedure_decl_part;
            ir_type_t *param_types = analyze_param_types(analyzer, decl->params);

            maybe_error_conflict(analyzer, decl->name->symbol, decl->name->region);
            item = ir_item(analyzer->factory, IR_ITEM_PROCEDURE, decl->name->symbol, decl->name->region, ir_type_procedure(analyzer->factory, param_types));
            ir_scope_push(analyzer->factory, item);
            {
                ast_decl_part_t *decl_part = decl->variables;
                ir_block_t *block;
                analyze_param_decl(analyzer, decl->params);
                if (decl_part) {
                    analyze_variable_decl(analyzer, decl_part->u.variable_decl_part.decls, 1);
                }
                block = analyze_stmt(analyzer, inner, decl->stmt);
                ir_block_terminate_return(block);
            }
            item->body = new_ir_body(inner, ir_scope_pop(analyzer->factory));
            break;
        }
        }
        decl_part = decl_part->next;
    }
}

ir_item_t *analyze_program(analyzer_t *analyzer, ast_program_t *program)
{
    ir_item_t *ret;
    ir_block_t *inner;
    assert(analyzer && program);

    ret = ir_item(analyzer->factory, IR_ITEM_PROGRAM, program->name->symbol, program->name->region, ir_type_program(analyzer->factory));
    inner = ir_block(analyzer->factory);
    ir_scope_push(analyzer->factory, ret);
    {
        ir_block_t *block;
        analyze_decl_part(analyzer, program->decl_part);
        block = analyze_stmt(analyzer, inner, program->stmt);
        ir_block_terminate_return(block);
    }
    ret->body = new_ir_body(inner, ir_scope_pop(analyzer->factory));
    return ret;
}

ir_t *analyze_ast(ast_t *ast)
{
    ir_item_t *items;
    analyzer_t analyzer;
    assert(ast);

    analyzer.source = ast->source;
    analyzer.factory = new_ir_factory();
    items = analyze_program(&analyzer, ast->program);
    return new_ir(analyzer.source, items, analyzer.factory);
}
