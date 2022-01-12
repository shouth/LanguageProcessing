#include <assert.h>
#include <stdarg.h>
#include <stdio.h>

#include "mppl.h"

typedef size_t codegen_addr_t;

typedef const char *codegen_gr_t;

#define GR(num) "GR" #num

typedef struct {
    FILE *file;
    codegen_addr_t addr;
} codegen_t;

static const char *label(codegen_addr_t addr)
{
    static char buf[16];
    sprintf(buf, "L%ld", addr);
    return buf;
}

static void start(codegen_t *gen)
{ fprintf(gen->file, "\tSTART"); }

static void end(codegen_t *gen)
{ fprintf(gen->file, "\tEND"); }

static void ds(codegen_t *gen, uint16_t size)
{ fprintf(gen->file, "\tDS\t%u", size); }

static void dc_char(codegen_t *gen, uint8_t c)
{ fprintf(gen->file, "\tDC\t%u", (int) c); }

static void dc_num(codegen_t *gen, uint16_t n)
{ fprintf(gen->file, "\tDC\t%u,%u", n & 0xff, n >> 8); }

static void dc_string(codegen_t *gen, symbol_t symbol)
{
    const symbol_instance_t *instance = symbol_get_instance(symbol);
    fprintf(gen->file, "\tDC\t'%.*s'", (int) instance->len, instance->ptr);
}

static void in(codegen_t *gen)
{ fprintf(gen->file, "\tIN\tIBUF,IBUFSIZE"); }

static void out(codegen_t *gen)
{ fprintf(gen->file, "\tOUT\tOBUF,OBUFSIZE"); }

static void rpush(codegen_t *gen)
{ fprintf(gen->file, "\tRPUSH"); }

static void rpop(codegen_t *gen)
{ fprintf(gen->file, "\tRPOP"); }

static void codegen_block(ir_block_t *block)
{
}

void codegen_item(ir_item_t *item);

void codegen_body(ir_body_t *body)
{
    codegen_item(body->items);

}

void codegen_item(ir_item_t *item)
{
    while (item) {
        if (item->refs.head) {
            switch (item->kind) {
            case IR_ITEM_PROGRAM:
                codegen_body(item->body);
                break;
            case IR_ITEM_PROCEDURE:
                codegen_body(item->body);
                break;
            case IR_ITEM_VAR:
                break;
            case IR_ITEM_LOCAL_VAR:
                break;
            case IR_ITEM_ARG_VAR:
                break;
            }
        }
        item = item->next;
    }
}

void codegen(ir_t *ir)
{
    codegen_t gen;
    ir_item_t *item;
    assert(ir);

    gen.file = fopen(ir->source->output_filename, "w");
    gen.addr = 0;
    codegen_item(ir->items);
    fclose(gen.file);
}
