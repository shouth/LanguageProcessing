#include <assert.h>
#include <stdarg.h>
#include <stdio.h>

#include "mppl.h"

typedef size_t codegen_addr_t;

typedef struct {
    FILE *file;
    struct {
        codegen_addr_t cnt;
        hash_table_t *table;
    } addr;
} codegen_t;

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

void codegen_constant(codegen_t *codegen, const ir_constant_t *constant)
{
    assert(codegen && constant);

    while (constant) {
        codegen_addr_t addr = codegen_addr_for(codegen, constant);
        /* コード生成 */
        constant = constant->next;
    }
}

void codegen_item(codegen_t *codegen, const ir_item_t *item);

void codegen_stmt(codegen_t *codegen, const ir_stmt_t *stmt)
{
    assert(codegen && stmt);

    /* コード生成 */
    while (stmt) {
        switch (stmt->kind) {
        case IR_STMT_ASSIGN:
            break;
        case IR_STMT_CALL:
            break;
        case IR_STMT_READ:
            break;
        case IR_STMT_WRITE:
            break;
        }
        stmt = stmt->next;
    }
}

void codegen_block(codegen_t *codegen, const ir_block_t *block)
{
    codegen_addr_t addr;
    assert(codegen && block);

    addr = codegen_addr_for(codegen, block);
    /* コード生成 */
}

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
    codegen.addr.cnt = 0;
    codegen.addr.table = new_hash_table(hash_table_default_comparator, hash_table_default_hasher);
    codegen_ir(&codegen, ir);
    delete_hash_table(codegen.addr.table, NULL, NULL);
    fclose(codegen.file);
}
