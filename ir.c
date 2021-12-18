#include <assert.h>

#include "mppl.h"

static ir_type_instance_t *new_ir_type_instance(ir_type_kind_t kind)
{
    ir_type_instance_t *ret = new(ir_type_instance_t);
    ret->kind = kind;
    ret->next = NULL;
    return ret;
}

ir_type_instance_t *new_ir_program_type_instance()
{
    return new_ir_type_instance(IR_TYPE_PROGRAM);
}

ir_type_instance_t *new_ir_procedure_type_instance(ir_type_instance_t *param_types)
{
    ir_type_instance_t *ret;
    assert(param_types);

    ret = new_ir_type_instance(IR_TYPE_PROCEDURE);
    ret->u.procedure_type.param_types = param_types;
    return ret;
}

ir_type_instance_t *new_ir_array_type_instance(ir_type_instance_t *base_type, size_t size)
{
    ir_type_instance_t *ret = new_ir_type_instance(IR_TYPE_ARRAY);
    ret->u.array_type.base_type = base_type;
    ret->u.array_type.size = size;
    return ret;
}

ir_type_instance_t *new_ir_integer_type_instance()
{
    return new_ir_type_instance(IR_TYPE_INTEGER);
}

ir_type_instance_t *new_ir_boolean_type_instance()
{
    return new_ir_type_instance(IR_TYPE_BOOLEAN);
}

ir_type_instance_t *new_ir_char_type_instance()
{
    return new_ir_type_instance(IR_TYPE_CHAR);
}

void delete_ir_type_instance(ir_type_instance_t *type)
{
    if (!type) {
        return;
    }
    switch (type->kind) {
    case IR_TYPE_PROCEDURE:
        delete_ir_type_instance(type->u.procedure_type.param_types);
        break;
    case IR_TYPE_ARRAY:
        delete_ir_type_instance(type->u.array_type.base_type);
        break;
    }
    delete_ir_type_instance(type->next);
    free(type);
}

static int ir_type_instance_comparator(const void *lhs, const void *rhs)
{
    const ir_type_instance_t *l = lhs, *r = rhs;
    ir_type_instance_t *lcur, *rcur;

    if (l->kind != r->kind) {
        return 0;
    }
    switch (l->kind) {
    case IR_TYPE_PROCEDURE:
        lcur = l->u.procedure_type.param_types;
        rcur = r->u.procedure_type.param_types;
        while (lcur && rcur) {
            if (lcur->u.ref != rcur->u.ref) {
                return 0;
            }
            lcur = lcur->next;
            rcur = rcur->next;
        }
        return !lcur && !rcur;

    case IR_TYPE_ARRAY:
        lcur = l->u.array_type.base_type;
        rcur = r->u.array_type.base_type;
        if (lcur->u.ref != rcur->u.ref) {
            return 0;
        }
        return l->u.array_type.size == r->u.array_type.size;

    default:
        return 1;
    }
}

static uint64_t ir_type_instance_hasher(const void *ptr)
{
    const ir_type_instance_t *p = ptr;
    ir_type_instance_t *cur;
    uint64_t ret = fnv1_int(p->kind);

    switch (p->kind) {
    case IR_TYPE_PROCEDURE:
        cur = p->u.procedure_type.param_types;
        while (cur) {
            ret = 31 * ret + fnv1_ptr(ir_type_get_instance(cur->u.ref));
            cur = cur->next;
        }
        break;
    case IR_TYPE_ARRAY:
        cur = p->u.array_type.base_type;
        ret = 31 * ret + fnv1_ptr(ir_type_get_instance(cur->u.ref));
        ret = 31 * ret + fnv1_int(p->u.array_type.size);
        break;
    }
    return ret;
}

ir_type_storage_t *new_ir_type_storage()
{
    ir_type_storage_t *ret = new(ir_type_storage_t);
    ret->table = new_hash_table(ir_type_instance_comparator, ir_type_instance_hasher);
    return ret;
}

void delete_ir_type_storage(ir_type_storage_t *storage)
{
    if (!storage) {
        return;
    }
    delete_hash_table(storage->table, free, NULL);
    free(storage);
}

ir_type_instance_t *new_ir_type_ref(ir_type_t type)
{
    ir_type_instance_t *ret;
    assert(type);

    ret = new_ir_type_instance(-1);
    ret->u.ref = type;
    return ret;
}

static ir_type_instance_t *ir_type_intern_chaining(ir_type_storage_t *storage, ir_type_instance_t *types)
{
    ir_type_instance_t *ret, *next;
    ir_type_t type;
    assert(storage);
    if (!types) {
        return NULL;
    }

    next = ir_type_intern_chaining(storage, types->next);
    if (types->kind != -1) {
        type = ir_type_intern(storage, types);
        ret = new_ir_type_ref(type);
    } else {
        ret = types;
    }
    ret->next = next;
    return ret;
}

ir_type_t ir_type_intern(ir_type_storage_t *storage, ir_type_instance_t *instance)
{
    ir_type_instance_t *cur;
    const hash_table_entry_t *entry;
    assert(storage && instance);
    assert(instance->kind != -1);

    switch (instance->kind) {
    case IR_TYPE_PROCEDURE:
        instance->u.procedure_type.param_types =
            ir_type_intern_chaining(storage, instance->u.procedure_type.param_types);
        break;
    case IR_TYPE_ARRAY:
        instance->u.array_type.base_type =
            ir_type_intern_chaining(storage, instance->u.array_type.base_type);
        break;
    }
    if (entry = hash_table_find(storage->table, instance)) {
        delete_ir_type_instance(instance);
        return (ir_type_t) entry->value;
    }
    hash_table_insert_unchecked(storage->table, instance, instance);
    return (ir_type_t) instance;
}

const ir_type_instance_t *ir_type_get_instance(ir_type_t type)
{
    return (ir_type_instance_t *) type;
}

static ir_local_t *new_ir_local(ir_local_kind_t kind)
{
    ir_local_t *ret = new(ir_local_t);
    ret->kind = kind;
    return ret;
}

ir_local_t *new_ir_normal_local(symbol_t key)
{
    ir_local_t *ret = new_ir_local(IR_LOCAL_NORMAL);
    ret->key = key;
    return ret;
}

ir_local_t *new_ir_temp_local(symbol_t key)
{
    ir_local_t *ret = new_ir_local(IR_LOCAL_TEMP);
    ret->key = key;
    return ret;
}

ir_local_t *new_ir_ref_local(symbol_t key)
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
    ret->next = NULL;
    return ret;
}

void delete_ir_place(ir_place_t *place)
{
    if (!place) {
        return;
    }
    delete_ir_local(place->local);
    delete_ir_place_access(place->place_access);
    delete_ir_place(place->next);
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

ir_constant_t *new_ir_string_constant(symbol_t value)
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

ir_rvalue_t *new_ir_cast_rvalue(ir_type_t type, ir_operand_t *value)
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
    free(rvalue);
}

static ir_stmt_t *new_ir_stmt(ir_stmt_kind_t kind)
{
    ir_stmt_t *ret = new(ir_stmt_t);
    ret->kind = kind;
    ret->next = NULL;
    return ret;
}

ir_stmt_t *new_ir_assign_stmt(ir_place_t *lhs, ir_rvalue_t *rhs)
{
    ir_stmt_t *ret;
    assert(lhs && rhs);

    ret = new_ir_stmt(IR_STMT_ASSIGN);
    ret->u.assign_stmt.lhs = lhs;
    ret->u.assign_stmt.rhs = rhs;
    return ret;
}

ir_stmt_t *new_ir_call_stmt(ir_place_t *func, ir_place_t *args)
{
    ir_stmt_t *ret;
    assert(func && args);

    ret = new_ir_stmt(IR_STMT_CALL);
    ret->u.call_stmt.func = func;
    ret->u.call_stmt.args = args;
    return ret;
}

void delete_ir_stmt(ir_stmt_t *stmt)
{
    if (!stmt) {
        return;
    }

    switch (stmt->kind) {
    case IR_STMT_ASSIGN:
        delete_ir_place(stmt->u.assign_stmt.lhs);
        delete_ir_rvalue(stmt->u.assign_stmt.rhs);
        break;
    case IR_STMT_CALL:
        delete_ir_place(stmt->u.call_stmt.func);
        delete_ir_place(stmt->u.call_stmt.args);
        break;
    }
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

ir_item_table_t *new_ir_item_table()
{
    ir_item_table_t *ret = new(ir_item_table_t);
    ret->table = new_hash_table(hash_table_default_comparator, hash_table_default_hasher);
    return ret;
}

void delete_ir_item_table(ir_item_table_t *table)
{
    if (!table) {
        return;
    }
    delete_hash_table(table->table, NULL, NULL);
    free(table);
}

int ir_item_table_try_register(ir_item_table_t *table, ir_item_t *item)
{
    if (hash_table_find(table->table, (void *) item->symbol)) {
        return 0;
    }
    hash_table_insert_unchecked(table->table, (void *) item->symbol, item);
    return 1;
}

ir_body_t *new_ir_body(ir_block_t *inner, ir_item_table_t *items)
{
    ir_body_t *ret;
    assert(inner);

    ret->inner = inner;
    ret->items = items;
    return ret;
}

void delete_ir_body(ir_body_t *body)
{
    if (!body) {
        return;
    }
    delete_ir_block(body->inner);
    delete_ir_item_table(body->items);
    delete_ir_item_table(body->refs);
    free(body);
}

static ir_item_t *new_ir_item(ir_item_kind_t kind, ir_type_t type, symbol_t symbol)
{
    ir_item_t *ret;
    assert(type && symbol);

    ret = new(ir_item_t);
    ret->kind = kind;
    ret->type = type;
    ret->symbol = symbol;
    ret->body = 0;
    return ret;
}

ir_item_t *new_ir_program_item(ir_type_t type, symbol_t symbol)
{
    assert(type && symbol);
    return new_ir_item(IR_ITEM_PROGRAM, type, symbol);
}

ir_item_t *new_ir_procedure_item(ir_type_t type, symbol_t symbol)
{
    assert(type && symbol);
    return new_ir_item(IR_ITEM_PROCEDURE, type, symbol);
}

ir_item_t *new_ir_var_item(ir_type_t type, symbol_t symbol)
{
    assert(type && symbol);
    return new_ir_item(IR_ITEM_VAR, type, symbol);
}

ir_item_t *new_ir_param_var_item(ir_type_t type, symbol_t symbol)
{
    assert(type && symbol);
    return new_ir_item(IR_ITEM_ARG_VAR, type, symbol);
}

ir_item_t *new_ir_local_var_item(ir_type_t type, symbol_t symbol)
{
    assert(type && symbol);
    return new_ir_item(IR_ITEM_LOCAL_VAR, type, symbol);
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
