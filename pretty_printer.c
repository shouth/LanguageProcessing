#include <assert.h>
#include <stdio.h>
#include <stddef.h>
#include <stdint.h>

#include "pretty_printer.h"
#include "ast.h"

typedef struct {
    size_t indent;
} printer_t;

void pp_type(printer_t *printer, const type_t *type)
{
    assert(printer && type);
    switch (type->kind) {
    case TYPE_INTEGER:
        printf("integer");
        break;
    case TYPE_BOOLEAN:
        printf("boolean");
        break;
    case TYPE_CHAR:
        printf("char");
        break;
    case TYPE_ARRAY:
        printf("array[%ld] of ", type->array.len);
        pp_type(printer, type->array.base);
        printf(";");
        break;
    }
}

void pp_indent(printer_t *printer)
{
    size_t i;
    assert(printer);
    for (i = 0; i < printer->indent; i++) {
        printf("    ");
    }
}

void pp_ident(printer_t *printer, const ident_t *ident)
{
    const ident_t *cur;
    assert(printer && ident);

    printf("%.*s", (int) ident->len, ident->ptr);
    cur = ident->next;
    while (cur) {
        printf(", %.*s", (int) cur->len, cur->ptr);
        cur = cur->next;
    }
}

void pp_lit(printer_t *printer, const lit_t *lit)
{
    assert(printer && lit);
    switch (lit->kind) {
    case LIT_NUMBER:
        printf("%ld", lit->u.number_lit.value);
        break;
    case LIT_BOOLEAN:
        printf(lit->u.boolean_lit.value ? "true" : "false");
        break;
    case LIT_STRING:
        printf("'%.*s'", (int) lit->u.string_lit.len, lit->u.string_lit.ptr);
        break;
    }
}

void pp_expr(printer_t *printer, const expr_t *expr);

void pp_ref_expr(printer_t *printer, const ref_expr_t *expr)
{
    assert(printer && expr);
    pp_ident(printer, expr->name);
}

void pp_array_subscript_expr(printer_t *printer, const array_subscript_expr_t *expr)
{
    assert(printer && expr);
    pp_ident(printer, expr->name);
    printf("[");
    pp_expr(printer, expr->index_expr);
    printf("]");
}

void pp_constant_expr(printer_t *printer, const constant_expr_t *expr)
{
    assert(printer && expr);
    pp_lit(printer, expr->lit);
}

void pp_expr(printer_t *printer, const expr_t *expr)
{
    const expr_t *cur;
    assert(printer && expr);
    cur = expr;
    while (cur) {
        if (cur != expr) {
            printf(", ");
        }
        switch (cur->kind) {
        case EXPR_BINARY_OP:
            break;
        case EXPR_UNARY_OP:
            break;
        case EXPR_PAREN:
            break;
        case EXPR_CAST:
            break;
        case EXPR_REF:
            pp_ref_expr(printer, &cur->u.ref_expr);
            break;
        case EXPR_ARRAY_SUBSCRIPT:
            pp_array_subscript_expr(printer, &cur->u.array_subscript_expr);
            break;
        case EXPR_CONSTANT:
            pp_constant_expr(printer, &cur->u.constant_expr);
            break;
        case EXPR_EMPTY:
            break;
        }
        cur = cur->next;
    }
}

void pp_stmt(printer_t *printer, const stmt_t *stmt);

void pp_assign_stmt(printer_t *printer, const assign_stmt_t *stmt)
{
    pp_expr(printer, stmt->lhs);
    printf(" := ");
    pp_expr(printer, stmt->rhs);
}

void pp_if_stmt(printer_t *printer, const if_stmt_t *stmt)
{
    assert(printer && stmt);
    printf("if ");
    pp_expr(printer, stmt->cond);
    printf(" then\n");
    if (stmt->then_stmt->kind != STMT_COMPOUND) {
        printer->indent++;
        pp_indent(printer);
        pp_stmt(printer, stmt->then_stmt);
        printer->indent--;
    } else {
        pp_indent(printer);
        pp_stmt(printer, stmt->then_stmt);
    }
    if (stmt->else_stmt) {
        printf("\n");
        pp_indent(printer);
        printf("else");
        if (stmt->else_stmt->kind == STMT_IF) {
            printf(" ");
            pp_stmt(printer, stmt->else_stmt);
        } else if (stmt->else_stmt->kind != STMT_COMPOUND) {
            printf("\n");
            printer->indent++;
            pp_indent(printer);
            pp_stmt(printer, stmt->else_stmt);
            printer->indent--;
        } else {
            printf("\n");
            pp_indent(printer);
            pp_stmt(printer, stmt->else_stmt);
        }
    }
}

void pp_while_stmt(printer_t *printer, const while_stmt_t *stmt)
{
    assert(printer && stmt);
    printf("while ");
    pp_expr(printer, stmt->cond);
    printf(" do\n");
    if (stmt->do_stmt->kind != STMT_COMPOUND) {
        printer->indent++;
    }
    pp_indent(printer);
    pp_stmt(printer, stmt->do_stmt);
    if (stmt->do_stmt->kind != STMT_COMPOUND) {
        printer->indent--;
    }
}

void pp_call_stmt(printer_t *printer, const call_stmt_t *stmt)
{
    assert(printer && stmt);
    printf("call ");
    pp_ident(printer, stmt->name);
    if (stmt->args) {
        printf("(");
        pp_expr(printer, stmt->args);
        printf(")");
    }
}

void pp_read_stmt(printer_t *printer, const read_stmt_t *stmt)
{
    assert(printer && stmt);
    printf(stmt->newline ? "readln" : "read");
    if (stmt->args) {
        expr_t *cur = stmt->args;
        printf("(");
        pp_expr(printer, cur);
        printf(")");
    }
}

void pp_write_stmt(printer_t *printer, const write_stmt_t *stmt)
{
    assert(printer && stmt);
    printf(stmt->newline ? "writeln" : "write");
    if (stmt->formats) {
        output_format_t *cur = stmt->formats;
        printf("(");
        while (cur) {
            if (cur != stmt->formats) {
                printf(", ");
            }
            pp_expr(printer, cur->expr);
            if (cur->len != SIZE_MAX) {
                printf(" : %ld", cur->len);
            }
            cur = cur->next;
        }
        printf(")");
    }
}

void pp_compound_stmt(printer_t *printer, const compound_stmt_t *stmt)
{
    stmt_t *cur;
    assert(printer && stmt);
    printf("begin\n");
    printer->indent++;
    cur = stmt->stmts;
    while (cur) {
        if (cur != stmt->stmts) {
            printf(";\n");
        }
        pp_indent(printer);
        pp_stmt(printer, cur);
        cur = cur->next;
    }
    printer->indent--;
    printf("\n");
    pp_indent(printer);
    printf("end");
}

void pp_stmt(printer_t *printer, const stmt_t *stmt)
{
    assert(printer && stmt);
    switch (stmt->kind) {
    case STMT_ASSIGN:
        pp_assign_stmt(printer, &stmt->u.assign_stmt);
        break;
    case STMT_IF:
        pp_if_stmt(printer, &stmt->u.if_stmt);
        break;
    case STMT_WHILE:
        pp_while_stmt(printer, &stmt->u.while_stmt);
        break;
    case STMT_BREAK:
        printf("break");
        break;
    case STMT_CALL:
        pp_call_stmt(printer, &stmt->u.call_stmt);
        break;
    case STMT_RETURN:
        printf("return");
        break;
    case STMT_READ:
        pp_read_stmt(printer, &stmt->u.read_stmt);
        break;
    case STMT_WRITE:
        pp_write_stmt(printer, &stmt->u.write_stmt);
        break;
    case STMT_COMPOUND:
        pp_compound_stmt(printer, &stmt->u.compound_stmt);
        break;
    case STMT_EMPTY:
        break;
    }
}

void pp_decl_part(printer_t *printer, const decl_part_t *decl_part);

void pp_variable_decl_part(printer_t *printer, const variable_decl_part_t *decl_part)
{
    const variable_decl_t *cur;
    assert(printer && decl_part);

    cur = decl_part->decls;
    while (cur) {
        pp_indent(printer);
        printf(cur == decl_part->decls ? "var " : "    ");
        pp_ident(printer, cur->names);
        printf(" : ");
        pp_type(printer, cur->type);
        printf(";\n");
        cur = cur->next;
    }
}

void pp_procedure_decl_part(printer_t *printer, const procedure_decl_part_t *decl_part)
{
    assert(printer && decl_part);
    printf("procedure ");
    pp_ident(printer, decl_part->name);
    if (decl_part->params) {
        const params_t *cur;
        printf("(");
        cur = decl_part->params;
        while (cur) {
            if (cur != decl_part->params) {
                printf("; ");
            }
            pp_ident(printer, cur->names);
            printf(" : ");
            pp_type(printer, cur->type);
            cur = cur->next;
        }
        printf(")");
    }
    printf(";\n");
    if (decl_part->variables) {
        pp_decl_part(printer, decl_part->variables);
    }
    pp_stmt(printer, decl_part->stmt);
    printf(";\n\n");
}

void pp_decl_part(printer_t *printer, const decl_part_t *decl_part)
{
    const decl_part_t *cur;
    assert(printer && decl_part);

    cur = decl_part;
    while (cur) {
        switch (cur->kind) {
        case DECL_PART_VARIABLE:
            pp_variable_decl_part(printer, &cur->u.variable_decl_part);
            break;
        case DECL_PART_PROCEDURE:
            pp_procedure_decl_part(printer, &cur->u.procedure_decl_part);
            break;
        default:
            assert("wrong decl_part");
        }
        cur = cur->next;
    }
}

void pp_program(printer_t *printer, const program_t *program)
{
    assert(printer && program);
    printf("program ");
    pp_ident(printer, program->name);
    printf(";\n");
    pp_decl_part(printer, program->decl_part);
    assert(program->stmt->kind == STMT_COMPOUND);
    pp_stmt(printer, program->stmt);
    printf(".\n");
}

void pretty_print(const ast_t *ast)
{
    printer_t printer;
    assert(ast);
    printer.indent = 0;
    pp_program(&printer, ast->program);
}
