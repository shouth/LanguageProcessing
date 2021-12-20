#include <assert.h>

#include "mppl.h"

typedef struct impl_analyzer_scope analyzer_scope_t;
struct impl_analyzer_scope {
    ir_item_table_t *table;
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

void analyzer_push_scope(analyzer_t *analyzer)
{
    analyzer_scope_t *scope;
    assert(analyzer);

    scope = new(analyzer_scope_t);
    scope->table = new_ir_item_table();
    scope->next = analyzer->scope;
    analyzer->scope = scope;
}

ir_item_table_t *analyzer_pop_scope(analyzer_t *analyzer)
{
    analyzer_scope_t *scope;
    ir_item_table_t *ret;
    assert(analyzer);

    scope = analyzer->scope;
    ret = scope->table;
    analyzer->scope = scope->next;
    free(scope);
    return ret;
}

const ir_item_t *analyzer_try_register_item(analyzer_t *analyzer, ir_item_t *item)
{
    analyzer_scope_t *scope;
    const ir_item_t *ret;
    assert(analyzer && item);

    ret = ir_item_table_try_register(analyzer->scope->table, item);
    return ret ? ret : NULL;
}

const ir_item_t *analyzer_lookup_item(analyzer_t *analyzer, symbol_t symbol)
{
    analyzer_scope_t *scope;
    assert(analyzer && symbol);

    scope = analyzer->scope;
    while (scope) {
        const ir_item_t *item;
        if (item = ir_item_table_lookup(scope->table, symbol)) {
            return item;
        }
        scope = scope->next;
    }
    return NULL;
}

ir_local_t *analyzer_create_local_for(analyzer_t *analyzer, const ir_item_t *item)
{
    assert(analyzer && item);

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
    ir_type_instance_t *instance;
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
            /* エラー */
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
        const ir_item_t *lookup;
        ir_local_t *local;

        lookup = analyzer_lookup_item(analyzer, expr->u.decl_ref_expr.decl->symbol);
        if (!lookup) {
            /* エラー */
        }
        local = analyzer_create_local_for(analyzer, lookup);
        return new_ir_place(local, NULL);
    }
    case AST_EXPR_ARRAY_SUBSCRIPT: {
        ir_operand_t *index;
        ir_place_access_t *access;
        const ir_item_t *lookup;
        ir_local_t *local;

        index = analyze_expr(analyzer, expr->u.array_subscript_expr.expr);
        access = new_ir_index_place_access(index);
        lookup = analyzer_lookup_item(analyzer, expr->u.array_subscript_expr.decl->symbol);
        if (!lookup) {
            /* エラー */
        }
        local = analyzer_create_local_for(analyzer, lookup);
        return new_ir_place(local, access);
    }
    }
}

ir_operand_t *analyze_binary_expr(analyzer_t *analyzer, ast_binary_expr_t *expr)
{
    ir_rvalue_t *ret;
    ir_operand_t *lhs, *rhs;
    ir_type_kind_t ltype, rtype;
    ir_local_t *result;
    ir_place_t *place;
    ir_type_instance_t *instance;
    ir_type_t type;
    assert(analyzer && expr);

    rhs = analyze_expr(analyzer, expr->rhs);
    rtype = ir_operand_type_kind(rhs);

    if (expr->lhs->kind == AST_EXPR_EMPTY) {
        assert(expr->kind == AST_BINARY_OP_PLUS || expr->kind == AST_BINARY_OP_MINUS);
        if (!ir_type_kind_is_std(rtype)) {
            /* エラー */
        }

        instance = new_ir_integer_type_instance();
    } else {
        lhs = analyze_expr(analyzer, expr->lhs);
        ltype = ir_operand_type_kind(lhs);

        switch (expr->kind) {
        case AST_BINARY_OP_EQUAL:
        case AST_BINARY_OP_NOTEQ:
        case AST_BINARY_OP_LE:
        case AST_BINARY_OP_LEEQ:
        case AST_BINARY_OP_GR:
        case AST_BINARY_OP_GREQ:
            if (ltype != rtype || !ir_type_kind_is_std(ltype) || !ir_type_kind_is_std(rtype)) {
                /* エラー */
            }
            instance = new_ir_boolean_type_instance();
            break;
        case AST_BINARY_OP_PLUS:
        case AST_BINARY_OP_MINUS:
        case AST_BINARY_OP_STAR:
        case AST_BINARY_OP_DIV:
            if (ltype != IR_TYPE_INTEGER || rtype != IR_TYPE_INTEGER) {
                /* エラー */
            }
            instance = new_ir_integer_type_instance();
            break;
        case AST_BINARY_OP_OR:
        case AST_BINARY_OP_AND:
            if (ltype != IR_TYPE_BOOLEAN || rtype != IR_TYPE_BOOLEAN) {
                /* エラー */
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
    ir_rvalue_t *ret;
    ir_operand_t *operand;
    ir_type_kind_t type_kind;
    ir_local_t *result;
    ir_place_t *place;
    ir_type_instance_t *instance;
    ir_type_t type;
    assert(analyzer && expr);

    operand = analyze_expr(analyzer, expr->expr);
    type_kind = ir_operand_type_kind(operand);

    switch (expr->kind) {
    case AST_UNARY_OP_NOT:
        if (type_kind != IR_TYPE_BOOLEAN) {
            /* エラー */
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
    ir_type_kind_t type_kind;
    ir_type_kind_t cast_kind;
    ir_type_t cast;
    ir_rvalue_t *rvalue;
    ir_local_t *result;
    ir_place_t *place;
    ir_type_instance_t *instance;
    ir_type_t type;

    operand = analyze_expr(analyzer, expr->expr);
    type_kind = ir_operand_type_kind(operand);
    cast = analyze_type(analyzer, expr->type);
    cast_kind = ir_type_get_instance(cast)->kind;

    if (!ir_type_kind_is_std(type_kind) || !ir_type_kind_is_std(cast_kind)) {
        /* エラー */
    }

    switch (cast_kind) {
    case IR_TYPE_INTEGER:
        instance = new_ir_integer_type_instance();
        break;
    case IR_TYPE_BOOLEAN:
        instance = new_ir_boolean_type_instance();
        break;
    case IR_TYPE_CHAR:
        instance = new_ir_char_type_instance();
        break;
    default:
        unreachable();
    }

    type = ir_type_intern(analyzer->type_storage, instance);
    rvalue = new_ir_cast_rvalue(cast, operand);
    result = analyzer_create_temp_local(analyzer, type);
    /* [課題4] 現在のブロックに追加する */
    place = new_ir_place(result, NULL);
    return new_ir_place_operand(place);
}

ir_operand_t *analyze_constant_expr(analyzer_t *analyzer, ast_constant_expr_t *expr)
{
    assert(analyzer && expr);

    switch (expr->lit->kind) {
    case AST_LIT_NUMBER: {
        ir_constant_t *constant = new_ir_number_constant(expr->lit->u.number_lit.value);
        return new_ir_constant_operand(constant);
    }
    case AST_LIT_BOOLEAN: {
        ir_constant_t *constant = new_ir_boolean_constant(expr->lit->u.boolean_lit.value);
        return new_ir_constant_operand(constant);
    }
    case AST_LIT_STRING: {
        const symbol_instance_t *instance;
        ir_constant_t *constant;
        if (expr->lit->u.string_lit.str_len != 1) {
            /* エラー */
        }
        instance = symbol_get_instance(expr->lit->u.string_lit.symbol);
        constant = new_ir_char_constant(instance->ptr[0]);
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
            /* [課題4] 現在のブロックに追加する */
            break;
        }
        case AST_STMT_IF: {
            ir_operand_t *cond = analyze_expr(analyzer, stmt->u.if_stmt.cond);
            ir_type_kind_t type_kind = ir_operand_type_kind(cond);
            ir_block_t *then = analyze_stmt(analyzer, stmt->u.if_stmt.then_stmt);
            ir_block_t *els = stmt->u.if_stmt.else_stmt ? analyze_stmt(analyzer, stmt->u.if_stmt.else_stmt) : NULL;

            if (type_kind != IR_TYPE_BOOLEAN) {
                /* エラー */
            }

            /* [課題4] 現在のブロックをifで終端し分岐する */
            break;
        }
        case AST_STMT_WHILE: {
            ir_operand_t *cond = analyze_expr(analyzer, stmt->u.while_stmt.cond);
            ir_type_kind_t type_kind = ir_operand_type_kind(cond);
            ir_block_t *block = analyze_stmt(analyzer, stmt->u.while_stmt.do_stmt);

            if (type_kind != IR_TYPE_BOOLEAN) {
                /* エラー */
            }

            /* [課題4] 別のブロックに区切りループする */
            break;
        }
        case AST_STMT_BREAK: {
            /* [課題4] 現在のブロックを未終端で区切り処理を続行する */
            break;
        }
        case AST_STMT_CALL: {
            const ir_item_t *item;
            ir_local_t *func;
            ast_expr_t *args;
            ir_place_t *place, **place_back;
            ir_type_instance_t *type, **type_back;
            ir_type_instance_t *procedure_instance;
            ir_type_t interned;

            item = analyzer_lookup_item(analyzer, stmt->u.call_stmt.name->symbol);
            if (!func) {
                /* エラー */
            }
            func = analyzer_create_local_for(analyzer, item);
            args = stmt->u.call_stmt.args;
            type = NULL, type_back = &type;
            place = NULL, place_back = &place;
            while (args) {
                ir_operand_t *operand = analyze_expr(analyzer, args);
                if (operand->kind == IR_OPERAND_PLACE) {
                    *place_back = operand->u.place_operand.place;
                } else {
                    ir_local_t *tmp; /* 一時変数に格納し引数とする */
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
                if (args->kind != AST_EXPR_DECL_REF || args->kind != AST_EXPR_ARRAY_SUBSCRIPT) {
                    /* エラー */
                }

                *place_back = analyze_lvalue(analyzer, args);
                if (!ir_type_kind_is_std(ir_place_type_kind(*place_back))) {
                    /* エラー */
                }

                place_back = &(*place_back)->next;
                args = args->next;
            }
            break;
        }
        case AST_STMT_WRITE: {
            ast_output_format_t *formats = stmt->u.write_stmt.formats;

            while (formats) {
                ir_operand_t *operand = analyze_expr(analyzer, formats->expr);
                if (!ir_type_kind_is_std(ir_operand_type_kind(operand))) {
                    /* エラー */
                }
                formats = formats->next;
            }
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
    assert(analyzer && decl);

    ret = NULL;
    last = &ret;
    while (decl) {
        ast_ident_t *ident = decl->names;
        ir_type_t type = analyze_type(analyzer, decl->type);
        if (!ir_type_kind_is_std(ir_type_get_instance(type)->kind)) {
            /* エラー */
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
            ir_item_t *item = local ? new_ir_local_var_item(type, ident->symbol) : new_ir_var_item(type, ident->symbol);
            if (analyzer_try_register_item(analyzer, item)) {
                /* エラー */
            }
            ident = ident->next;
        }
        decl = decl->next;
    }
}

void analyze_param_decl(analyzer_t *analyzer, ast_param_decl_t *decl)
{
    assert(analyzer && decl);

    while (decl) {
        ast_ident_t *ident = decl->names;
        ir_type_t type = analyze_type(analyzer, decl->type);
        if (!ir_type_kind_is_std(ir_type_get_instance(type)->kind)) {
            /* エラー */
        }
        while (ident) {
            ir_item_t *item = new_ir_param_var_item(type, ident->symbol);
            if (analyzer_try_register_item(analyzer, item)) {
                /* エラー */
            }
            ident = ident->next;
        }
        decl = decl->next;
    }
}

void analyze_decl_part(analyzer_t *analyzer, ast_decl_part_t *decl_part)
{
    ir_item_table_t *table;
    assert(analyzer && decl_part);

    table = new_ir_item_table();
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
            ir_item_t *item;
            ir_item_table_t *items;

            decl = &decl_part->u.procedure_decl_part;
            param_types = analyze_param_types(analyzer, decl->params);
            instance = new_ir_procedure_type_instance(param_types);
            type = ir_type_intern(analyzer->type_storage, instance);
            item = new_ir_procedure_item(type, decl->name->symbol);
            if (analyzer_try_register_item(analyzer, item)) {
                /* エラー */
            }

            analyzer_push_scope(analyzer);
            {
                ast_decl_part_t *decl_part = decl->variables;
                analyze_param_decl(analyzer, decl->params);
                if (decl_part) {
                    analyze_variable_decl(analyzer, decl_part->u.variable_decl_part.decls, 1);
                }
                inner = analyze_stmt(analyzer, decl->stmt);
            }
            items = analyzer_pop_scope(analyzer);
            item->body = new_ir_body(inner, items);
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
    ir_item_table_t *items;
    ir_type_instance_t *instance;
    ir_type_t type;
    assert(analyzer && program);

    instance = new_ir_program_type_instance();
    type = ir_type_intern(analyzer->type_storage, instance);
    ret = new_ir_program_item(type, program->name->symbol);
    analyzer_push_scope(analyzer);
    analyze_decl_part(analyzer, program->decl_part);
    inner = analyze_stmt(analyzer, program->stmt);
    items = analyzer_pop_scope(analyzer);
    ret->body = new_ir_body(inner, items);
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
    return new_ir(program);
}
