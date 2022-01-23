#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "mppl.h"

typedef size_t codegen_addr_t;

typedef struct {
    FILE *file;
    size_t stmt_cnt;
    struct {
        codegen_addr_t cnt;
        hash_table_t *table;
    } addr;
    char label[16];
    struct {
        int r_int, r_char, r_bool, r_ln;
        int w_int, w_char, w_bool, w_ln;
    } builtin;
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

const char *codegen_label_for(codegen_t *codegen, const void *ptr)
{
    codegen_addr_t addr = codegen_addr_for(codegen, ptr);
    static char buf[16];
    sprintf(buf, "L%ld", addr);
    return buf;
}

const char *codegen_tmp_label(codegen_t *codegen)
{
    static char buf[16];
    if (codegen->label[0]) {
        strcpy(buf, codegen->label);
    } else {
        codegen_addr_t addr = codegen->addr.cnt++;
        sprintf(buf, "L%ld", addr);
    }
    return buf;
}

void codegen_set_label(codegen_t *codegen, const char *label)
{ strcpy(codegen->label, label); }

void codegen_print(codegen_t *codegen, const char *instruction, const char *operand_format, ...)
{
    va_list arg;
    va_start(arg, operand_format);
    fprintf(codegen->file, "%-10s", codegen->label);
    if (operand_format) {
        fprintf(codegen->file, "%-8s", instruction);
        vfprintf(codegen->file, operand_format, arg);
    } else {
        fprintf(codegen->file, "%s", instruction);
    }
    fprintf(codegen->file, "\n");
    va_end(arg);
    codegen->label[0] = '\0';
}

void codegen_constant(codegen_t *codegen, const ir_constant_t *constant)
{
    assert(codegen && constant);

    while (constant) {
        codegen_set_label(codegen, codegen_label_for(codegen, constant));
        switch (constant->kind) {
        case IR_CONSTANT_STRING: {
            const symbol_instance_t *instance = symbol_get_instance(constant->u.string_constant.value);
            codegen_print(codegen, "DC", "\'%.*s\'", (int) instance->len, instance->ptr);
            break;
        }
        }
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
        case IR_CONSTANT_NUMBER:
            codegen_print(codegen, "LAD", "%s, %ld", reg, constant->u.number_constant.value);
            break;
        case IR_CONSTANT_CHAR:
            codegen_print(codegen, "LAD", "%s, %d", reg, constant->u.char_constant.value);
            break;
        case IR_CONSTANT_BOOLEAN:
            codegen_print(codegen, "LAD", "%s, %d", reg, constant->u.boolean_constant.value);
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
                codegen_load(codegen, "GR7", place->place_access->u.index_place_access.index);
                switch (local->kind) {
                case IR_LOCAL_VAR:
                    codegen_print(codegen, "LD", "%s, %s, GR7", reg, codegen_label_for(codegen, local->u.var.item));
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
                codegen_print(codegen, "LD", "%s, %s", reg, codegen_label_for(codegen, local->u.var.item));
                break;
            case IR_LOCAL_ARG:
                codegen_print(codegen, "LAD", "GR7, %s", codegen_label_for(codegen, local->u.arg.item));
                codegen_print(codegen, "LD", "%s, 0, GR7", reg);
                break;
            case IR_LOCAL_TEMP:
                codegen_print(codegen, "POP", "%s", reg);
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
            codegen_load(codegen, "GR7", place->place_access->u.index_place_access.index);
            codegen_print(codegen, "ST", "%s, %s, GR7", reg, codegen_label_for(codegen, local->u.var.item));
            break;
        default:
            unreachable();
        }
    } else {
        switch (local->kind) {
        case IR_LOCAL_VAR:
            codegen_print(codegen, "ST", "%s, %s", reg, codegen_label_for(codegen, local->u.var.item));
            break;
        case IR_LOCAL_ARG:
            codegen_print(codegen, "LAD", "GR7, %s", codegen_label_for(codegen, local->u.arg.item));
            codegen_print(codegen, "ST", "%s, 0, GR7", reg);
            break;
        case IR_LOCAL_TEMP:
            codegen_print(codegen, "PUSH", "0, %s", reg);
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
            codegen_print(codegen, "ADDA", "GR1, GR2");
            break;
        case AST_BINARY_OP_MINUS:
            codegen_print(codegen, "SUBA", "GR1, GR2");
            break;
        case AST_BINARY_OP_STAR:
            codegen_print(codegen, "MULA", "GR1, GR2");
            break;
        case AST_BINARY_OP_DIV:
            codegen_print(codegen, "DIVA", "GR1, GR2");
            break;
        case AST_BINARY_OP_AND:
            codegen_print(codegen, "AND", "GR1, GR2");
            break;
        case AST_BINARY_OP_OR:
            codegen_print(codegen, "OR", "GR1, GR2");
            break;
        default: {
            switch (stmt->rhs->u.binary_op_rvalue.kind) {
            case AST_BINARY_OP_EQUAL: {
                const char *jmp = codegen_tmp_label(codegen);
                codegen_print(codegen, "CPA", "GR1, GR2");
                codegen_print(codegen, "LAD", "GR1, 1");
                codegen_print(codegen, "JZE", "%s", jmp);
                codegen_print(codegen, "LAD", "GR1, 0");
                codegen_set_label(codegen, jmp);
                codegen_print(codegen, "NOP", NULL);
                break;
            }
            case AST_BINARY_OP_NOTEQ: {
                const char *jmp = codegen_tmp_label(codegen);
                codegen_print(codegen, "CPA", "GR1, GR2");
                codegen_print(codegen, "LAD", "GR1, 1");
                codegen_print(codegen, "JNZ", "%s", jmp);
                codegen_print(codegen, "LAD", "GR1, 0");
                codegen_set_label(codegen, jmp);
                codegen_print(codegen, "NOP", NULL);
                break;
            }
            case AST_BINARY_OP_LE: {
                const char *jmp = codegen_tmp_label(codegen);
                codegen_print(codegen, "CPA", "GR1, GR2");
                codegen_print(codegen, "LAD", "GR1, 1");
                codegen_print(codegen, "JMI", "%s", jmp);
                codegen_print(codegen, "LAD", "GR1, 0");
                codegen_set_label(codegen, jmp);
                codegen_print(codegen, "NOP", NULL);
                break;
            }
            case AST_BINARY_OP_LEEQ: {
                const char *jmp = codegen_tmp_label(codegen);
                codegen_print(codegen, "CPA", "GR2, GR1");
                codegen_print(codegen, "LAD", "GR1, 0");
                codegen_print(codegen, "JMI", "%s", jmp);
                codegen_print(codegen, "LAD", "GR1, 1");
                codegen_set_label(codegen, jmp);
                codegen_print(codegen, "NOP", NULL);
                break;
            }
            case AST_BINARY_OP_GR: {
                const char *jmp = codegen_tmp_label(codegen);
                codegen_print(codegen, "CPA", "GR2, GR1");
                codegen_print(codegen, "LAD", "GR1, 1");
                codegen_print(codegen, "JMI", "%s", jmp);
                codegen_print(codegen, "LAD", "GR1, 0");
                codegen_set_label(codegen, jmp);
                codegen_print(codegen, "NOP", NULL);
                break;
            }
            case AST_BINARY_OP_GREQ: {
                const char *jmp = codegen_tmp_label(codegen);
                codegen_print(codegen, "CPA", "GR1, GR2");
                codegen_print(codegen, "LAD", "GR1, 0");
                codegen_print(codegen, "JMI", "%s", jmp);
                codegen_print(codegen, "LAD", "GR1, 1");
                codegen_set_label(codegen, jmp);
                codegen_print(codegen, "NOP", NULL);
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
            codegen_print(codegen, "LAD", "GR2, 1");
            codegen_print(codegen, "XOR", "GR1, GR2");
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
                codegen_print(codegen, "LAD", "GR2, 0");
                codegen_print(codegen, "CPA", "GR1, GR2");
                codegen_print(codegen, "JZE", "%s", jmp);
                codegen_print(codegen, "LAD", "GR1, 1");
                codegen_set_label(codegen, jmp);
                codegen_print(codegen, "NOP", NULL);
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
        case IR_TYPE_CHAR:
            switch (lhs_type) {
            case IR_TYPE_BOOLEAN: {
                const char *jmp = codegen_tmp_label(codegen);
                codegen_print(codegen, "LAD", "GR2, 0");
                codegen_print(codegen, "CPA", "GR1, GR2");
                codegen_print(codegen, "JZE", "%s", jmp);
                codegen_print(codegen, "LAD", "GR1, 1");
                codegen_set_label(codegen, jmp);
                codegen_print(codegen, "NOP", NULL);
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
        codegen_print(codegen, "CALL", "%s", codegen_label_for(codegen, stmt->func->local->u.var.item->body->inner));
        break;
    default:
        unreachable();
    }
}

void codegen_push_constant_address(codegen_t *codegen, const ir_constant_t *constant)
{
    assert(codegen && constant);
    switch (constant->kind) {
    case IR_CONSTANT_NUMBER: {
        const char *label = codegen_tmp_label(codegen);
        codegen_set_label(codegen, label);
        codegen_print(codegen, "DC", "%ld", constant->u.number_constant.value);
        codegen_print(codegen, "PUSH", "%s", label);
        break;
    }
    case IR_CONSTANT_CHAR: {
        const char *label = codegen_tmp_label(codegen);
        codegen_set_label(codegen, label);
        codegen_print(codegen, "DC", "%d", constant->u.char_constant.value);
        codegen_print(codegen, "PUSH", "%s", label);
        break;
    }
    case IR_CONSTANT_BOOLEAN: {
        const char *label = codegen_tmp_label(codegen);
        codegen_set_label(codegen, label);
        codegen_print(codegen, "DC", "%d", constant->u.boolean_constant.value);
        codegen_print(codegen, "PUSH", "%s", label);
        break;
    }
    case IR_CONSTANT_STRING:
        codegen_print(codegen, "PUSH", codegen_label_for(codegen, constant));
        break;
    default:
        unreachable();
    }
}

void codegen_push_place_address(codegen_t *codegen, const ir_place_t *place)
{
    assert(codegen && place);

    if (place->place_access) {
        switch (place->local->kind) {
        case IR_LOCAL_VAR:
            codegen_load(codegen, "GR7", place->place_access->u.index_place_access.index);
            codegen_print(codegen, "PUSH", "%s, GR7", codegen_label_for(codegen, place->local->u.var.item));
            break;
        default:
            unreachable();
        }
    } else {
        switch (place->local->kind) {
        case IR_LOCAL_VAR:
            codegen_print(codegen, "PUSH", "%s", codegen_label_for(codegen, place->local->u.var.item));
            break;
        case IR_LOCAL_ARG:
            codegen_print(codegen, "LD", "GR7, %s", codegen_label_for(codegen, place->local->u.arg.item));
            codegen_print(codegen, "PUSH", "0, GR7");
            break;
        case IR_LOCAL_TEMP: {
            const char *label = codegen_tmp_label(codegen);
            codegen_set_label(codegen, label);
            codegen_print(codegen, "DS", "1");
            codegen_print(codegen, "POP", "GR1");
            codegen_print(codegen, "ST", "GR1, %s", label);
            codegen_print(codegen, "PUSH", "%s", label);
            break;
        }
        default:
            unreachable();
        }
    }
}

void codegen_push_operand_address(codegen_t *codegen, const ir_operand_t *operand)
{
    assert(codegen && operand);

    switch (operand->kind) {
    case IR_OPERAND_CONSTANT:
        codegen_push_constant_address(codegen, operand->u.constant_operand.constant);
        break;
    case IR_OPERAND_PLACE:
        codegen_push_place_address(codegen, operand->u.place_operand.place);
        break;
    default:
        unreachable();
    }
}

void codegen_read_stmt(codegen_t *codegen, const ir_read_stmt_t *stmt)
{
    assert(codegen && stmt);

    /* call builtin `read` functions for each types */
    switch (ir_place_type(stmt->ref)->kind) {
    case IR_TYPE_INTEGER:
        codegen->builtin.r_int++;
        codegen_push_place_address(codegen, stmt->ref);
        codegen_print(codegen, "CALL", "BRINT");
        break;
    case IR_TYPE_BOOLEAN:
        codegen->builtin.r_bool++;
        codegen_push_place_address(codegen, stmt->ref);
        codegen_print(codegen, "CALL", "BRBOOL");
        break;
    case IR_TYPE_CHAR:
        codegen->builtin.r_char++;
        codegen_push_place_address(codegen, stmt->ref);
        codegen_print(codegen, "CALL", "BRCHAR");
        break;
    default:
        unreachable();
    }
}

void codegen_write_stmt(codegen_t *codegen, const ir_write_stmt_t *stmt)
{
    assert(codegen && stmt);

    /* call builtin `write` functions for each types */
    switch (ir_operand_type(stmt->value)->kind) {
    case IR_TYPE_INTEGER:
        codegen->builtin.w_int++;
        codegen_push_operand_address(codegen, stmt->value);
        codegen_print(codegen, "CALL", "BWINT");
        break;
    case IR_TYPE_BOOLEAN:
        codegen->builtin.w_bool++;
        codegen_push_operand_address(codegen, stmt->value);
        codegen_print(codegen, "CALL", "BWBOOL");
        break;
    case IR_TYPE_CHAR:
        codegen->builtin.w_char++;
        codegen_push_operand_address(codegen, stmt->value);
        codegen_print(codegen, "CALL", "BWCHAR");
        break;
    case IR_TYPE_ARRAY: {
        const ir_constant_t *constant = stmt->value->u.constant_operand.constant;
        const ir_type_t *type = ir_constant_type(constant);
        const char *label = codegen_tmp_label(codegen);
        codegen_set_label(codegen, label);
        codegen_print(codegen, "DC", "%ld", type->u.array_type.size);
        codegen_print(codegen, "OUT", "%s, %s", codegen_label_for(codegen, constant), label);
        break;
    }
    default:
        unreachable();
    }
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
        case IR_STMT_READLN:
            codegen->builtin.r_ln++;
            codegen_print(codegen, "CALL", "BRLN");
            break;
        case IR_STMT_WRITE:
            codegen_write_stmt(codegen, &stmt->u.write_stmt);
            break;
        case IR_STMT_WRITELN:
            codegen->builtin.w_ln++;
            codegen_print(codegen, "CALL", "BWLN");
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

    codegen_set_label(codegen, codegen_label_for(codegen, block));
    codegen->stmt_cnt = 0;
    codegen_stmt(codegen, block->stmt);
    switch (block->termn.kind) {
    case IR_TERMN_GOTO: {
        const ir_block_t *next = block->termn.u.goto_termn.next;
        if (codegen_addr_lookup(codegen, next)) {
            codegen_print(codegen, "JUMP", "%s", codegen_label_for(codegen, next));
        } else {
            codegen_block(codegen, next);
        }
        break;
    }
    case IR_TERMN_IF: {
        const ir_operand_t *cond = block->termn.u.if_termn.cond;
        const ir_block_t *then = block->termn.u.if_termn.then;
        const ir_block_t *els = block->termn.u.if_termn.els;

        codegen_load(codegen, "GR1", cond);
        codegen_print(codegen, "LAD", "GR2, 0");
        codegen_print(codegen, "CPA", "GR1, GR2");
        if (codegen_addr_lookup(codegen, then)) {
            codegen_print(codegen, "JNZ", "%s", codegen_label_for(codegen, then));
            if (codegen_addr_lookup(codegen, els)) {
                codegen_print(codegen, "JUMP", "%s", codegen_label_for(codegen, els));
            } else {
                codegen_block(codegen, els);
            }
        } else {
            if (codegen_addr_lookup(codegen, els)) {
                codegen_print(codegen, "JZE", "%s", codegen_label_for(codegen, els));
                codegen_block(codegen, then);
            } else {
                codegen_print(codegen, "JZE", "%s", codegen_label_for(codegen, els));
                codegen_block(codegen, then);
                codegen_block(codegen, els);
            }
        }
        break;
    }
    case IR_TERMN_RETURN:
        codegen_print(codegen, "RET", NULL);
        break;
    case IR_TERMN_ARG:
        codegen_push_operand_address(codegen, block->termn.u.arg_termn.arg);
        codegen_block(codegen, block->termn.u.arg_termn.next);
        break;
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
    assert(codegen);

    while (item) {
        switch (item->kind) {
        case IR_ITEM_PROGRAM:
            codegen_print(codegen, "CALL", codegen_label_for(codegen, item->body->inner));
            codegen_print(codegen, "SVC", "0");
            codegen_body(codegen, item->body);
            break;
        case IR_ITEM_PROCEDURE:
            codegen_body(codegen, item->body);
            break;
        case IR_ITEM_VAR:
        case IR_ITEM_LOCAL_VAR:
            codegen_set_label(codegen, codegen_label_for(codegen, item));
            switch (item->type->kind) {
            case IR_TYPE_INTEGER:
            case IR_TYPE_BOOLEAN:
            case IR_TYPE_CHAR:
                codegen_print(codegen, "DS", "1");
                break;
            case IR_TYPE_ARRAY:
                codegen_print(codegen, "DS", "%ld", item->type->u.array_type.size);
                break;
            default:
                unreachable();
            }
            break;
        case IR_ITEM_ARG_VAR:
            codegen_set_label(codegen, codegen_label_for(codegen, item));
            switch (item->type->kind) {
            case IR_TYPE_INTEGER:
            case IR_TYPE_BOOLEAN:
            case IR_TYPE_CHAR:
                codegen_print(codegen, "DS", "1");
                break;
            default:
                unreachable();
            }
            break;
        }
        item = item->next;
    }
}

void codegen_builtin(codegen_t *codegen)
{

}

void codegen_ir(codegen_t *codegen, const ir_t *ir)
{
    codegen_print(codegen, "START", NULL);
    codegen_item(codegen, ir->items);
    codegen_constant(codegen, ir->constants);
    codegen_builtin(codegen);
    codegen_print(codegen, "END", NULL);
}

void casl2_codegen(const ir_t *ir)
{
    codegen_t codegen;
    ir_item_t *item;
    assert(ir);

    codegen.file = fopen(ir->source->output_filename, "w");
    codegen.addr.cnt = 1;
    codegen.addr.table = new_hash_table(hash_table_default_comparator, hash_table_default_hasher);
    codegen.label[0] = '\0';
    codegen.builtin.r_int = 0;
    codegen.builtin.r_char = 0;
    codegen.builtin.r_bool = 0;
    codegen.builtin.r_ln = 0;
    codegen.builtin.w_int = 0;
    codegen.builtin.w_char = 0;
    codegen.builtin.w_bool = 0;
    codegen.builtin.w_ln = 0;
    codegen_ir(&codegen, ir);
    delete_hash_table(codegen.addr.table, NULL, NULL);
    fclose(codegen.file);
}
