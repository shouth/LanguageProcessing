#include <assert.h>
#include <stdarg.h>
#include <stdio.h>

#include "mppl.h"

typedef size_t codegen_addr_t;

typedef struct {
    FILE *file;
    size_t stmt_cnt;
    struct {
        codegen_addr_t cnt;
        hash_table_t *table;
    } addr;
} codegen_t;

codegen_addr_t codegen_addr_lookup(codegen_t *codegen, const void *ptr)
{
    const hash_table_entry_t *entry;
    assert(codegen);
    if (entry = hash_table_find(codegen->addr.table, ptr)) {
        return (codegen_addr_t) entry->value;
    } else {
        return 0;
    }
}

codegen_addr_t codegen_addr_for(codegen_t *codegen, const void *ptr)
{
    const hash_table_entry_t *entry;
    assert(codegen);
    if (entry = hash_table_find(codegen->addr.table, ptr)) {
        return (codegen_addr_t) entry->value;
    } else {
        codegen_addr_t addr = codegen->addr.cnt++;
        hash_table_insert_unchecked(codegen->addr.table, (void *) ptr, (void *) addr);
        return addr;
    }
}

const char *codegen_item_label(codegen_t *codegen, const ir_item_t *item)
{
    codegen_addr_t addr = codegen_addr_for(codegen, item);
    static char buf[16];
    sprintf(buf, "L%ld", addr);
    return buf;
}

const char *codegen_tmp_label(codegen_t *codegen)
{
    static char buf[16];
    codegen->addr.cnt++;
    sprintf(buf, "L%ld", codegen->addr.cnt);
    return buf;
}

void codegen_constant(codegen_t *codegen, const ir_constant_t *constant)
{
    assert(codegen && constant);

    while (constant) {
        codegen_addr_t addr = codegen_addr_for(codegen, constant);
        /* コード生成 */
        constant = constant->next;
    }
}

void codegen_load(codegen_t *codegen, const char *reg, const ir_operand_t *operand)
{
    assert(codegen && reg && operand);

    switch (operand->kind) {
    case IR_OPERAND_CONSTANT: {
        const ir_constant_t *constant = operand->u.constant_operand.constant;
        switch (constant->kind) {
        case IR_CONSTANT_CHAR:
            fprintf(codegen->file, "\tLAD\t%s,%d\n", reg, constant->u.char_constant.value);
            break;
        case IR_CONSTANT_NUMBER:
            fprintf(codegen->file, "\tLAD\t%s,%ld\n", reg, constant->u.number_constant.value);
            break;
        case IR_CONSTANT_BOOLEAN:
            fprintf(codegen->file, "\tLAD\t%s,%d\n", reg, constant->u.boolean_constant.value);
            break;
        default:
            unreachable();
        }
        break;
    }
    case IR_OPERAND_PLACE: {
        const ir_place_t *place = operand->u.place_operand.place;
        const ir_local_t *local = place->local;
        if (place->place_access) {
            switch (place->place_access->kind) {
            case IR_PLACE_ACCESS_INDEX:
                codegen_load(codegen, "GR8", place->place_access->u.index_place_access.index);
                switch (local->kind) {
                case IR_LOCAL_VAR:
                    fprintf(codegen->file, "\tLD\t%s,%s,GR8\n", reg, codegen_item_label(codegen, local->u.var.item));
                    break;
                default:
                    unreachable();
                }
                break;
            default:
                unreachable();
            }
        } else {
            switch (local->kind) {
            case IR_LOCAL_VAR:
                fprintf(codegen->file, "\tLD\t%s,%s\n", reg, codegen_item_label(codegen, local->u.var.item));
                break;
            case IR_LOCAL_ARG:
                fprintf(codegen->file, "\tLAD\tGR8,%s\n", codegen_item_label(codegen, local->u.arg.item));
                fprintf(codegen->file, "\tLD\t%s,0,GR8\n", reg);
                break;
            case IR_LOCAL_TEMP:
                fprintf(codegen->file, "\tPOP\t%s\n", reg);
                break;
            default:
                unreachable();
            }
        }
        break;
    }
    default:
        unreachable();
    }
}

void codegen_store(codegen_t *codegen, const char *reg, const ir_place_t *place)
{
    const ir_local_t *local;
    assert(codegen && reg && place);

    local = place->local;
    if (place->place_access) {
        switch (local->kind) {
        case IR_LOCAL_VAR:
            codegen_load(codegen, "GR8", place->place_access->u.index_place_access.index);
            fprintf(codegen->file, "\tST\t%s,%s,GR8\n", reg, codegen_item_label(codegen, local->u.var.item));
            break;
        default:
            unreachable();
        }
    } else {
        switch (local->kind) {
        case IR_LOCAL_VAR:
            fprintf(codegen->file, "\tST\t%s,%s\n", reg, codegen_item_label(codegen, local->u.var.item));
            break;
        case IR_LOCAL_ARG:
            fprintf(codegen->file, "\tLAD\tGR8,%s\n", codegen_item_label(codegen, local->u.arg.item));
            fprintf(codegen->file, "\tST\t%s,0,GR8\n", reg);
            break;
        case IR_LOCAL_TEMP:
            fprintf(codegen->file, "\tPUSH\t0,%s\n", reg);
            break;
        default:
            unreachable();
        }
    }
}

void codegen_assign_stmt(codegen_t *codegen, const ir_assign_stmt_t *stmt)
{
    assert(codegen && stmt);

    switch (stmt->rhs->kind) {
    case IR_RVALUE_USE:
        codegen_load(codegen, "GR1", stmt->rhs->u.use_rvalue.operand);
        break;
    case IR_RVALUE_BINARY_OP:
        codegen_load(codegen, "GR1", stmt->rhs->u.binary_op_rvalue.lhs);
        codegen_load(codegen, "GR2", stmt->rhs->u.binary_op_rvalue.rhs);
        switch (stmt->rhs->u.binary_op_rvalue.kind) {
        case AST_BINARY_OP_PLUS:
            fprintf(codegen->file, "\tADDA\tGR1,GR2\n");
            break;
        case AST_BINARY_OP_MINUS:
            fprintf(codegen->file, "\tSUBA\tGR1,GR2\n");
            break;
        case AST_BINARY_OP_STAR:
            fprintf(codegen->file, "\tMULA\tGR1,GR2\n");
            break;
        case AST_BINARY_OP_DIV:
            fprintf(codegen->file, "\tDIVA\tGR1,GR2\n");
            break;
        case AST_BINARY_OP_AND:
            fprintf(codegen->file, "\tAND\tGR1,GR2\n");
            break;
        case AST_BINARY_OP_OR:
            fprintf(codegen->file, "\tOR\tGR1,GR2\n");
            break;
        default: {
            fprintf(codegen->file, "\tCPA\tGR1,GR2\n");
            switch (stmt->rhs->u.binary_op_rvalue.kind) {
            case AST_BINARY_OP_EQUAL: {
                const char *jmp = codegen_tmp_label(codegen);
                fprintf(codegen->file, "\tCPA\tGR1,GR2\n");
                fprintf(codegen->file, "\tLAD\tGR1,1\n");
                fprintf(codegen->file, "\tJZE\t%s\n", jmp);
                fprintf(codegen->file, "\tLAD\tGR1,0\n");
                fprintf(codegen->file, "%s\n", jmp);
                break;
            }
            case AST_BINARY_OP_NOTEQ: {
                const char *jmp = codegen_tmp_label(codegen);
                fprintf(codegen->file, "\tCPA\tGR1,GR2\n");
                fprintf(codegen->file, "\tLAD\tGR1,1\n");
                fprintf(codegen->file, "\tJNZ\t%s\n", jmp);
                fprintf(codegen->file, "\tLAD\tGR1,0\n");
                fprintf(codegen->file, "%s\n", jmp);
                break;
            }
            case AST_BINARY_OP_LE: {
                const char *jmp = codegen_tmp_label(codegen);
                fprintf(codegen->file, "\tCPA\tGR1,GR2\n");
                fprintf(codegen->file, "\tLAD\tGR1,1\n");
                fprintf(codegen->file, "\tJMI\t%s\n", jmp);
                fprintf(codegen->file, "\tLAD\tGR1,0\n");
                fprintf(codegen->file, "%s\n", jmp);
                break;
            }
            case AST_BINARY_OP_LEEQ: {
                const char *jmp = codegen_tmp_label(codegen);
                fprintf(codegen->file, "\tCPA\tGR2,GR1\n");
                fprintf(codegen->file, "\tLAD\tGR1,0\n");
                fprintf(codegen->file, "\tJMI\t%s\n", jmp);
                fprintf(codegen->file, "\tLAD\tGR1,1\n");
                fprintf(codegen->file, "%s\n", jmp);
                break;
            }
            case AST_BINARY_OP_GR: {
                const char *jmp = codegen_tmp_label(codegen);
                fprintf(codegen->file, "\tCPA\tGR2,GR1\n");
                fprintf(codegen->file, "\tLAD\tGR1,1\n");
                fprintf(codegen->file, "\tJMI\t%s\n", jmp);
                fprintf(codegen->file, "\tLAD\tGR1,0\n");
                fprintf(codegen->file, "%s\n", jmp);
                break;
            }
            case AST_BINARY_OP_GREQ: {
                const char *jmp = codegen_tmp_label(codegen);
                fprintf(codegen->file, "\tCPA\tGR1,GR2\n");
                fprintf(codegen->file, "\tLAD\tGR1,0\n");
                fprintf(codegen->file, "\tJMI\t%s\n", jmp);
                fprintf(codegen->file, "\tLAD\tGR1,1\n");
                fprintf(codegen->file, "%s\n", jmp);
                break;
            }
            default:
                unreachable();
            }
        }
        }
        break;
    case IR_RVALUE_UNARY_OP:
        switch (stmt->rhs->u.unary_op_rvalue.kind) {
        case AST_UNARY_OP_NOT:
            codegen_load(codegen, "GR1", stmt->rhs->u.unary_op_rvalue.value);
            fprintf(codegen->file, "\tLAD\tGR2,1\n");
            fprintf(codegen->file, "\tXOR\tGR1,GR2");
            break;
        default:
            unreachable();
        }
        break;
    case IR_RVALUE_CAST: {
        ir_type_kind_t lhs_type = stmt->rhs->u.cast_rvalue.type->kind;
        ir_type_kind_t rhs_type = ir_operand_type(stmt->rhs->u.cast_rvalue.value)->kind;

        codegen_load(codegen, "GR1", stmt->rhs->u.cast_rvalue.value);
        switch (rhs_type) {
        case IR_TYPE_INTEGER:
            switch (lhs_type) {
            case IR_TYPE_BOOLEAN: {
                const char *jmp = codegen_tmp_label(codegen);
                fprintf(codegen->file, "\tLAD\tGR2,0\n");
                fprintf(codegen->file, "\tCMP\tGR1,GR2");
                fprintf(codegen->file, "\tJZE\t%s\n", jmp);
                fprintf(codegen->file, "\tLAD\tGR1,1\n");
                fprintf(codegen->file, "%s\n", jmp);
                break;
            }
            case IR_TYPE_CHAR:
                fprintf(codegen->file, "\tLAD\tGR2,127\n");
                fprintf(codegen->file, "\tAND\tGR1,GR2\n");
                break;
            case IR_TYPE_INTEGER:
                /* do nothing */
                break;
            default:
                unreachable();
            }
            break;
        case IR_TYPE_CHAR:
            switch (lhs_type) {
            case IR_TYPE_BOOLEAN: {
                const char *jmp = codegen_tmp_label(codegen);
                fprintf(codegen->file, "\tLAD\tGR2,0\n");
                fprintf(codegen->file, "\tCMP\tGR1,GR2");
                fprintf(codegen->file, "\tJZE\t%s\n", jmp);
                fprintf(codegen->file, "\tLAD\tGR1,1\n");
                fprintf(codegen->file, "%s\n", jmp);
                break;
            }
            case IR_TYPE_INTEGER:
            case IR_TYPE_CHAR:
                /* do nothing */
                break;
            default:
                unreachable();
            }
            break;
        case IR_TYPE_BOOLEAN:
            /* do nothing */
            break;
        default:
            unreachable();
        }
        break;
    }
    }
    codegen_store(codegen, "GR1", stmt->lhs);
}

void codegen_call_stmt(codegen_t *codegen, const ir_call_stmt_t *stmt)
{
    assert(codegen && stmt);
    assert(!stmt->func->place_access);

    switch (stmt->func->local->kind) {
    case IR_LOCAL_VAR:
        fprintf(codegen->file, "\tCALL\t%s\n", codegen_item_label(codegen, stmt->func->local->u.var.item));
        break;
    default:
        unreachable();
    }
}

void codegen_read_stmt(codegen_t *codegen, const ir_read_stmt_t *stmt)
{

}

void codegen_write_stmt(codegen_t *codegen, const ir_write_stmt_t *stmt)
{

}

void codegen_stmt(codegen_t *codegen, const ir_stmt_t *stmt)
{
    assert(codegen);

    while (stmt) {
        switch (stmt->kind) {
        case IR_STMT_ASSIGN:
            codegen_assign_stmt(codegen, &stmt->u.assign_stmt);
            break;
        case IR_STMT_CALL:
            codegen_call_stmt(codegen, &stmt->u.call_stmt);
            break;
        case IR_STMT_READ:
            codegen_read_stmt(codegen, &stmt->u.read_stmt);
            break;
        case IR_STMT_WRITE:
            codegen_write_stmt(codegen, &stmt->u.write_stmt);
            break;
        }
        stmt = stmt->next;
        codegen->stmt_cnt++;
    }
}

void codegen_block(codegen_t *codegen, const ir_block_t *block)
{
    codegen_addr_t addr;
    assert(codegen && block);

    addr = codegen_addr_for(codegen, block);
    /* コード生成 */
    codegen->stmt_cnt = 0;
    codegen_stmt(codegen, block->stmt);
    switch (block->termn.kind) {
    case IR_TERMN_GOTO: {
        const ir_block_t *next = block->termn.u.goto_termn.next;
        /* コード生成 */
        if (codegen_addr_lookup(codegen, next)) {
            codegen_addr_t jmp = codegen_addr_for(codegen, next);
        } else {
            codegen_block(codegen, next);
        }
        break;
    }
    case IR_TERMN_IF: {
        const ir_block_t *then = block->termn.u.if_termn.then;
        const ir_block_t *els = block->termn.u.if_termn.els;

        if (codegen_addr_lookup(codegen, then)) {
            codegen_addr_t then_jmp = codegen_addr_for(codegen, then);
            /* コード生成 */
            if (codegen_addr_lookup(codegen, els)) {
                codegen_addr_t els_jmp = codegen_addr_for(codegen, els);
                /* コード生成 */
            } else {
                codegen_block(codegen, els);
            }
        } else {
            if (codegen_addr_lookup(codegen, els)) {
                codegen_addr_t els_jmp = codegen_addr_for(codegen, els);
                /* コード生成 */
                codegen_block(codegen, then);
            } else {
                codegen_addr_t els_jmp = codegen_addr_for(codegen, els);
                /* コード生成 */
                codegen_block(codegen, then);
                codegen_block(codegen, els);
            }
        }
        break;
    }
    case IR_TERMN_RETURN:
        /* コード生成 */
        break;
    case IR_TERMN_ARG: {
        const ir_operand_t *arg = block->termn.u.arg_termn.arg;
        const ir_block_t *next = block->termn.u.arg_termn.next;

        switch (arg->kind) {
        case IR_OPERAND_CONSTANT: {
            const ir_constant_t *constant = arg->u.constant_operand.constant;
            const char *label = codegen_tmp_label(codegen);
            fprintf(codegen->file, "%s\n", label);
            switch (constant->kind) {
            case IR_CONSTANT_NUMBER:
                fprintf(codegen->file, "\tDC\t%ld\n", constant->u.number_constant.value);
                break;
            case IR_CONSTANT_CHAR:
                fprintf(codegen->file, "\tDC\t%d\n", constant->u.char_constant.value);
                break;
            case IR_CONSTANT_BOOLEAN:
                fprintf(codegen->file, "\tDC\t%d\n", constant->u.boolean_constant.value);
                break;
            default:
                unreachable();
            }
            fprintf(codegen->file, "\tPUSH\t%s\n", label);
            break;
        }
        case IR_OPERAND_PLACE: {
            ir_place_t *place = arg->u.place_operand.place;
            if (place->place_access) {
                switch (place->local->kind) {
                case IR_LOCAL_VAR:
                    codegen_load(codegen, "GR8", place->place_access->u.index_place_access.index);
                    fprintf(codegen->file, "\tPUSH\t%s,GR8\n", codegen_item_label(codegen, place->local->u.var.item));
                    break;
                default:
                    unreachable();
                }
            } else {
                switch (place->local->kind) {
                case IR_LOCAL_VAR:
                    fprintf(codegen->file, "\tPUSH\t%s\n", codegen_item_label(codegen, place->local->u.var.item));
                    break;
                case IR_LOCAL_ARG:
                    fprintf(codegen->file, "\tLD\tGR8,%s\n", codegen_item_label(codegen, place->local->u.arg.item));
                    fprintf(codegen->file, "\tPUSH\t0,GR8\n");
                    break;
                case IR_LOCAL_TEMP: {
                    const char *label = codegen_tmp_label(codegen);
                    fprintf(codegen->file, "%s\n", label);
                    fprintf(codegen->file, "\tDS\t1\n");
                    fprintf(codegen->file, "\tPOP\tGR1\n");
                    fprintf(codegen->file, "\tST\tGR1,%s\n", label);
                    fprintf(codegen->file, "\tPUSH\t%s\n", label);
                    break;
                }
                default:
                    unreachable();
                }
            }
            break;
        }
        default:
            unreachable();
        }

        codegen_block(codegen, next);
        break;
    }
    default:
        unreachable();
    }
}

void codegen_item(codegen_t *codegen, const ir_item_t *item);

void codegen_body(codegen_t *codegen, const ir_body_t *body)
{
    assert(codegen && body);

    codegen_item(codegen, body->items);
    codegen_block(codegen, body->inner);
}

void codegen_item(codegen_t *codegen, const ir_item_t *item)
{
    codegen_addr_t addr;
    assert(codegen && item);

    while (item) {
        switch (item->kind) {
        case IR_ITEM_PROGRAM:
            codegen_body(codegen, item->body);
            break;
        case IR_ITEM_PROCEDURE:
            codegen_body(codegen, item->body);
            break;
        case IR_ITEM_VAR:
            addr = codegen_addr_for(codegen, item);
            /* コード生成 */
            break;
        case IR_ITEM_ARG_VAR:
            addr = codegen_addr_for(codegen, item);
            /* コード生成 */
            break;
        case IR_ITEM_LOCAL_VAR:
            addr = codegen_addr_for(codegen, item);
            /* コード生成 */
            break;
        }
        item = item->next;
    }
}

void codegen_ir(codegen_t *codegen, const ir_t *ir)
{
    codegen_constant(codegen, ir->constants);
    codegen_item(codegen, ir->items);
}

void casl2_codegen(const ir_t *ir)
{
    codegen_t codegen;
    ir_item_t *item;
    assert(ir);

    codegen.file = fopen(ir->source->output_filename, "w");
    codegen.addr.cnt = 1;
    codegen.addr.table = new_hash_table(hash_table_default_comparator, hash_table_default_hasher);
    codegen_ir(&codegen, ir);
    delete_hash_table(codegen.addr.table, NULL, NULL);
    fclose(codegen.file);
}
