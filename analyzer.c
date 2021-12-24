#include <assert.h>

#include "mppl.h"

typedef struct impl_analyzer_scope analyzer_scope_t;
struct impl_analyzer_scope {
    const ir_item_t *owner;
    struct {
        hash_table_t *table;
        ir_item_t **tail;
    } items;
    struct {
        hash_table_t *table;
        ir_local_t **tail;
    } locals;
    analyzer_scope_t *next;
};

typedef struct impl_analyzer_tails analyzer_tails_t;
struct impl_analyzer_tails {
    ir_block_t *tail;
    analyzer_tails_t *next;
};

typedef struct {
    const source_t *source;
    analyzer_scope_t *scope;
    analyzer_tails_t *tails;
    analyzer_tails_t *breaks;
    ir_type_storage_t *type_storage;
} analyzer_t;

void analyzer_push_scope(analyzer_t *analyzer, const ir_item_t *owner, ir_item_t **items, ir_local_t **locals)
{
    analyzer_scope_t *scope;
    assert(analyzer);

    scope = new(analyzer_scope_t);
    scope->owner = owner;
    scope->next = analyzer->scope;
    scope->items.table = new_hash_table(hash_table_default_comparator, hash_table_default_hasher);
    scope->items.tail = items;
    scope->locals.table = new_hash_table(hash_table_default_comparator, hash_table_default_hasher);
    scope->locals.tail = locals;
    analyzer->scope = scope;
}

void analyzer_pop_scope(analyzer_t *analyzer)
{
    analyzer_scope_t *scope;
    ir_item_t *ret;
    assert(analyzer);

    scope = analyzer->scope;
    analyzer->scope = scope->next;
    delete_hash_table(scope->items.table, NULL, NULL);
    delete_hash_table(scope->locals.table, NULL, NULL);
    free(scope);
}

void analyzer_register_item(analyzer_t *analyzer, ir_item_t *item)
{
    const hash_table_entry_t *entry;
    analyzer_scope_t *scope;
    assert(analyzer && item);

    scope = analyzer->scope;
    if (entry = hash_table_find(scope->items.table, (void *) item->symbol)) {
        msg_t *msg = new_msg(analyzer->source, item->name_region, MSG_ERROR, "conflicting names");
        ir_item_t *registered = (ir_item_t *) entry->value;
        msg_add_inline_entry(msg, registered->name_region, "first used here");
        msg_add_inline_entry(msg, item->name_region, "second used here");
        msg_emit(msg);
        exit(1);
    }
    hash_table_insert_unchecked(scope->items.table, (void *) item->symbol, item);
    *scope->items.tail = item;
    scope->items.tail = &item->next;
}

ir_item_t *analyzer_lookup_item(analyzer_t *analyzer, ast_ident_t *ident)
{
    analyzer_scope_t *scope;
    assert(analyzer && ident);

    scope = analyzer->scope;
    while (scope) {
        const hash_table_entry_t *entry;
        if (entry = hash_table_find(scope->items.table, (void *) ident->symbol)) {
            return entry->value;
        }
        scope = scope->next;
    }

    {
        const symbol_instance_t *instance = symbol_get_instance(ident->symbol);
        msg_t *msg = new_msg(analyzer->source, ident->region,
            MSG_ERROR, "`%.*s` is not declared", (int) instance->len, instance->ptr);
        msg_emit(msg);
        exit(1);
    }
}

static ir_local_t *analyzer_append_local(analyzer_t *analyzer, ir_local_t *local)
{
    *analyzer->scope->locals.tail = local;
    analyzer->scope->locals.tail = &local->next;
    return local;
}

ir_local_t *analyzer_create_local_for(analyzer_t *analyzer, ir_item_t *item, size_t pos)
{
    const hash_table_entry_t *entry;
    ir_local_t *local;
    assert(analyzer && item);

    ir_item_add_ref(item, pos);
    if (entry = hash_table_find(analyzer->scope->locals.table, item)) {
        return entry->value;
    }

    switch (item->kind) {
    case IR_ITEM_ARG_VAR:
    case IR_ITEM_LOCAL_VAR:
        local = new_ir_normal_local(item);
    default:
        local = new_ir_ref_local(item);
    }
    hash_table_insert_unchecked(analyzer->scope->locals.table, item, local);
    return analyzer_append_local(analyzer, local);
}

ir_local_t *analyzer_create_temp_local(analyzer_t *analyzer, ir_type_t type)
{
    assert(analyzer && type);
    return analyzer_append_local(analyzer, new_ir_temp_local(type));
}

analyzer_tails_t *analyzer_push_tail(analyzer_tails_t *tails, ir_block_t *block)
{
    analyzer_tails_t *tail;
    assert(tails && block);

    tail = new(analyzer_tails_t);
    tail->tail = block;
    tail->next = tails;
    return tail;
}

void analyzer_connect_tail(analyzer_tails_t *tails, ir_block_t *block)
{
    analyzer_tails_t *cur;
    assert(tails && block);

    while (cur = tails) {
        ir_termn_t *termn = cur->tail->termn;
        switch (termn->kind) {
        case IR_TERMN_GOTO:
            termn->u.goto_termn.next = block;
            break;
        case IR_TERMN_IF:
            assert(termn->u.if_termn.then);
            termn->u.if_termn.els = block;
            break;
        case IR_TERMN_RETURN:
            unreachable();
        }
        tails = cur->next;
        free(cur);
    }
}

ir_type_t analyze_type(analyzer_t *analyzer, ast_type_t *type)
{
    assert(analyzer && type);

    switch (type->kind) {
    case AST_TYPE_BOOLEAN:
        return ir_type_boolean(analyzer->type_storage);
    case AST_TYPE_CHAR:
        return ir_type_char(analyzer->type_storage);
    case AST_TYPE_INTEGER:
        return ir_type_integer(analyzer->type_storage);
    case AST_TYPE_ARRAY: {
        ir_type_t base_type = analyze_type(analyzer, type->u.array_type.base);
        ir_type_instance_t *base_type_ref = new_ir_type_ref(base_type);
        size_t size = type->u.array_type.size->u.number_lit.value;
        if (size == 0) {
            msg_t *msg = new_msg(analyzer->source, type->u.array_type.size->region,
                MSG_ERROR, "size of array needs to be greater than 0");
            msg_emit(msg);
            exit(1);
        }
        return ir_type_array(analyzer->type_storage, base_type_ref, size);
    }
    }

    unreachable();
}

ir_operand_t *analyze_expr(analyzer_t *analyzer, ast_expr_t *expr);

ir_place_t *analyze_lvalue(analyzer_t *analyzer, ast_expr_t *expr)
{
    assert(analyzer && expr);
    assert(expr->kind == AST_EXPR_DECL_REF || expr->kind == AST_EXPR_ARRAY_SUBSCRIPT);

    switch (expr->kind) {
    case AST_EXPR_DECL_REF: {
        ir_item_t *lookup;
        ir_local_t *local;
        ast_ident_t *ident;

        ident = expr->u.decl_ref_expr.decl;
        lookup = analyzer_lookup_item(analyzer, ident);
        local = analyzer_create_local_for(analyzer, lookup, ident->region.pos);
        return new_ir_place(local, NULL);
    }
    case AST_EXPR_ARRAY_SUBSCRIPT: {
        ir_operand_t *index;
        ir_place_access_t *access;
        ir_item_t *lookup;
        ir_local_t *local;
        ir_type_t operand_type;
        ast_ident_t *ident;

        index = analyze_expr(analyzer, expr->u.array_subscript_expr.expr);
        access = new_ir_index_place_access(index);
        ident = expr->u.array_subscript_expr.decl;
        lookup = analyzer_lookup_item(analyzer, ident);
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
        local = analyzer_create_local_for(analyzer, lookup, ident->region.pos);
        return new_ir_place(local, access);
    }
    }
}

void error_invalid_binary_expr(
    analyzer_t *analyzer, ast_binary_expr_t *expr,
    ir_type_t lhs_type, ir_type_t rhs_type, const char *expected)
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

ir_operand_t *analyze_binary_expr(analyzer_t *analyzer, ast_binary_expr_t *expr)
{
    ir_operand_t *lhs, *rhs;
    ir_type_t ltype, rtype;
    ir_local_t *result;
    ir_place_t *place;
    ir_type_t type;
    assert(analyzer && expr);

    rhs = analyze_expr(analyzer, expr->rhs);
    rtype = ir_operand_type(rhs);

    if (expr->lhs->kind == AST_EXPR_EMPTY) {
        if (!ir_type_is_kind(rtype, IR_TYPE_INTEGER)) {
            msg_t *msg = new_msg(analyzer->source, expr->op_region,
                MSG_ERROR, "`%s` cannot be prefixed by `%s`", ir_type_str(rtype), ast_binop_str(expr->kind));
            msg_emit(msg);
            exit(1);
        }

        type = ir_type_integer(analyzer->type_storage);
    } else {
        lhs = analyze_expr(analyzer, expr->lhs);
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
            type = ir_type_boolean(analyzer->type_storage);
            break;
        case AST_BINARY_OP_PLUS:
        case AST_BINARY_OP_MINUS:
        case AST_BINARY_OP_STAR:
        case AST_BINARY_OP_DIV:
            if (!ir_type_is_kind(ltype, IR_TYPE_INTEGER) || !ir_type_is_kind(rtype, IR_TYPE_INTEGER)) {
                error_invalid_binary_expr(analyzer, expr, ltype, rtype, "type integer");
                exit(1);
            }
            type = ir_type_integer(analyzer->type_storage);
            break;
        case AST_BINARY_OP_OR:
        case AST_BINARY_OP_AND:
            if (!ir_type_is_kind(ltype, IR_TYPE_BOOLEAN) || !ir_type_is_kind(rtype, IR_TYPE_BOOLEAN)) {
                error_invalid_binary_expr(analyzer, expr, ltype, rtype, "type boolean");
                exit(1);
            }
            type = ir_type_boolean(analyzer->type_storage);
            break;
        }
    }

    result = analyzer_create_temp_local(analyzer, type);
    place = new_ir_place(result, NULL);
    /* [課題4] 現在のブロックに追加する */
    return new_ir_place_operand(place);
}

ir_operand_t *analyze_unary_expr(analyzer_t *analyzer, ast_unary_expr_t *expr)
{
    ir_operand_t *operand;
    ir_type_t operand_type;
    ir_local_t *result;
    ir_place_t *place;
    ir_type_t type;
    assert(analyzer && expr);

    operand = analyze_expr(analyzer, expr->expr);
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
        type = ir_type_boolean(analyzer->type_storage);
        break;
    }

    result = analyzer_create_temp_local(analyzer, type);
    place = new_ir_place(result, NULL);
    /* [課題4] 現在のブロックに追加する */
    return new_ir_place_operand(place);
}

ir_operand_t *analyze_cast_expr(analyzer_t *analyzer, ast_cast_expr_t *expr)
{
    ir_operand_t *operand;
    ir_type_t operand_type;
    ir_type_t cast_type;
    ir_local_t *result;
    ir_place_t *place;

    operand = analyze_expr(analyzer, expr->expr);
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

    result = analyzer_create_temp_local(analyzer, cast_type);
    /* [課題4] 現在のブロックに追加する */
    place = new_ir_place(result, NULL);
    return new_ir_place_operand(place);
}

ir_operand_t *analyze_constant_expr(analyzer_t *analyzer, ast_constant_expr_t *expr)
{
    assert(analyzer && expr);

    switch (expr->lit->kind) {
    case AST_LIT_NUMBER: {
        ir_constant_t *constant = new_ir_number_constant(
            analyzer->type_storage->std_integer,
            expr->lit->u.number_lit.value);
        return new_ir_constant_operand(constant);
    }
    case AST_LIT_BOOLEAN: {
        ir_constant_t *constant = new_ir_boolean_constant(
            analyzer->type_storage->std_boolean,
            expr->lit->u.boolean_lit.value);
        return new_ir_constant_operand(constant);
    }
    case AST_LIT_STRING: {
        const symbol_instance_t *instance;
        ir_constant_t *constant;
        if (expr->lit->u.string_lit.str_len != 1) {
            msg_t *msg = new_msg(analyzer->source, expr->lit->region, MSG_ERROR, "string is not a valid expression");
            msg_emit(msg);
            exit(1);
        }
        instance = symbol_get_instance(expr->lit->u.string_lit.symbol);
        constant = new_ir_char_constant(analyzer->type_storage->std_char, instance->ptr[0]);
        return new_ir_constant_operand(constant);
    }
    }

    unreachable();
}

ir_operand_t *analyze_expr(analyzer_t *analyzer, ast_expr_t *expr)
{
    assert(analyzer && expr);

    switch (expr->kind) {
    case AST_EXPR_DECL_REF:
    case AST_EXPR_ARRAY_SUBSCRIPT: {
        ir_place_t *place = analyze_lvalue(analyzer, expr);
        return new_ir_place_operand(place);
    }
    case AST_EXPR_BINARY_OP:
        return analyze_binary_expr(analyzer, &expr->u.binary_expr);
    case AST_EXPR_UNARY_OP:
        return analyze_unary_expr(analyzer, &expr->u.unary_expr);
    case AST_EXPR_PAREN:
        return analyze_expr(analyzer, expr->u.paren_expr.expr);
    case AST_EXPR_CAST:
        return analyze_cast_expr(analyzer, &expr->u.cast_expr);
    case AST_EXPR_CONSTANT:
        return analyze_constant_expr(analyzer, &expr->u.constant_expr);
    }

    unreachable();
}

ir_block_t *analyze_stmt(analyzer_t *analyzer, ast_stmt_t *stmt)
{
    ir_block_t *ret;
    assert(analyzer && stmt);

    while (stmt) {
        switch (stmt->kind) {
        case AST_STMT_ASSIGN: {
            ast_assign_stmt_t *assign_stmt = &stmt->u.assign_stmt;
            ir_place_t *lhs = analyze_lvalue(analyzer, assign_stmt->lhs);
            ir_operand_t *rhs_operand = analyze_expr(analyzer, assign_stmt->rhs);
            ir_rvalue_t *rhs = new_ir_use_rvalue(rhs_operand);
            ir_type_t ltype = ir_place_type(lhs);
            ir_type_t rtype = ir_operand_type(rhs_operand);

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

            /* [課題4] 現在のブロックに追加する */
            break;
        }
        case AST_STMT_IF: {
            ast_if_stmt_t *if_stmt = &stmt->u.if_stmt;
            ir_operand_t *cond = analyze_expr(analyzer, stmt->u.if_stmt.cond);
            ir_type_t type = ir_operand_type(cond);
            ir_block_t *then = analyze_stmt(analyzer, stmt->u.if_stmt.then_stmt);
            ir_block_t *els = stmt->u.if_stmt.else_stmt ? analyze_stmt(analyzer, stmt->u.if_stmt.else_stmt) : NULL;

            if (!ir_type_is_kind(type, IR_TYPE_BOOLEAN)) {
                msg_t *msg = new_msg(analyzer->source, if_stmt->cond->region,
                    MSG_ERROR, "expression of type `%s` cannot be condition", ir_type_str(type));
                msg_add_inline_entry(msg, if_stmt->cond->region, "condition expressions are of type boolean");
                msg_emit(msg);
                exit(1);
            }

            /* [課題4] 現在のブロックをifで終端し分岐する */
            break;
        }
        case AST_STMT_WHILE: {
            ast_while_stmt_t *while_stmt = &stmt->u.while_stmt;
            ir_operand_t *cond = analyze_expr(analyzer, while_stmt->cond);
            ir_type_t type = ir_operand_type(cond);
            ir_block_t *block = analyze_stmt(analyzer, while_stmt->do_stmt);

            if (!ir_type_is_kind(type, IR_TYPE_BOOLEAN)) {
                msg_t *msg = new_msg(analyzer->source, while_stmt->cond->region,
                    MSG_ERROR, "expression of type `%s` cannot be condition", ir_type_str(type));
                msg_add_inline_entry(msg, while_stmt->cond->region, "condition expressions are of type boolean");
                msg_emit(msg);
                exit(1);
            }

            /* [課題4] 別のブロックに区切りループする */
            break;
        }
        case AST_STMT_BREAK: {
            /* [課題4] 現在のブロックを未終端で区切り処理を続行する */
            break;
        }
        case AST_STMT_CALL: {
            ir_item_t *item;
            ir_local_t *func;
            ast_expr_t *args;
            ast_ident_t *ident;
            ir_place_t *place, **place_back;
            ir_type_instance_t *type, **type_back;
            ir_type_t interned;
            analyzer_scope_t *scope;

            ident = stmt->u.call_stmt.name;
            item = analyzer_lookup_item(analyzer, ident);
            if (item->kind != IR_ITEM_PROCEDURE) {
                const symbol_instance_t *instance = symbol_get_instance(stmt->u.call_stmt.name->symbol);
                msg_t *msg = new_msg(analyzer->source, ident->region,
                    MSG_ERROR, "`%.*s` is not a procedure", (int) instance->len, instance->ptr);
                msg_emit(msg);
                exit(1);
            }

            scope = analyzer->scope;
            while (scope) {
                if (scope->owner->symbol == item->symbol && scope->owner->kind == IR_ITEM_PROCEDURE) {
                    msg_t *msg = new_msg(analyzer->source, ident->region,
                        MSG_ERROR, "recursive call of procedure is not allowed");
                    msg_emit(msg);
                    exit(1);
                }
                scope = scope->next;
            }

            func = analyzer_create_local_for(analyzer, item, ident->region.pos);
            args = stmt->u.call_stmt.args;
            type = NULL, type_back = &type;
            place = NULL, place_back = &place;
            while (args) {
                ir_operand_t *operand = analyze_expr(analyzer, args);
                if (operand->kind == IR_OPERAND_PLACE) {
                    *place_back = operand->u.place_operand.place;
                } else {
                    ir_local_t *tmp = analyzer_create_temp_local(
                        analyzer, operand->u.constant_operand.constant->type);
                    *place_back = new_ir_place(tmp, NULL);
                }
                operand->kind = -1;
                delete_ir_operand(operand);
                *type_back = new_ir_type_ref(ir_local_type((*place_back)->local));
                type_back = &(*type_back)->next;
                place_back = &(*place_back)->next;
                args = args->next;
            }

            interned = ir_type_procedure(analyzer->type_storage, type);

            if (ir_local_type(func) != interned) {
                const symbol_instance_t *instance = symbol_get_instance(item->symbol);
                msg_t *msg = new_msg(analyzer->source, ident->region,
                    MSG_ERROR, "mismatching arguments for procedure `%.*s`", (int) instance->len, instance->ptr);
                msg_emit(msg);
                exit(1);
            }

            /* [課題4] 現在のブロックに追加する */
            break;
        }
        case AST_STMT_RETURN: {
            /* [課題4] 現在のブロックをreturnで終端し処理を続行する */
            break;
        }
        case AST_STMT_READ: {
            ast_expr_t *args = stmt->u.read_stmt.args;
            ir_place_t *place, **place_back;

            place = NULL, place_back = &place;
            while (args) {
                ir_type_t type;
                if (args->kind != AST_EXPR_DECL_REF && args->kind != AST_EXPR_ARRAY_SUBSCRIPT) {
                    msg_t *msg = new_msg(analyzer->source, args->region,
                        MSG_ERROR, "cannot read value for expression");
                    msg_add_inline_entry(msg, args->region,
                        "arguments for read statements are of reference and of type integer or char");
                    msg_emit(msg);
                    exit(1);
                }

                *place_back = analyze_lvalue(analyzer, args);
                type = ir_place_type(*place_back);
                if (!ir_type_is_kind(type, IR_TYPE_INTEGER) && !ir_type_is_kind(type, IR_TYPE_CHAR)) {
                    msg_t *msg = new_msg(analyzer->source, args->region,
                        MSG_ERROR, "cannot read value for reference of type `%s`", ir_type_str(type));
                    msg_add_inline_entry(msg, args->region,
                        "arguments for read statements are of reference and of type integer or char");
                    msg_emit(msg);
                    exit(1);
                }

                place_back = &(*place_back)->next;
                args = args->next;
            }
            /* [課題4] 現在のブロックに追加する */
            break;
        }
        case AST_STMT_WRITE: {
            ast_output_format_t *formats = stmt->u.write_stmt.formats;

            while (formats) {
                ir_operand_t *operand;
                ir_type_t type;
                if (formats->expr->kind == AST_EXPR_CONSTANT) {
                    if (formats->expr->u.constant_expr.lit->kind == AST_LIT_STRING) {
                        formats = formats->next;
                        continue;
                    }
                }

                operand = analyze_expr(analyzer, formats->expr);
                type = ir_operand_type(operand);
                if (!ir_type_is_std(type)) {
                    msg_t *msg = new_msg(analyzer->source, formats->expr->region,
                        MSG_ERROR, "cannot write value of type `%s`", ir_type_str(type));
                    msg_add_inline_entry(msg, formats->expr->region,
                        "arguments for write statements are of standard types");
                    msg_emit(msg);
                    exit(1);
                }
                formats = formats->next;
            }
            /* [課題4] 現在のブロックに追加する */
            break;
        }
        case AST_STMT_COMPOUND: {
            ir_block_t *block = analyze_stmt(analyzer, stmt->u.compound_stmt.stmts);
            /* [課題4] 現在のブロックに接続する */
            break;
        }
        case AST_STMT_EMPTY:
            break;
        }
        stmt = stmt->next;
    }

    return ret;
}

ir_type_instance_t *analyze_param_types(analyzer_t *analyzer, ast_param_decl_t *decl)
{
    ir_type_instance_t *ret, **last;
    assert(analyzer);

    ret = NULL;
    last = &ret;
    while (decl) {
        ast_ident_t *ident = decl->names;
        ir_type_t type = analyze_type(analyzer, decl->type);
        if (!ir_type_is_std(type)) {
            msg_t *msg = new_msg(analyzer->source, decl->type->region,
                MSG_ERROR, "invalid parameter of type `%s`", ir_type_str(type));
            msg_add_inline_entry(msg, decl->type->region,
                "parameters are of standard types");
            msg_emit(msg);
            exit(1);
        }
        while (ident) {
            *last = new_ir_type_ref(type);
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
        ir_type_t type = analyze_type(analyzer, decl->type);
        while (ident) {
            ir_item_t *item = local
                ? new_ir_local_var_item(type, ident->symbol, ident->region)
                : new_ir_var_item(type, ident->symbol, ident->region);
            analyzer_register_item(analyzer, item);
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
        ir_type_t type = analyze_type(analyzer, decl->type);
        if (!ir_type_is_std(type)) {
            msg_t *msg = new_msg(analyzer->source, decl->type->region,
                MSG_ERROR, "invalid parameter of type `%s`", ir_type_str(type));
            msg_add_inline_entry(msg, decl->type->region,
                "parameters are of standard types");
            msg_emit(msg);
            exit(1);
        }
        while (ident) {
            ir_item_t *item = new_ir_param_var_item(type, ident->symbol, ident->region);
            analyzer_register_item(analyzer, item);
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
            ir_block_t *inner;
            ir_item_t *inner_item;
            ir_local_t *inner_local;
            ast_procedure_decl_part_t *decl = &decl_part->u.procedure_decl_part;
            ir_type_instance_t *param_types = analyze_param_types(analyzer, decl->params);
            ir_type_t type = ir_type_procedure(analyzer->type_storage, param_types);
            ir_item_t *item = new_ir_procedure_item(type, decl->name->symbol, decl->name->region);
            analyzer_register_item(analyzer, item);
            analyzer_push_scope(analyzer, item, &inner_item, &inner_local);
            {
                ast_decl_part_t *decl_part = decl->variables;
                analyze_param_decl(analyzer, decl->params);
                if (decl_part) {
                    analyze_variable_decl(analyzer, decl_part->u.variable_decl_part.decls, 1);
                }
                inner = analyze_stmt(analyzer, decl->stmt);
            }
            analyzer_pop_scope(analyzer);
            item->body = new_ir_body(inner, inner_item, inner_local);
            break;
        }
        }
        decl_part = decl_part->next;
    }
}

ir_item_t *analyze_program(analyzer_t *analyzer, ast_program_t *program)
{
    ir_item_t *ret, *inner_item;
    ir_local_t *inner_local;
    ir_block_t *inner;
    ir_type_t type;
    assert(analyzer && program);

    type = ir_type_program(analyzer->type_storage);
    ret = new_ir_program_item(type, program->name->symbol, program->name->region);
    analyzer_push_scope(analyzer, ret, &inner_item, &inner_local);
    {
        analyze_decl_part(analyzer, program->decl_part);
        inner = analyze_stmt(analyzer, program->stmt);
    }
    analyzer_pop_scope(analyzer);
    ret->body = new_ir_body(inner, inner_item, inner_local);
    return ret;
}

ir_t *analyze_ast(ast_t *ast)
{
    ir_item_t *program;
    analyzer_t analyzer;
    assert(ast);

    analyzer.source = ast->source;
    analyzer.scope = NULL;
    analyzer.type_storage = new_ir_type_storage();
    program = analyze_program(&analyzer, ast->program);
    return new_ir(analyzer.source, program);
}
