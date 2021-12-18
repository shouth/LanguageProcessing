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
    ir_type_storage_t *type_storage;
    analyzer_scope_t *scope;
    analyzer_tails_t *tails;
    analyzer_tails_t *breaks;
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
            break;
        }
        tails = cur->next;
        free(cur);
    }
}

ir_block_t *analyze_stmt(analyzer_t *analyzer, ast_stmt_t *stmt)
{
    ir_block_t *ret;
    assert(analyzer && stmt);

    while (stmt) {
        switch (stmt->kind) {
        case AST_STMT_ASSIGN:
            break;
        case AST_STMT_IF:
            break;
        case AST_STMT_WHILE:
            break;
        case AST_STMT_BREAK:
            break;
        case AST_STMT_CALL:
            break;
        case AST_STMT_RETURN:
            break;
        case AST_STMT_READ:
            break;
        case AST_STMT_WRITE:
            break;
        case AST_STMT_COMPOUND:
            break;
        case AST_STMT_EMPTY:
            break;
        }
        stmt = stmt->next;
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

ir_type_instance_t *analyze_param_types(analyzer_t *analyzer, ast_param_decl_t *decl)
{
    ir_type_instance_t *ret, **last;
    assert(analyzer && decl);

    ret = NULL;
    last = &ret;
    while (decl) {
        ast_ident_t *ident = decl->names;
        ir_type_t type = analyze_type(analyzer, decl->type);
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
            /* TODO: ここでテーブルに挿入する */
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
        while (ident) {
            ir_item_t *item = new_ir_param_var_item(type, ident->symbol);
            /* TODO: ここでテーブルに挿入する */
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
            /* TODO: ここでテーブルに挿入する */
            analyzer_push_scope(analyzer);
            analyze_param_decl(analyzer, decl->params);
            analyze_variable_decl(analyzer, decl->variables->u.variable_decl_part.decls, 1);
            inner = analyze_stmt(analyzer, decl->stmt);
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
