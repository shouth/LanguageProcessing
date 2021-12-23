#include <assert.h>

#include "mppl.h"

typedef struct impl_analyzer_scope analyzer_scope_t;
struct impl_analyzer_scope {
    const ir_item_t *owner;
    ir_item_table_t *table;
    struct {
        ir_item_t *head;
        ir_item_t **tail;
    } items;
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

void analyzer_push_scope(analyzer_t *analyzer, const ir_item_t *owner)
{
    analyzer_scope_t *scope;
    assert(analyzer);

    scope = new(analyzer_scope_t);
    scope->owner = owner;
    scope->table = new_ir_item_table();
    scope->next = analyzer->scope;
    scope->items.head = NULL;
    scope->items.tail = &scope->items.head;
    analyzer->scope = scope;
}

ir_item_t *analyzer_pop_scope(analyzer_t *analyzer)
{
    analyzer_scope_t *scope;
    ir_item_t *ret;
    assert(analyzer);

    scope = analyzer->scope;
    ret = scope->items.head;
    analyzer->scope = scope->next;
    delete_ir_item_table(scope->table);
    free(scope);
    return ret;
}

void analyzer_register_item(analyzer_t *analyzer, ir_item_t *item)
{
    ir_item_t *ret;
    assert(analyzer && item);

    if (ret = ir_item_table_try_register(analyzer->scope->table, item)) {
        msg_t *msg = new_msg(analyzer->source, item->name_region, MSG_ERROR, "conflicting names");
        msg_add_inline_entry(msg, ret->name_region, "first used here");
        msg_add_inline_entry(msg, item->name_region, "second used here");
        msg_emit(msg);
        exit(1);
    }
    *analyzer->scope->items.tail = item;
    analyzer->scope->items.tail = &item->next;
}

ir_item_t *analyzer_lookup_item(analyzer_t *analyzer, ast_ident_t *ident)
{
    analyzer_scope_t *scope;
    assert(analyzer && ident);

    scope = analyzer->scope;
    while (scope) {
        ir_item_t *item;
        if (item = ir_item_table_lookup(scope->table, ident->symbol)) {
            return item;
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

ir_local_t *analyzer_create_local_for(analyzer_t *analyzer, ir_item_t *item, size_t pos)
{
    assert(analyzer && item);

    ir_item_add_ref(item, pos);
    switch (item->kind) {
    case IR_ITEM_ARG_VAR:
    case IR_ITEM_LOCAL_VAR:
        return new_ir_normal_local(item);
    default:
        return new_ir_ref_local(item);
    }

    unreachable();
}

ir_local_t *analyzer_create_temp_local(analyzer_t *analyzer, ir_type_t type)
{
    assert(analyzer && type);
    return new_ir_temp_local(type);
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

static ir_type_instance_t *internal_analyze_type(analyzer_t *analyzer, ast_type_t *type)
{
    assert(analyzer && type);

    switch (type->kind) {
    case AST_TYPE_BOOLEAN:
        return new_ir_boolean_type_instance();
    case AST_TYPE_CHAR:
        return new_ir_char_type_instance();
    case AST_TYPE_INTEGER:
        return new_ir_integer_type_instance();
    case AST_TYPE_ARRAY: {
        ir_type_instance_t *base_type = internal_analyze_type(analyzer, type->u.array_type.base);
        size_t size = type->u.array_type.size->u.number_lit.value;
        if (size == 0) {
            msg_t *msg = new_msg(analyzer->source, type->u.array_type.size->region,
                MSG_ERROR, "size of array needs to be greater than 0");
            msg_emit(msg);
            exit(1);
        }
        return new_ir_array_type_instance(base_type, size);
    }
    }

    unreachable();
}

ir_type_t analyze_type(analyzer_t *analyzer, ast_type_t *type)
{
    ir_type_instance_t *instance;
    assert(analyzer && type);

    instance = internal_analyze_type(analyzer, type);
    return ir_type_intern(analyzer->type_storage, instance);
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
        if (!ir_type_is_kind(ir_operand_type(index), IR_TYPE_INTEGER)) {
            msg_t *msg = new_msg(analyzer->source, expr->u.array_subscript_expr.expr->region,
                MSG_ERROR, "index needs to be integer");
            msg_emit(msg);
            exit(1);
        }
        local = analyzer_create_local_for(analyzer, lookup, ident->region.pos);
        return new_ir_place(local, access);
    }
    }
}

ir_operand_t *analyze_binary_expr(analyzer_t *analyzer, ast_binary_expr_t *expr)
{
    ir_operand_t *lhs, *rhs;
    ir_type_t ltype, rtype;
    ir_local_t *result;
    ir_place_t *place;
    ir_type_instance_t *instance;
    ir_type_t type;
    assert(analyzer && expr);

    rhs = analyze_expr(analyzer, expr->rhs);
    rtype = ir_operand_type(rhs);

    if (expr->lhs->kind == AST_EXPR_EMPTY) {
        assert(expr->kind == AST_BINARY_OP_PLUS || expr->kind == AST_BINARY_OP_MINUS);
        if (!ir_type_is_kind(rtype, IR_TYPE_INTEGER)) {
            /* エラー */
            fprintf(stderr, "the type of the operand needs to be integer.\n");
            exit(1);
        }

        instance = new_ir_integer_type_instance();
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
            if (ltype != rtype) {
                /* エラー */
                fprintf(stderr, "the types of the operands need to be same.\n");
                exit(1);
            }
            if (!ir_type_is_std(ltype) || !ir_type_is_std(rtype)) {
                /* エラー */
                fprintf(stderr, "the types of the operands need to be standard types.\n");
                exit(1);
            }
            instance = new_ir_boolean_type_instance();
            break;
        case AST_BINARY_OP_PLUS:
        case AST_BINARY_OP_MINUS:
        case AST_BINARY_OP_STAR:
        case AST_BINARY_OP_DIV:
            if (!ir_type_is_kind(ltype, IR_TYPE_INTEGER) || !ir_type_is_kind(rtype, IR_TYPE_INTEGER)) {
                /* エラー */
                fprintf(stderr, "the type of the operands need to be integer.\n");
                exit(1);
            }
            instance = new_ir_integer_type_instance();
            break;
        case AST_BINARY_OP_OR:
        case AST_BINARY_OP_AND:
            if (!ir_type_is_kind(ltype, IR_TYPE_BOOLEAN) || !ir_type_is_kind(rtype, IR_TYPE_BOOLEAN)) {
                /* エラー */
                fprintf(stderr, "the type of the operands need to be boolean.\n");
                exit(1);
            }
            instance = new_ir_boolean_type_instance();
            break;
        }
    }

    type = ir_type_intern(analyzer->type_storage, instance);
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
    ir_type_instance_t *instance;
    ir_type_t type;
    assert(analyzer && expr);

    operand = analyze_expr(analyzer, expr->expr);
    operand_type = ir_operand_type(operand);

    switch (expr->kind) {
    case AST_UNARY_OP_NOT:
        if (!ir_type_is_kind(operand_type, IR_TYPE_BOOLEAN)) {
            /* エラー */
            fprintf(stderr, "the types of the operands need to be boolean.\n");
            exit(1);
        }
        instance = new_ir_boolean_type_instance();
        break;
    }

    type = ir_type_intern(analyzer->type_storage, instance);
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
        /* エラー */
        fprintf(stderr, "operand needs to be a standard type.\n");
        exit(1);
    }
    if (!ir_type_is_std(cast_type)) {
        /* エラー */
        fprintf(stderr, "casting to non-standard types is not allowed.\n");
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
            ir_place_t *lhs = analyze_lvalue(analyzer, stmt->u.assign_stmt.lhs);
            ir_operand_t *rhs_operand = analyze_expr(analyzer, stmt->u.assign_stmt.rhs);
            ir_rvalue_t *rhs = new_ir_use_rvalue(rhs_operand);
            ir_type_t ltype = ir_place_type(lhs);
            ir_type_t rtype = ir_operand_type(rhs_operand);

            if (ltype != rtype) {
                /* エラー */
                fprintf(stderr, "types of operands need to be same.\n");
                exit(1);
            }
            if (!ir_type_is_std(ltype) || !ir_type_is_std(rtype)) {
                /* エラー */
                fprintf(stderr, "operands need to be a standard type.\n");
                exit(1);
            }

            /* [課題4] 現在のブロックに追加する */
            break;
        }
        case AST_STMT_IF: {
            ir_operand_t *cond = analyze_expr(analyzer, stmt->u.if_stmt.cond);
            ir_type_t type = ir_operand_type(cond);
            ir_block_t *then = analyze_stmt(analyzer, stmt->u.if_stmt.then_stmt);
            ir_block_t *els = stmt->u.if_stmt.else_stmt ? analyze_stmt(analyzer, stmt->u.if_stmt.else_stmt) : NULL;

            if (!ir_type_is_kind(type, IR_TYPE_BOOLEAN)) {
                /* エラー */
                fprintf(stderr, "type of condition expression needs to be boolean.\n");
                exit(1);
            }

            /* [課題4] 現在のブロックをifで終端し分岐する */
            break;
        }
        case AST_STMT_WHILE: {
            ir_operand_t *cond = analyze_expr(analyzer, stmt->u.while_stmt.cond);
            ir_type_t type = ir_operand_type(cond);
            ir_block_t *block = analyze_stmt(analyzer, stmt->u.while_stmt.do_stmt);

            if (!ir_type_is_kind(type, IR_TYPE_BOOLEAN)) {
                /* エラー */
                fprintf(stderr, "type of condition expression needs to be boolean.\n");
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
            ir_type_instance_t *procedure_instance;
            ir_type_t interned;
            analyzer_scope_t *scope;

            ident = stmt->u.call_stmt.name;
            item = analyzer_lookup_item(analyzer, ident);
            if (item->kind != IR_ITEM_PROCEDURE) {
                /* エラー */
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

            procedure_instance = new_ir_procedure_type_instance(type);
            interned = ir_type_intern(analyzer->type_storage, procedure_instance);

            if (ir_local_type(func) != interned) {
                /* エラー */
                const symbol_instance_t *instance = symbol_get_instance(item->symbol);
                fprintf(stderr, "arguments are not suitable for procedure `%.*s`.\n", (int) instance->len, instance->ptr);
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
                    /* エラー */
                    fprintf(stderr, "arguments for input statement needs to be references.\n");
                    exit(1);
                }

                *place_back = analyze_lvalue(analyzer, args);
                type = ir_place_type(*place_back);
                if (!ir_type_is_kind(type, IR_TYPE_INTEGER) && !ir_type_is_kind(type, IR_TYPE_CHAR)) {
                    /* エラー */
                    fprintf(stderr, "types of arguments for input statement needs to be integer or char.\n");
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
                if (formats->expr->kind == AST_EXPR_CONSTANT) {
                    if (formats->expr->u.constant_expr.lit->kind == AST_LIT_STRING) {
                        formats = formats->next;
                        continue;
                    }
                }

                operand = analyze_expr(analyzer, formats->expr);
                if (!ir_type_is_std(ir_operand_type(operand))) {
                    /* エラー */
                    fprintf(stderr, "arguments for output statement needs to be standard types.\n");
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
            /* エラー */
            fprintf(stderr, "parameters needs to be standard types.\n");
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
            /* エラー */
            fprintf(stderr, "parameters needs to be standard types.\n");
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
            ast_procedure_decl_part_t *decl;
            ir_type_instance_t *param_types;
            ir_type_instance_t *instance;
            ir_type_t type;
            ir_block_t *inner;
            ir_item_t *item, *inner_item;

            decl = &decl_part->u.procedure_decl_part;
            param_types = analyze_param_types(analyzer, decl->params);
            instance = new_ir_procedure_type_instance(param_types);
            type = ir_type_intern(analyzer->type_storage, instance);
            item = new_ir_procedure_item(type, decl->name->symbol, decl->name->region);
            analyzer_register_item(analyzer, item);

            analyzer_push_scope(analyzer, item);
            {
                ast_decl_part_t *decl_part = decl->variables;
                analyze_param_decl(analyzer, decl->params);
                if (decl_part) {
                    analyze_variable_decl(analyzer, decl_part->u.variable_decl_part.decls, 1);
                }
                inner = analyze_stmt(analyzer, decl->stmt);
            }
            inner_item = analyzer_pop_scope(analyzer);
            item->body = new_ir_body(inner, inner_item);
            break;
        }
        }
        decl_part = decl_part->next;
    }
}

ir_item_t *analyze_program(analyzer_t *analyzer, ast_program_t *program)
{
    ir_item_t *ret, *inner_item;
    ir_block_t *inner;
    ir_type_instance_t *instance;
    ir_type_t type;
    assert(analyzer && program);

    instance = new_ir_program_type_instance();
    type = ir_type_intern(analyzer->type_storage, instance);
    ret = new_ir_program_item(type, program->name->symbol, program->name->region);
    analyzer_push_scope(analyzer, ret);
    {
        analyze_decl_part(analyzer, program->decl_part);
        inner = analyze_stmt(analyzer, program->stmt);
    }
    inner_item = analyzer_pop_scope(analyzer);
    ret->body = new_ir_body(inner, inner_item);
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
