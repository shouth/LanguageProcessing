#include <assert.h>
#include <stdlib.h>

#include "ast.h"
#include "util.h"

static lit_t *new_lit(lit_kind_t kind)
{
    lit_t *ret = new(lit_t);
    ret->kind = kind;
    return ret;
}

lit_t *new_number_lit(const char *ptr, size_t len, unsigned long value)
{
    lit_t *ret = new_lit(LIT_NUMBER);
    ret->u.number_lit.ptr = ptr;
    ret->u.number_lit.len = len;
    ret->u.number_lit.value = value;
    return ret;
}

lit_t *new_boolean_lit(int value)
{
    lit_t *ret = new_lit(LIT_BOOLEAN);
    ret->u.boolean_lit.value = value;
    return ret;
}

lit_t *new_string_lit(const char *ptr, size_t len, size_t str_len)
{
    lit_t *ret = new_lit(LIT_STRING);
    ret->u.string_lit.ptr = ptr;
    ret->u.string_lit.len = len;
    ret->u.string_lit.str_len = str_len;
    return ret;
}

void delete_lit(lit_t *lit)
{
    if (!lit) {
        return;
    }
    free(lit);
}

type_t *new_std_type(type_kind_t kind)
{
    type_t *ret = new(type_t);
    ret->kind = kind;
    return ret;
}

type_t *new_array_type(type_t *base, lit_t *size)
{
    type_t *ret = new(type_t);
    ret->kind = TYPE_ARRAY;
    ret->array.base = base;
    ret->array.size = size;
    return ret;
}

void delete_type(type_t *type)
{
    if (!type) {
        return;
    }
    if (type->kind == TYPE_ARRAY) {
        delete_type(type->array.base);
        delete_lit(type->array.size);
    }
    free(type);
}

ident_t *new_ident(const char *ptr, size_t len)
{
    ident_t *ret = new(ident_t);
    ret->next = NULL;
    ret->ptr = ptr;
    ret->len = len;
    return ret;
}

void delete_ident(ident_t *ident)
{
    if (!ident) {
        return;
    }
    delete_ident(ident->next);
    free(ident);
}

static expr_t *new_expr(expr_kind_t kind)
{
    expr_t *ret = new(expr_t);
    ret->kind = kind;
    ret->next = NULL;
    return ret;
}

expr_t *new_binary_expr(binary_op_kind_t kind, expr_t *lhs, expr_t *rhs)
{
    expr_t *ret = new_expr(EXPR_BINARY_OP);
    ret->u.binary_expr.kind = kind;
    ret->u.binary_expr.lhs = lhs;
    ret->u.binary_expr.rhs = rhs;
    return ret;
}

expr_t *new_unary_expr(unary_op_kind_t kind, expr_t *expr)
{
    expr_t *ret = new_expr(EXPR_UNARY_OP);
    ret->u.unary_expr.kind = kind;
    ret->u.unary_expr.expr = expr;
    return ret;
}

expr_t *new_paren_expr(expr_t *expr)
{
    expr_t *ret = new_expr(EXPR_PAREN);
    ret->u.paren_expr.expr = expr;
    return ret;
}

expr_t *new_cast_expr(type_t *type, expr_t *expr)
{
    expr_t *ret = new_expr(EXPR_CAST);
    ret->u.cast_expr.type = type;
    ret->u.cast_expr.expr = expr;
    return ret;
}

expr_t *new_decl_ref_expr(ident_t *decl)
{
    expr_t *ret = new_expr(EXPR_DECL_REF);
    ret->u.decl_ref_expr.decl = decl;
    return ret;
}

expr_t *new_array_subscript_expr(ident_t *decl, expr_t *expr)
{
    expr_t *ret = new_expr(EXPR_ARRAY_SUBSCRIPT);
    ret->u.array_subscript_expr.decl = decl;
    ret->u.array_subscript_expr.expr = expr;
    return ret;
}

expr_t *new_constant_expr(lit_t *lit)
{
    expr_t *ret = new_expr(EXPR_CONSTANT);
    ret->u.constant_expr.lit = lit;
    return ret;
}

expr_t *new_empty_expr()
{
    return new_expr(EXPR_EMPTY);
}

void delete_expr(expr_t *expr)
{
    if (!expr) {
        return;
    }
    switch (expr->kind) {
    case EXPR_BINARY_OP:
        delete_expr(expr->u.binary_expr.lhs);
        delete_expr(expr->u.binary_expr.rhs);
        break;
    case EXPR_UNARY_OP:
        delete_expr(expr->u.unary_expr.expr);
        break;
    case EXPR_PAREN:
        delete_expr(expr->u.paren_expr.expr);
        break;
    case EXPR_CAST:
        delete_type(expr->u.cast_expr.type);
        delete_expr(expr->u.cast_expr.expr);
        break;
    case EXPR_DECL_REF:
        delete_ident(expr->u.decl_ref_expr.decl);
        break;
    case EXPR_ARRAY_SUBSCRIPT:
        delete_ident(expr->u.array_subscript_expr.decl);
        delete_expr(expr->u.array_subscript_expr.expr);
        break;
    case EXPR_CONSTANT:
        delete_lit(expr->u.constant_expr.lit);
        break;
    }
    delete_expr(expr->next);
    free(expr);
}

static stmt_t *new_stmt(stmt_kind_t kind)
{
    stmt_t *ret = new(stmt_t);
    ret->kind = kind;
    ret->next = NULL;
    return ret;
}

stmt_t *new_assign_stmt(expr_t *lhs, expr_t *rhs)
{
    stmt_t *ret = new_stmt(STMT_ASSIGN);
    ret->u.assign_stmt.lhs = lhs;
    ret->u.assign_stmt.rhs = rhs;
    return ret;
}

stmt_t *new_if_stmt(expr_t *cond, stmt_t *then_stmt, stmt_t *else_stmt)
{
    stmt_t *ret = new_stmt(STMT_IF);
    ret->u.if_stmt.cond = cond;
    ret->u.if_stmt.then_stmt = then_stmt;
    ret->u.if_stmt.else_stmt = else_stmt;
    return ret;
}

stmt_t *new_while_stmt(expr_t *cond, stmt_t *do_stmt)
{
    stmt_t *ret = new_stmt(STMT_WHILE);
    ret->u.while_stmt.cond = cond;
    ret->u.while_stmt.do_stmt = do_stmt;
    return ret;
}

stmt_t *new_break_stmt()
{
    return new_stmt(STMT_BREAK);
}

stmt_t *new_call_stmt(ident_t *name, expr_t *args)
{
    stmt_t *ret = new_stmt(STMT_CALL);
    ret->u.call_stmt.name = name;
    ret->u.call_stmt.args = args;
    return ret;
}

stmt_t *new_return_stmt()
{
    return new_stmt(STMT_RETURN);
}

stmt_t *new_read_stmt(int newline, expr_t *args)
{
    stmt_t *ret = new_stmt(STMT_READ);
    ret->u.read_stmt.newline = newline;
    ret->u.read_stmt.args = args;
    return ret;
}

stmt_t *new_write_stmt(int newline, output_format_t *formats)
{
    stmt_t *ret = new_stmt(STMT_WRITE);
    ret->u.write_stmt.newline = newline;
    ret->u.write_stmt.formats = formats;
    return ret;
}

stmt_t *new_compound_stmt(stmt_t *stmts)
{
    stmt_t *ret = new_stmt(STMT_COMPOUND);
    ret->u.compound_stmt.stmts = stmts;
    return ret;
}

stmt_t *new_empty_stmt()
{
    return new_stmt(STMT_EMPTY);
}

void delete_stmt(stmt_t *stmt)
{
    if (!stmt) {
        return;
    }
    switch (stmt->kind) {
    case STMT_ASSIGN:
        delete_expr(stmt->u.assign_stmt.lhs);
        delete_expr(stmt->u.assign_stmt.rhs);
        break;
    case STMT_IF:
        delete_expr(stmt->u.if_stmt.cond);
        delete_stmt(stmt->u.if_stmt.then_stmt);
        delete_stmt(stmt->u.if_stmt.else_stmt);
        break;
    case STMT_WHILE:
        delete_expr(stmt->u.while_stmt.cond);
        delete_stmt(stmt->u.while_stmt.do_stmt);
        break;
    case STMT_CALL:
        delete_ident(stmt->u.call_stmt.name);
        delete_expr(stmt->u.call_stmt.args);
        break;
    case STMT_READ:
        delete_expr(stmt->u.read_stmt.args);
        break;
    case STMT_WRITE:
        delete_output_format(stmt->u.write_stmt.formats);
        break;
    case STMT_COMPOUND:
        delete_stmt(stmt->u.compound_stmt.stmts);
        break;
    }
    delete_stmt(stmt->next);
    free(stmt);
}

output_format_t *new_output_format(expr_t *expr, lit_t *len)
{
    output_format_t *ret = new(output_format_t);
    ret->expr = expr;
    ret->len = len;
    ret->next = NULL;
    return ret;
}

void delete_output_format(output_format_t *format)
{
    if (!format) {
        return;
    }
    delete_expr(format->expr);
    delete_lit(format->len);
    delete_output_format(format->next);
    free(format);
}

param_decl_t *new_param_decl(ident_t *names, type_t *type)
{
    param_decl_t *ret = new(param_decl_t);
    ret->names = names;
    ret->type = type;
    ret->next = NULL;
    return ret;
}

void delete_param_decl(param_decl_t *params)
{
    if (!params) {
        return;
    }
    delete_ident(params->names);
    delete_type(params->type);
    delete_param_decl(params->next);
    free(params);
}

static decl_part_t *new_decl_part(decl_part_kind_t kind)
{
    decl_part_t *ret = new(decl_part_t);
    ret->kind = kind;
    ret->next = NULL;
    return ret;
}

variable_decl_t *new_variable_decl(ident_t *names, type_t *type)
{
    variable_decl_t *ret = new(variable_decl_t);
    ret->names = names;
    ret->type = type;
    ret->next = NULL;
    return ret;
}

void delete_variable_decl(variable_decl_t *decl)
{
    if (!decl) {
        return;
    }
    delete_ident(decl->names);
    delete_type(decl->type);
    delete_variable_decl(decl->next);
    free(decl);
}

decl_part_t *new_variable_decl_part(variable_decl_t *decls)
{
    decl_part_t *ret = new_decl_part(DECL_PART_VARIABLE);
    ret->u.variable_decl_part.decls = decls;
    if (!decls) {
        delete_decl_part(ret);
        return NULL;
    }
    return ret;
}

decl_part_t *new_procedure_decl_part(ident_t *name, param_decl_t *params, decl_part_t *variables, stmt_t *stmt)
{
    decl_part_t *ret = new_decl_part(DECL_PART_PROCEDURE);
    ret->u.procedure_decl_part.name = name;
    ret->u.procedure_decl_part.params = params;
    ret->u.procedure_decl_part.variables = variables;
    ret->u.procedure_decl_part.stmt = stmt;
    return ret;
}

void delete_decl_part(decl_part_t *decl)
{
    decl_part_t *cur;
    if (!decl) {
        return;
    }
    switch (decl->kind) {
    case DECL_PART_VARIABLE:
        delete_variable_decl(decl->u.variable_decl_part.decls);
        break;
    case DECL_PART_PROCEDURE:
        delete_ident(decl->u.procedure_decl_part.name);
        delete_param_decl(decl->u.procedure_decl_part.params);
        delete_stmt(decl->u.procedure_decl_part.stmt);
        delete_decl_part(decl->u.procedure_decl_part.variables);
        break;
    }
    delete_decl_part(decl->next);
    free(decl);
}

program_t *new_program(ident_t *name, decl_part_t *decl_part, stmt_t *stmt)
{
    program_t *ret = new(program_t);
    ret->name = name;
    ret->decl_part = decl_part;
    ret->stmt = stmt;
    return ret;
}

void delete_program(program_t *program)
{
    if (!program) {
        return;
    }
    delete_ident(program->name);
    delete_decl_part(program->decl_part);
    delete_stmt(program->stmt);
    free(program);
}

ast_t *new_ast(program_t *program, const source_t *source)
{
    ast_t *ret = new(ast_t);
    ret->program = program;
    ret->source = source;
    return ret;
}

void delete_ast(ast_t *ast)
{
    if (!ast) {
        return;
    }
    delete_program(ast->program);
    free(ast);
}
