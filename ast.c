#include <assert.h>
#include <stdlib.h>

#include "mppl.h"

static ast_lit_t *new_lit(ast_lit_kind_t kind, region_t region)
{
    ast_lit_t *ret = new(ast_lit_t);
    ret->kind = kind;
    ret->region = region;
    return ret;
}

ast_lit_t *new_ast_number_lit(symbol_t symbol, unsigned long value, region_t region)
{
    ast_lit_t *ret = new_lit(AST_LIT_NUMBER, region);
    ret->u.number_lit.symbol = symbol;
    ret->u.number_lit.value = value;
    return ret;
}

ast_lit_t *new_ast_boolean_lit(int value, region_t region)
{
    ast_lit_t *ret = new_lit(AST_LIT_BOOLEAN, region);
    ret->u.boolean_lit.value = value;
    return ret;
}

ast_lit_t *new_ast_string_lit(symbol_t symbol, size_t str_len, region_t region)
{
    ast_lit_t *ret = new_lit(AST_LIT_STRING, region);
    ret->u.string_lit.symbol = symbol;
    ret->u.string_lit.str_len = str_len;
    return ret;
}

void delete_ast_lit(ast_lit_t *lit)
{
    if (!lit) {
        return;
    }
    free(lit);
}

static ast_type_t *new_ast_type(ast_type_kind_t kind, region_t region)
{
    ast_type_t *ret = new(ast_type_t);
    ret->kind = kind;
    ret->region = region;
    return ret;
}

ast_type_t *new_ast_std_type(ast_type_kind_t kind, region_t region)
{
    ast_type_t *ret = new_ast_type(kind, region);
    ret->region = region;
    return ret;
}

ast_type_t *new_ast_array_type(ast_type_t *base, ast_lit_t *size, region_t region)
{
    ast_type_t *ret = new_ast_type(AST_TYPE_ARRAY, region);
    ret->u.array_type.base = base;
    ret->u.array_type.size = size;
    return ret;
}

void delete_ast_type(ast_type_t *type)
{
    if (!type) {
        return;
    }
    if (type->kind == AST_TYPE_ARRAY) {
        delete_ast_type(type->u.array_type.base);
        delete_ast_lit(type->u.array_type.size);
    }
    free(type);
}

ast_ident_t *new_ast_ident(symbol_t symbol, region_t region)
{
    ast_ident_t *ret = new(ast_ident_t);
    ret->symbol = symbol;
    ret->region = region;
    ret->next = NULL;
    return ret;
}

void delete_ast_ident(ast_ident_t *ident)
{
    if (!ident) {
        return;
    }
    delete_ast_ident(ident->next);
    free(ident);
}

static ast_expr_t *new_expr(ast_expr_kind_t kind, region_t region)
{
    ast_expr_t *ret = new(ast_expr_t);
    ret->kind = kind;
    ret->region = region;
    ret->next = NULL;
    return ret;
}

const char *ast_binop_str(ast_binary_op_kind_t kind)
{
    switch (kind) {
    case AST_BINARY_OP_STAR:
        return "*";
    case AST_BINARY_OP_DIV:
        return "div";
    case AST_BINARY_OP_AND:
        return "and";
    case AST_BINARY_OP_PLUS:
        return "+";
    case AST_BINARY_OP_MINUS:
        return "-";
    case AST_BINARY_OP_OR:
        return "or";
    case AST_BINARY_OP_EQUAL:
        return "=";
    case AST_BINARY_OP_NOTEQ:
        return "<>";
    case AST_BINARY_OP_LE:
        return "<";
    case AST_BINARY_OP_LEEQ:
        return "<=";
    case AST_BINARY_OP_GR:
        return ">";
    case AST_BINARY_OP_GREQ:
        return ">=";
    default:
        unreachable();
    }
}

ast_expr_t *new_ast_binary_expr(ast_binary_op_kind_t kind, ast_expr_t *lhs, ast_expr_t *rhs, region_t region)
{
    ast_expr_t *ret = new_expr(AST_EXPR_BINARY_OP, region);
    ret->u.binary_expr.kind = kind;
    ret->u.binary_expr.lhs = lhs;
    ret->u.binary_expr.rhs = rhs;
    return ret;
}

ast_expr_t *new_ast_unary_expr(ast_unary_op_kind_t kind, ast_expr_t *expr, region_t region)
{
    ast_expr_t *ret = new_expr(AST_EXPR_UNARY_OP, region);
    ret->u.unary_expr.kind = kind;
    ret->u.unary_expr.expr = expr;
    return ret;
}

ast_expr_t *new_ast_paren_expr(ast_expr_t *expr, region_t region)
{
    ast_expr_t *ret = new_expr(AST_EXPR_PAREN, region);
    ret->u.paren_expr.expr = expr;
    return ret;
}

ast_expr_t *new_ast_cast_expr(ast_type_t *type, ast_expr_t *expr, region_t region)
{
    ast_expr_t *ret = new_expr(AST_EXPR_CAST, region);
    ret->u.cast_expr.type = type;
    ret->u.cast_expr.expr = expr;
    return ret;
}

ast_expr_t *new_ast_decl_ref_expr(ast_ident_t *decl, region_t region)
{
    ast_expr_t *ret = new_expr(AST_EXPR_DECL_REF, region);
    ret->u.decl_ref_expr.decl = decl;
    return ret;
}

ast_expr_t *new_ast_array_subscript_expr(ast_ident_t *decl, ast_expr_t *expr, region_t region)
{
    ast_expr_t *ret = new_expr(AST_EXPR_ARRAY_SUBSCRIPT, region);
    ret->u.array_subscript_expr.decl = decl;
    ret->u.array_subscript_expr.expr = expr;
    return ret;
}

ast_expr_t *new_ast_constant_expr(ast_lit_t *lit, region_t region)
{
    ast_expr_t *ret = new_expr(AST_EXPR_CONSTANT, region);
    ret->u.constant_expr.lit = lit;
    return ret;
}

ast_expr_t *new_ast_empty_expr(region_t region)
{
    return new_expr(AST_EXPR_EMPTY, region);
}

void delete_ast_expr(ast_expr_t *expr)
{
    if (!expr) {
        return;
    }
    switch (expr->kind) {
    case AST_EXPR_BINARY_OP:
        delete_ast_expr(expr->u.binary_expr.lhs);
        delete_ast_expr(expr->u.binary_expr.rhs);
        break;
    case AST_EXPR_UNARY_OP:
        delete_ast_expr(expr->u.unary_expr.expr);
        break;
    case AST_EXPR_PAREN:
        delete_ast_expr(expr->u.paren_expr.expr);
        break;
    case AST_EXPR_CAST:
        delete_ast_type(expr->u.cast_expr.type);
        delete_ast_expr(expr->u.cast_expr.expr);
        break;
    case AST_EXPR_DECL_REF:
        delete_ast_ident(expr->u.decl_ref_expr.decl);
        break;
    case AST_EXPR_ARRAY_SUBSCRIPT:
        delete_ast_ident(expr->u.array_subscript_expr.decl);
        delete_ast_expr(expr->u.array_subscript_expr.expr);
        break;
    case AST_EXPR_CONSTANT:
        delete_ast_lit(expr->u.constant_expr.lit);
        break;
    }
    delete_ast_expr(expr->next);
    free(expr);
}

static ast_stmt_t *new_stmt(ast_stmt_kind_t kind)
{
    ast_stmt_t *ret = new(ast_stmt_t);
    ret->kind = kind;
    ret->next = NULL;
    return ret;
}

ast_stmt_t *new_ast_assign_stmt(ast_expr_t *lhs, ast_expr_t *rhs)
{
    ast_stmt_t *ret = new_stmt(AST_STMT_ASSIGN);
    ret->u.assign_stmt.lhs = lhs;
    ret->u.assign_stmt.rhs = rhs;
    return ret;
}

ast_stmt_t *new_ast_if_stmt(ast_expr_t *cond, ast_stmt_t *then_stmt, ast_stmt_t *else_stmt)
{
    ast_stmt_t *ret = new_stmt(AST_STMT_IF);
    ret->u.if_stmt.cond = cond;
    ret->u.if_stmt.then_stmt = then_stmt;
    ret->u.if_stmt.else_stmt = else_stmt;
    return ret;
}

ast_stmt_t *new_ast_while_stmt(ast_expr_t *cond, ast_stmt_t *do_stmt)
{
    ast_stmt_t *ret = new_stmt(AST_STMT_WHILE);
    ret->u.while_stmt.cond = cond;
    ret->u.while_stmt.do_stmt = do_stmt;
    return ret;
}

ast_stmt_t *new_ast_break_stmt()
{
    return new_stmt(AST_STMT_BREAK);
}

ast_stmt_t *new_ast_call_stmt(ast_ident_t *name, ast_expr_t *args)
{
    ast_stmt_t *ret = new_stmt(AST_STMT_CALL);
    ret->u.call_stmt.name = name;
    ret->u.call_stmt.args = args;
    return ret;
}

ast_stmt_t *new_ast_return_stmt()
{
    return new_stmt(AST_STMT_RETURN);
}

ast_stmt_t *new_ast_read_stmt(int newline, ast_expr_t *args)
{
    ast_stmt_t *ret = new_stmt(AST_STMT_READ);
    ret->u.read_stmt.newline = newline;
    ret->u.read_stmt.args = args;
    return ret;
}

ast_stmt_t *new_ast_write_stmt(int newline, ast_output_format_t *formats)
{
    ast_stmt_t *ret = new_stmt(AST_STMT_WRITE);
    ret->u.write_stmt.newline = newline;
    ret->u.write_stmt.formats = formats;
    return ret;
}

ast_stmt_t *new_ast_compound_stmt(ast_stmt_t *stmts)
{
    ast_stmt_t *ret = new_stmt(AST_STMT_COMPOUND);
    ret->u.compound_stmt.stmts = stmts;
    return ret;
}

ast_stmt_t *new_ast_empty_stmt()
{
    return new_stmt(AST_STMT_EMPTY);
}

void delete_ast_stmt(ast_stmt_t *stmt)
{
    if (!stmt) {
        return;
    }
    switch (stmt->kind) {
    case AST_STMT_ASSIGN:
        delete_ast_expr(stmt->u.assign_stmt.lhs);
        delete_ast_expr(stmt->u.assign_stmt.rhs);
        break;
    case AST_STMT_IF:
        delete_ast_expr(stmt->u.if_stmt.cond);
        delete_ast_stmt(stmt->u.if_stmt.then_stmt);
        delete_ast_stmt(stmt->u.if_stmt.else_stmt);
        break;
    case AST_STMT_WHILE:
        delete_ast_expr(stmt->u.while_stmt.cond);
        delete_ast_stmt(stmt->u.while_stmt.do_stmt);
        break;
    case AST_STMT_CALL:
        delete_ast_ident(stmt->u.call_stmt.name);
        delete_ast_expr(stmt->u.call_stmt.args);
        break;
    case AST_STMT_READ:
        delete_ast_expr(stmt->u.read_stmt.args);
        break;
    case AST_STMT_WRITE:
        delete_ast_output_format(stmt->u.write_stmt.formats);
        break;
    case AST_STMT_COMPOUND:
        delete_ast_stmt(stmt->u.compound_stmt.stmts);
        break;
    }
    delete_ast_stmt(stmt->next);
    free(stmt);
}

ast_output_format_t *new_ast_output_format(ast_expr_t *expr, ast_lit_t *len)
{
    ast_output_format_t *ret = new(ast_output_format_t);
    ret->expr = expr;
    ret->len = len;
    ret->next = NULL;
    return ret;
}

void delete_ast_output_format(ast_output_format_t *format)
{
    if (!format) {
        return;
    }
    delete_ast_expr(format->expr);
    delete_ast_lit(format->len);
    delete_ast_output_format(format->next);
    free(format);
}

ast_param_decl_t *new_ast_param_decl(ast_ident_t *names, ast_type_t *type)
{
    ast_param_decl_t *ret = new(ast_param_decl_t);
    ret->names = names;
    ret->type = type;
    ret->next = NULL;
    return ret;
}

void delete_ast_param_decl(ast_param_decl_t *params)
{
    if (!params) {
        return;
    }
    delete_ast_ident(params->names);
    delete_ast_type(params->type);
    delete_ast_param_decl(params->next);
    free(params);
}

static ast_decl_part_t *new_decl_part(ast_decl_part_kind_t kind)
{
    ast_decl_part_t *ret = new(ast_decl_part_t);
    ret->kind = kind;
    ret->next = NULL;
    return ret;
}

ast_variable_decl_t *new_ast_variable_decl(ast_ident_t *names, ast_type_t *type)
{
    ast_variable_decl_t *ret = new(ast_variable_decl_t);
    ret->names = names;
    ret->type = type;
    ret->next = NULL;
    return ret;
}

void delete_ast_variable_decl(ast_variable_decl_t *decl)
{
    if (!decl) {
        return;
    }
    delete_ast_ident(decl->names);
    delete_ast_type(decl->type);
    delete_ast_variable_decl(decl->next);
    free(decl);
}

ast_decl_part_t *new_variable_decl_part(ast_variable_decl_t *decls)
{
    ast_decl_part_t *ret = new_decl_part(AST_DECL_PART_VARIABLE);
    ret->u.variable_decl_part.decls = decls;
    if (!decls) {
        delete_decl_part(ret);
        return NULL;
    }
    return ret;
}

ast_decl_part_t *new_procedure_decl_part(ast_ident_t *name, ast_param_decl_t *params, ast_decl_part_t *variables, ast_stmt_t *stmt)
{
    ast_decl_part_t *ret = new_decl_part(AST_DECL_PART_PROCEDURE);
    ret->u.procedure_decl_part.name = name;
    ret->u.procedure_decl_part.params = params;
    ret->u.procedure_decl_part.variables = variables;
    ret->u.procedure_decl_part.stmt = stmt;
    return ret;
}

void delete_decl_part(ast_decl_part_t *decl)
{
    ast_decl_part_t *cur;
    if (!decl) {
        return;
    }
    switch (decl->kind) {
    case AST_DECL_PART_VARIABLE:
        delete_ast_variable_decl(decl->u.variable_decl_part.decls);
        break;
    case AST_DECL_PART_PROCEDURE:
        delete_ast_ident(decl->u.procedure_decl_part.name);
        delete_ast_param_decl(decl->u.procedure_decl_part.params);
        delete_ast_stmt(decl->u.procedure_decl_part.stmt);
        delete_decl_part(decl->u.procedure_decl_part.variables);
        break;
    }
    delete_decl_part(decl->next);
    free(decl);
}

ast_program_t *new_program(ast_ident_t *name, ast_decl_part_t *decl_part, ast_stmt_t *stmt)
{
    ast_program_t *ret = new(ast_program_t);
    ret->name = name;
    ret->decl_part = decl_part;
    ret->stmt = stmt;
    return ret;
}

void delete_program(ast_program_t *program)
{
    if (!program) {
        return;
    }
    delete_ast_ident(program->name);
    delete_decl_part(program->decl_part);
    delete_ast_stmt(program->stmt);
    free(program);
}

ast_t *new_ast(ast_program_t *program, symbol_storage_t *storage, const source_t *source)
{
    ast_t *ret = new(ast_t);
    ret->program = program;
    ret->storage = storage;
    ret->source = source;
    return ret;
}

void delete_ast(ast_t *ast)
{
    if (!ast) {
        return;
    }
    delete_program(ast->program);
    delete_symbol_storage(ast->storage);
    free(ast);
}
