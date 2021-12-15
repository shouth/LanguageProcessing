#include <assert.h>

#include "mppl.h"

static ir_type_t *new_ir_type(ir_type_kind_t kind)
{
    ir_type_t *ret = new(ir_type_t);
    ret->kind = kind;
    ret->next = NULL;
    return ret;
}

ir_type_t *new_ir_program_type()
{
    return new_ir_type(IR_TYPE_PROGRAM);
}

ir_type_t *new_ir_procedure_type(ir_type_t *arg_types)
{
    ir_type_t *ret;
    assert(arg_types);

    ret = new_ir_type(IR_TYPE_PROCEDURE);
    ret->u.procedure_type.arg_types = arg_types;
    return ret;
}

ir_type_t *new_ir_array_type(size_t size)
{
    ir_type_t *ret = new_ir_type(IR_TYPE_ARRAY);
    ret->u.array_type.size = size;
    return ret;
}

ir_type_t *new_ir_integer_type()
{
    return new_ir_type(IR_TYPE_INTEGER);
}

ir_type_t *new_ir_boolean_type()
{
    return new_ir_type(IR_TYPE_BOOLEAN);
}

ir_type_t *new_ir_char_type()
{
    return new_ir_type(IR_TYPE_CHAR);
}

void delete_ir_type(ir_type_t *type)
{
    if (!type) {
        return;
    }
    switch (type->kind) {
    case IR_TYPE_PROCEDURE:
        delete_ir_type(type->u.procedure_type.arg_types);
        break;
    }
    delete_ir_type(type->next);
    free(type);
    return;
}

static ir_local_t *new_ir_local(ir_local_kind_t kind)
{
    ir_local_t *ret = new(ir_local_t);
    ret->kind = kind;
    return ret;
}

ir_local_t *new_ir_normal_local(const symbol_t *key)
{
    ir_local_t *ret = new_ir_local(IR_LOCAL_NORMAL);
    ret->key = key;
    return ret;
}

ir_local_t *new_ir_temp_local(const symbol_t *key)
{
    ir_local_t *ret = new_ir_local(IR_LOCAL_TEMP);
    ret->key = key;
    return ret;
}

ir_local_t *new_ir_ref_local(const symbol_t *key)
{
    ir_local_t *ret = new_ir_local(IR_LOCAL_REF);
    ret->key = key;
    return ret;
}

void delete_ir_local(ir_local_t *local)
{
    if (!local) {
        return;
    }
    free(local);
}

static ir_place_access_t *new_ir_place_access(ir_place_access_kind_t kind)
{
    ir_place_access_t *ret = new(ir_place_access_t);
    ret->kind = kind;
    return ret;
}

ir_place_access_t *new_ir_normal_place_access()
{
    return new_ir_place_access(IR_PLACE_ACCESS_NORMAL);
}

ir_place_access_t *new_ir_index_place_access(size_t index)
{
    ir_place_access_t *ret = new_ir_place_access(IR_PLACE_ACCESS_INDEX);
    ret->u.index_place_access.index = index;
    return ret;
}

void delete_ir_place_access(ir_place_access_t *place_access)
{
    free(place_access);
}

ir_place_t *new_ir_place(ir_local_t *local, ir_place_access_t *place_access)
{
    ir_place_t *ret;
    assert(local && place_access);
    ret = new(ir_place_t);
    ret->local = local;
    ret->place_access = place_access;
    return ret;
}

void delete_ir_place(ir_place_t *place)
{
    delete_ir_local(place->local);
    delete_ir_place_access(place->place_access);
    free(place);
}

static ir_constant_t *new_ir_constant(ir_constant_kind_t kind)
{
    ir_constant_t *ret = new(ir_constant_t);
    ret->kind = kind;
    return ret;
}

ir_constant_t *new_ir_number_constant(unsigned long value)
{
    ir_constant_t *ret = new_ir_constant(IR_CONSTANT_NUMBER);
    ret->u.number_constant.value = value;
    return ret;
}

ir_constant_t *new_ir_boolean_constant(int value)
{
    ir_constant_t *ret = new_ir_constant(IR_CONSTANT_BOOLEAN);
    ret->u.boolean_constant.value = value;
    return ret;
}

ir_constant_t *new_ir_char_constant(int value)
{
    ir_constant_t *ret = new_ir_constant(IR_CONSTANT_CHAR);
    ret->u.char_constant.value = value;
    return ret;
}

ir_constant_t *new_ir_string_constant(const symbol_t *value)
{
    ir_constant_t *ret = new_ir_constant(IR_CONSTANT_STRING);
    ret->u.string_constant.value = value;
    return ret;
}

void delete_ir_constant(ir_constant_t *constant)
{
    if (!constant) {
        return;
    }
    free(constant);
}

static ir_operand_t *new_ir_operand(ir_operand_kind_t kind)
{
    ir_operand_t *ret = new(ir_operand_t);
    ret->kind = kind;
    return ret;
}

ir_operand_t *new_ir_place_operand(ir_place_t *place)
{
    ir_operand_t *ret;
    assert(place);
    ret = new_ir_operand(IR_OPERAND_PLACE);
    ret->u.place_operand.place = place;
    return ret;
}

ir_operand_t *new_ir_constant_operand(ir_constant_t *constant)
{
    ir_operand_t *ret;
    assert(constant);
    ret = new_ir_operand(IR_OPERAND_CONSTANT);
    ret->u.constant_operand.constant = constant;
    return ret;
}

void delete_ir_operand(ir_operand_t *operand)
{
    if (!operand) {
        return;
    }
    switch (operand->kind) {
    case IR_OPERAND_PLACE:
        delete_ir_place(operand->u.place_operand.place);
        break;
    case IR_OPERAND_CONSTANT:
        delete_ir_constant(operand->u.constant_operand.constant);
        break;
    }
    free(operand);
}

static ir_rvalue_t *new_ir_rvalue(ir_rvalue_kind_t kind)
{
    ir_rvalue_t *ret = new(ir_rvalue_t);
    ret->kind = kind;
    ret->next = NULL;
    return ret;
}

ir_rvalue_t *new_ir_use_rvalue(ir_operand_t *place)
{
    ir_rvalue_t *ret;
    assert(place);

    ret = new_ir_rvalue(IR_RVALUE_USE);
    ret->u.use_rvalue.place = place;
    return ret;
}

ir_rvalue_t *new_ir_binary_op_rvalue(ast_binary_op_kind_t kind, ir_operand_t *lhs, ir_operand_t *rhs)
{
    ir_rvalue_t *ret;
    assert(lhs && rhs);

    ret = new_ir_rvalue(IR_RVALUE_BINARY_OP);
    ret->u.binary_op_rvalue.kind = kind;
    ret->u.binary_op_rvalue.lhs = lhs;
    ret->u.binary_op_rvalue.rhs = rhs;
    return ret;
}

ir_rvalue_t *new_ir_unary_op_rvalue(ast_unary_op_kind_t kind, ir_operand_t *value)
{
    ir_rvalue_t *ret;
    assert(value);

    ret = new_ir_rvalue(IR_RVALUE_UNARY_OP);
    ret->u.unary_op_rvalue.kind = kind;
    ret->u.unary_op_rvalue.value = value;
    return ret;
}

ir_rvalue_t *new_ir_cast_rvalue(const ir_type_t *type, ir_operand_t *value)
{
    ir_rvalue_t *ret;
    assert(type && value);

    ret = new_ir_rvalue(IR_RVALUE_CAST);
    ret->u.cast_rvalue.type = type;
    ret->u.cast_rvalue.value = value;
    return ret;
}

void delete_ir_rvalue(ir_rvalue_t *rvalue)
{
    if (!rvalue) {
        return;
    }
    switch (rvalue->kind) {
    case IR_RVALUE_USE:
        delete_ir_operand(rvalue->u.use_rvalue.place);
        break;
    case IR_RVALUE_BINARY_OP:
        delete_ir_operand(rvalue->u.binary_op_rvalue.lhs);
        delete_ir_operand(rvalue->u.binary_op_rvalue.rhs);
        break;
    case IR_RVALUE_UNARY_OP:
        delete_ir_operand(rvalue->u.unary_op_rvalue.value);
        break;
    case IR_RVALUE_CAST:
        delete_ir_operand(rvalue->u.cast_rvalue.value);
        break;
    }
    delete_ir_rvalue(rvalue->next);
    free(rvalue);
}

ir_stmt_t *new_ir_stmt(ir_place_t *lhs, ir_rvalue_t *rhs)
{
    ir_stmt_t *ret;
    assert(lhs && rhs);

    ret = new(ir_stmt_t);
    ret->lhs = lhs;
    ret->rhs = rhs;
    ret->next = NULL;
    return ret;
}

void delete_ir_stmt(ir_stmt_t *stmt)
{
    if (!stmt) {
        return;
    }
    delete_ir_place(stmt->lhs);
    delete_ir_rvalue(stmt->rhs);
    delete_ir_stmt(stmt->next);
    free(stmt);
}

static ir_termn_t *new_ir_termn(ir_termn_kind_t kind)
{
    ir_termn_t *ret = new(ir_termn_t);
    ret->kind = kind;
    return ret;
}

ir_termn_t *new_ir_goto_termn(ir_block_t *next)
{
    ir_termn_t *ret;
    assert(next);

    ret = new_ir_termn(IR_TERMN_GOTO);
    ret->u.goto_termn.next = next;
    return ret;
}

ir_termn_t *new_ir_if_termn(ir_operand_t *cond, ir_block_t *then, ir_block_t *els)
{
    ir_termn_t *ret;
    assert(cond && then && els);

    ret = new_ir_termn(IR_TERMN_IF);
    ret->u.if_termn.cond = cond;
    ret->u.if_termn.then = then;
    ret->u.if_termn.els = els;
    return ret;
}

ir_termn_t *new_ir_call_termn(ir_place_t *func, ir_rvalue_t *args, ir_block_t *dest)
{
    ir_termn_t *ret;
    assert(func && args && dest);

    ret = new_ir_termn(IR_TERMN_CALL);
    ret->u.call_termn.func = func;
    ret->u.call_termn.args = args;
    ret->u.call_termn.dest = dest;
    return ret;
}

ir_termn_t *new_ir_return_termn()
{
    return new_ir_termn(IR_TERMN_RETURN);
}

void delete_ir_block(ir_block_t *block);

void delete_ir_termn(ir_termn_t *termn)
{
    if (!termn) {
        return;
    }
    switch (termn->kind) {
    case IR_TERMN_GOTO:
        delete_ir_block(termn->u.goto_termn.next);
        break;
    case IR_TERMN_IF:
        delete_ir_operand(termn->u.if_termn.cond);
        delete_ir_block(termn->u.if_termn.then);
        delete_ir_block(termn->u.if_termn.els);
        break;
    case IR_TERMN_CALL:
        delete_ir_place(termn->u.call_termn.func);
        delete_ir_rvalue(termn->u.call_termn.args);
        delete_ir_block(termn->u.call_termn.dest);
        break;
    }
    free(termn);
}

ir_block_t *new_ir_block(ir_stmt_t *stmt, ir_termn_t *termn)
{
    ir_block_t *ret;
    assert(stmt && termn);

    ret->stmt = stmt;
    ret->termn = termn;
    return ret;
}

void delete_ir_block(ir_block_t *block)
{
    if (!block) {
        return;
    }
    delete_ir_stmt(block->stmt);
    delete_ir_termn(block->termn);
    free(block);
}

ir_body_t *new_ir_body(ir_block_t *inner)
{
    ir_body_t *ret;
    assert(inner);

    ret->inner = inner;
    ret->items = new_hash_table(hash_table_default_comparator, hash_table_default_hasher);
    ret->refs = new_hash_table(hash_table_default_comparator, hash_table_default_hasher);
    return ret;
}

void delete_ir_body(ir_body_t *body)
{
    if (!body) {
        return;
    }
    delete_ir_block(body->inner);
    delete_hash_table(body->items, NULL, NULL);
    delete_hash_table(body->refs, NULL, NULL);
    free(body);
}

ir_item_t *new_ir_item(ir_item_kind_t kind, const ir_type_t *type, const symbol_t *symbol)
{
    ir_item_t *ret;
    assert(type && symbol);

    ret = new(ir_item_t);
    ret->kind = kind;
    ret->type = type;
    ret->symbol = symbol;
    ret->body = NULL;
    ret->next_key = NULL;
    return ret;
}

void delete_ir_item(ir_item_t *item)
{
    if (!item) {
        return;
    }
    delete_ir_body(item->body);
    free(item);
}

ir_t *new_ir(ir_item_t *program)
{
    ir_t *ret;
    assert(program);

    ret = new(ir_t);
    ret->program = program;
    return ret;
}

void delete_ir(ir_t *ir)
{
    if (!ir) {
        return;
    }
    delete_ir_item(ir->program);
    free(ir);
}
