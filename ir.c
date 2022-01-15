#include <assert.h>

#include "mppl.h"

const char *ir_type_kind_str(ir_type_kind_t kind)
{
    switch (kind) {
    case IR_TYPE_PROGRAM:
        return "program";
    case IR_TYPE_PROCEDURE:
        return "procedure";
    case IR_TYPE_INTEGER:
        return "integer";
    case IR_TYPE_CHAR:
        return "char";
    case IR_TYPE_BOOLEAN:
        return "boolean";
    case IR_TYPE_ARRAY:
        return "array";
    default:
        unreachable();
    }
}

int ir_type_is_kind(const ir_type_t *type, ir_type_kind_t kind)
{ return type->kind == kind; }

int ir_type_is_std(const ir_type_t *type)
{
    return ir_type_is_kind(type, IR_TYPE_INTEGER)
        || ir_type_is_kind(type, IR_TYPE_CHAR)
        || ir_type_is_kind(type, IR_TYPE_BOOLEAN);
}

static ir_type_t *new_ir_type(ir_type_kind_t kind)
{
    ir_type_t *ret = new(ir_type_t);
    ret->kind = kind;
    ret->next = NULL;
    return ret;
}

ir_type_t *new_ir_type_ref(const ir_type_t *type)
{
    ir_type_t *ret;
    assert(type);

    ret = new_ir_type(-1);
    ret->u.ref = type;
    return ret;
}

static void delete_ir_type(ir_type_t *type)
{
    if (!type) {
        return;
    }
    switch (type->kind) {
    case IR_TYPE_PROCEDURE:
        delete_ir_type(type->u.procedure_type.param_types);
        break;
    case IR_TYPE_ARRAY:
        delete_ir_type(type->u.array_type.base_type);
        break;
    }
    delete_ir_type(type->next);
    free(type);
}

static char *internal_ir_type_str(char *buf, const ir_type_t *type)
{
    buf += sprintf(buf, "%s", ir_type_kind_str(type->kind));
    switch (type->kind) {
    case IR_TYPE_PROCEDURE: {
        const ir_type_t *cur = type->u.procedure_type.param_types;
        buf += sprintf(buf, "(");
        while (cur) {
            buf = internal_ir_type_str(buf, cur->u.ref);
            if (cur = cur->next) {
                buf += sprintf(buf, ", ");
            }
        }
        buf += sprintf(buf, ")");
        break;
    }
    case IR_TYPE_ARRAY:
        buf += sprintf(buf, "[%ld] of ", type->u.array_type.size);
        internal_ir_type_str(buf, type->u.array_type.base_type->u.ref);
        break;
    }
    return buf;
}

const char *ir_type_str(const ir_type_t *type)
{
    static char buffer[1024];
    internal_ir_type_str(buffer, type);
    return buffer;
}

static int ir_type_comparator(const void *lhs, const void *rhs)
{
    const ir_type_t *l = lhs, *r = rhs;
    ir_type_t *lcur, *rcur;

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
        return lcur->u.ref == rcur->u.ref && l->u.array_type.size == r->u.array_type.size;

    default:
        return 1;
    }
}

static uint64_t ir_type_hasher(const void *ptr)
{
    const ir_type_t *p = ptr;
    ir_type_t *cur;
    uint64_t ret = fnv1_int(p->kind);

    switch (p->kind) {
    case IR_TYPE_PROCEDURE:
        cur = p->u.procedure_type.param_types;
        while (cur) {
            ret = 31 * ret + fnv1_ptr(cur->u.ref);
            cur = cur->next;
        }
        break;
    case IR_TYPE_ARRAY:
        cur = p->u.array_type.base_type;
        ret = 31 * ret + fnv1_ptr(cur->u.ref);
        ret = 31 * ret + fnv1_int(p->u.array_type.size);
        break;
    }
    return ret;
}

ir_factory_t *new_ir_factory(ir_block_t **blocks, ir_constant_t **constants, ir_type_t **types)
{
    ir_factory_t *ret;
    assert(blocks && constants);
    ret = new(ir_factory_t);
    ret->scope = NULL;
    ret->blocks = blocks;
    ret->constants = constants;

    ret->types.tail = types;
    ret->types.table = new_hash_table(ir_type_comparator, ir_type_hasher);
    ret->types.program = ir_type_intern(ret, new_ir_type(IR_TYPE_PROGRAM));
    ret->types.std_integer = ir_type_intern(ret, new_ir_type(IR_TYPE_INTEGER));
    ret->types.std_char = ir_type_intern(ret, new_ir_type(IR_TYPE_CHAR));
    ret->types.std_boolean = ir_type_intern(ret, new_ir_type(IR_TYPE_BOOLEAN));
    return ret;
}

void delete_ir_factory(ir_factory_t *factory)
{
    if (!factory) {
        return;
    }
    delete_hash_table(factory->types.table, NULL, NULL);
    free(factory);
}

static ir_type_t *ir_type_intern_chaining(ir_factory_t *factory, ir_type_t *types)
{
    ir_type_t *ret, *next;
    const ir_type_t *type;
    assert(factory);
    if (!types) {
        return NULL;
    }

    next = ir_type_intern_chaining(factory, types->next);
    if (types->kind != -1) {
        type = ir_type_intern(factory, types);
        ret = new_ir_type_ref(type);
    } else {
        ret = types;
    }
    ret->next = next;
    return ret;
}

const ir_type_t *ir_type_intern(ir_factory_t *factory, ir_type_t *type)
{
    ir_type_t *cur;
    const hash_table_entry_t *entry;
    assert(factory && type);
    assert(type->kind != -1);

    switch (type->kind) {
    case IR_TYPE_PROCEDURE:
        type->u.procedure_type.param_types =
            ir_type_intern_chaining(factory, type->u.procedure_type.param_types);
        break;
    case IR_TYPE_ARRAY:
        type->u.array_type.base_type =
            ir_type_intern_chaining(factory, type->u.array_type.base_type);
        break;
    }
    if (entry = hash_table_find(factory->types.table, type)) {
        delete_ir_type(type);
        return entry->value;
    }
    hash_table_insert_unchecked(factory->types.table, type, type);
    *factory->types.tail = type;
    factory->types.tail = &type->next;
    return type;
}

const ir_type_t *ir_type_program(ir_factory_t *factory)
{ return factory->types.program; }

const ir_type_t *ir_type_procedure(ir_factory_t *factory, ir_type_t *params)
{
    ir_type_t *procedure;
    assert(factory);
    {
        ir_type_t *cur = params;
        while (cur) {
            assert(cur->kind == -1);
            cur = cur->next;
        }
    }

    procedure = new_ir_type(IR_TYPE_PROCEDURE);
    procedure->u.procedure_type.param_types = params;
    return ir_type_intern(factory, procedure);
}

const ir_type_t *ir_type_array(ir_factory_t *factory, ir_type_t *base, size_t size)
{
    ir_type_t *array;
    assert(factory && base);
    assert(size > 0);
    assert(base->kind == -1);

    array = new_ir_type(IR_TYPE_ARRAY);
    array->u.array_type.base_type = base;
    array->u.array_type.size = size;
    return ir_type_intern(factory, array);
}

const ir_type_t *ir_type_integer(ir_factory_t *factory)
{ return factory->types.std_integer; }

const ir_type_t *ir_type_char(ir_factory_t *factory)
{ return factory->types.std_char; }

const ir_type_t *ir_type_boolean(ir_factory_t *factory)
{ return factory->types.std_boolean; }

void ir_scope_push(ir_factory_t *factory, const ir_item_t *owner, ir_item_t **items, ir_local_t **locals)
{
    ir_scope_t *scope;
    assert(factory && owner);

    scope = new(ir_scope_t);
    scope->next = factory->scope;
    factory->scope = scope;
    scope->owner = owner;
    scope->items.table = new_hash_table(hash_table_default_comparator, hash_table_default_hasher);
    scope->items.tail = items;
    scope->locals.table = new_hash_table(hash_table_default_comparator, hash_table_default_hasher);
    scope->locals.tail = locals;
}

void ir_scope_pop(ir_factory_t *factory)
{
    ir_scope_t *scope;
    assert(factory);

    scope = factory->scope;
    factory->scope = scope->next;
    delete_hash_table(scope->items.table, NULL, NULL);
    delete_hash_table(scope->locals.table, NULL, NULL);
    free(scope);
}

static ir_local_t *ir_scope_append_local(ir_scope_t *scope, ir_local_t *local)
{
    *scope->locals.tail = local;
    scope->locals.tail = &local->next;
    return local;
}

static ir_local_t *new_ir_local(ir_local_kind_t kind)
{
    ir_local_t *ret = new(ir_local_t);
    ret->kind = kind;
    ret->next = NULL;
    return ret;
}

ir_local_t *ir_local_for(ir_factory_t *factory, ir_item_t *item, size_t pos)
{
    const hash_table_entry_t *entry;
    ir_local_t *local;
    assert(factory && item);

    ir_item_add_ref(item, pos);
    if (entry = hash_table_find(factory->scope->locals.table, item)) {
        return entry->value;
    }

    switch (item->kind) {
    case IR_ITEM_ARG_VAR:
    case IR_ITEM_LOCAL_VAR:
        local = new_ir_local(IR_LOCAL_NORMAL);
        local->u.normal.item = item;
    default:
        local = new_ir_local(IR_LOCAL_REF);
        local->u.ref.item = item;
    }
    hash_table_insert_unchecked(factory->scope->locals.table, item, local);
    return ir_scope_append_local(factory->scope, local);
}

ir_local_t *ir_local_temp(ir_factory_t *factory, const ir_type_t *type)
{
    ir_local_t *local;
    assert(factory && type);

    local = new_ir_local(IR_LOCAL_TEMP);
    local->u.temp.type = type;
    return ir_scope_append_local(factory->scope, local);
}

const ir_type_t *ir_local_type(const ir_local_t *local)
{
    switch (local->kind) {
    case IR_LOCAL_TEMP:
        return local->u.temp.type;
    case IR_LOCAL_REF:
        return local->u.ref.item->type;
    case IR_LOCAL_NORMAL:
        return local->u.normal.item->type;
    }

    unreachable();
}

void delete_ir_local(ir_local_t *local)
{
    if (!local) {
        return;
    }
    delete_ir_local(local->next);
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

ir_place_access_t *new_ir_index_place_access(ir_operand_t *index)
{
    ir_place_access_t *ret = new_ir_place_access(IR_PLACE_ACCESS_INDEX);
    ret->u.index_place_access.index = index;
    return ret;
}

void delete_ir_place_access(ir_place_access_t *place_access)
{
    free(place_access);
}

ir_place_t *new_ir_place(const ir_local_t *local, ir_place_access_t *place_access)
{
    ir_place_t *ret = new(ir_place_t);
    ret->local = local;
    ret->place_access = place_access;
    ret->next = NULL;
    return ret;
}

const ir_type_t *ir_place_type(ir_place_t *place)
{
    const ir_type_t *type;
    assert(place);

    type = ir_local_type(place->local);

    switch (type->kind) {
    case IR_TYPE_ARRAY:
        if (place->place_access != NULL) {
            const ir_type_t *base_instance = type->u.array_type.base_type;
            return base_instance->u.ref;
        } else {
            return type;
        }

    default:
        if (place->place_access == NULL) {
            return type;
        }
    }

    unreachable();
}

void delete_ir_place(ir_place_t *place)
{
    if (!place) {
        return;
    }
    delete_ir_place_access(place->place_access);
    delete_ir_place(place->next);
    free(place);
}

static ir_constant_t *new_ir_constant(ir_constant_kind_t kind, const ir_type_t *type)
{
    ir_constant_t *ret = new(ir_constant_t);
    ret->kind = kind;
    ret->type = type;
    return ret;
}

ir_constant_t *new_ir_number_constant(const ir_type_t *type, unsigned long value)
{
    ir_constant_t *ret = new_ir_constant(IR_CONSTANT_NUMBER, type);
    ret->u.number_constant.value = value;
    return ret;
}

ir_constant_t *new_ir_boolean_constant(const ir_type_t *type, int value)
{
    ir_constant_t *ret = new_ir_constant(IR_CONSTANT_BOOLEAN, type);
    ret->u.boolean_constant.value = value;
    return ret;
}

ir_constant_t *new_ir_char_constant(const ir_type_t *type, int value)
{
    ir_constant_t *ret = new_ir_constant(IR_CONSTANT_CHAR, type);
    ret->u.char_constant.value = value;
    return ret;
}

ir_constant_t *new_ir_string_constant(const ir_type_t *type, symbol_t value)
{
    ir_constant_t *ret = new_ir_constant(IR_CONSTANT_STRING, type);
    ret->u.string_constant.value = value;
    return ret;
}

const ir_type_t *ir_constant_type(const ir_constant_t *constant)
{
    return constant->type;
}

void delete_ir_constant(ir_constant_t *constant)
{
    if (!constant) {
        return;
    }
    delete_ir_constant(constant->next);
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
    ir_operand_t *ret = new_ir_operand(IR_OPERAND_PLACE);
    ret->u.place_operand.place = place;
    return ret;
}

ir_operand_t *new_ir_constant_operand(const ir_constant_t *constant)
{
    ir_operand_t *ret = new_ir_operand(IR_OPERAND_CONSTANT);
    ret->u.constant_operand.constant = constant;
    return ret;
}

const ir_type_t *ir_operand_type(ir_operand_t *operand)
{
    assert(operand);

    switch (operand->kind) {
    case IR_OPERAND_PLACE:
        return ir_place_type(operand->u.place_operand.place);
    case IR_OPERAND_CONSTANT:
        return ir_constant_type(operand->u.constant_operand.constant);
    }
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
    }
    free(operand);
}

static ir_rvalue_t *new_ir_rvalue(ir_rvalue_kind_t kind)
{
    ir_rvalue_t *ret = new(ir_rvalue_t);
    ret->kind = kind;
    return ret;
}

ir_rvalue_t *new_ir_use_rvalue(ir_operand_t *operand)
{
    ir_rvalue_t *ret = new_ir_rvalue(IR_RVALUE_USE);
    ret->u.use_rvalue.operand = operand;
    return ret;
}

ir_rvalue_t *new_ir_binary_op_rvalue(ast_binary_op_kind_t kind, ir_operand_t *lhs, ir_operand_t *rhs)
{
    ir_rvalue_t *ret = new_ir_rvalue(IR_RVALUE_BINARY_OP);
    ret->u.binary_op_rvalue.kind = kind;
    ret->u.binary_op_rvalue.lhs = lhs;
    ret->u.binary_op_rvalue.rhs = rhs;
    return ret;
}

ir_rvalue_t *new_ir_unary_op_rvalue(ast_unary_op_kind_t kind, ir_operand_t *value)
{
    ir_rvalue_t *ret = new_ir_rvalue(IR_RVALUE_UNARY_OP);
    ret->u.unary_op_rvalue.kind = kind;
    ret->u.unary_op_rvalue.value = value;
    return ret;
}

ir_rvalue_t *new_ir_cast_rvalue(const ir_type_t *type, ir_operand_t *value)
{
    ir_rvalue_t *ret = new_ir_rvalue(IR_RVALUE_CAST);
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
        delete_ir_operand(rvalue->u.use_rvalue.operand);
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
    ir_stmt_t *ret = new_ir_stmt(IR_STMT_ASSIGN);
    ret->u.assign_stmt.lhs = lhs;
    ret->u.assign_stmt.rhs = rhs;
    return ret;
}

ir_stmt_t *new_ir_call_stmt(ir_place_t *func, ir_place_t *args)
{
    ir_stmt_t *ret = new_ir_stmt(IR_STMT_CALL);
    ret->u.call_stmt.func = func;
    ret->u.call_stmt.args = args;
    return ret;
}

ir_stmt_t *new_ir_read_stmt(ir_place_t *ref)
{
    ir_stmt_t *ret = new_ir_stmt(IR_STMT_READ);
    ret->u.read_stmt.ref = ref;
    return ret;
}

ir_stmt_t *new_ir_write_stmt(ir_operand_t *value, size_t len)
{
    ir_stmt_t *ret = new_ir_stmt(IR_STMT_WRITE);
    ret->u.write_stmt.value = value;
    ret->u.write_stmt.len = len;
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

ir_termn_t *new_ir_goto_termn(const ir_block_t *next)
{
    ir_termn_t *ret = new_ir_termn(IR_TERMN_GOTO);
    ret->u.goto_termn.next = next;
    return ret;
}

ir_termn_t *new_ir_if_termn(ir_operand_t *cond, const ir_block_t *then, const ir_block_t *els)
{
    ir_termn_t *ret = new_ir_termn(IR_TERMN_IF);
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
    case IR_TERMN_IF:
        delete_ir_operand(termn->u.if_termn.cond);
        break;
    }
    free(termn);
}

ir_block_t *new_ir_block()
{
    ir_block_t *ret = new(ir_block_t);
    ret->stmt = NULL;
    ret->stmt_tail = &ret->stmt;
    ret->termn = NULL;
    ret->next = NULL;
    return ret;
}

void ir_block_push(ir_block_t *block, ir_stmt_t *stmt)
{
    *block->stmt_tail = stmt;
    block->stmt_tail = &stmt->next;
}

void ir_block_terminate(ir_block_t *block, ir_termn_t *termn)
{
    block->termn = termn;
}

void delete_ir_block(ir_block_t *block)
{
    if (!block) {
        return;
    }
    delete_ir_stmt(block->stmt);
    delete_ir_termn(block->termn);
    delete_ir_block(block->next);
    free(block);
}

ir_body_t *new_ir_body(const ir_block_t *inner, ir_item_t *items, ir_local_t *locals)
{
    ir_body_t *ret = new(ir_body_t);
    ret->inner = inner;
    ret->items = items;
    ret->locals = locals;
    return ret;
}

void delete_ir_body(ir_body_t *body)
{
    if (!body) {
        return;
    }
    delete_ir_item(body->items);
    delete_ir_local(body->locals);
    free(body);
}

static ir_item_t *new_ir_item(ir_item_kind_t kind, const ir_type_t *type, symbol_t symbol, region_t name_region)
{
    ir_item_t *ret = new(ir_item_t);
    ret->kind = kind;
    ret->type = type;
    ret->symbol = symbol;
    ret->body = 0;
    ret->next = NULL;
    ret->name_region = name_region;
    ret->refs.head = NULL;
    ret->refs.tail = &ret->refs.head;
    return ret;
}

ir_item_t *new_ir_program_item(const ir_type_t *type, symbol_t symbol, region_t name_region)
{
    return new_ir_item(IR_ITEM_PROGRAM, type, symbol, name_region);
}

ir_item_t *new_ir_procedure_item(const ir_type_t *type, symbol_t symbol, region_t name_region)
{
    return new_ir_item(IR_ITEM_PROCEDURE, type, symbol, name_region);
}

ir_item_t *new_ir_var_item(const ir_type_t *type, symbol_t symbol, region_t name_region)
{
    return new_ir_item(IR_ITEM_VAR, type, symbol, name_region);
}

ir_item_t *new_ir_param_var_item(const ir_type_t *type, symbol_t symbol, region_t name_region)
{
    return new_ir_item(IR_ITEM_ARG_VAR, type, symbol, name_region);
}

ir_item_t *new_ir_local_var_item(const ir_type_t *type, symbol_t symbol, region_t name_region)
{
    return new_ir_item(IR_ITEM_LOCAL_VAR, type, symbol, name_region);
}

void delete_ir_item(ir_item_t *item)
{
    if (!item) {
        return;
    }
    delete_ir_body(item->body);
    delete_ir_item(item->next);
    free(item);
}

void ir_item_add_ref(ir_item_t *item, size_t pos)
{
    ir_item_pos_t *item_pos = new(ir_item_pos_t);
    item_pos->pos = pos;
    item_pos->next = NULL;
    *item->refs.tail = item_pos;
    item->refs.tail = &item_pos->next;
}

ir_t *new_ir(const source_t *source, ir_item_t *items, ir_block_t *blocks, ir_constant_t *constants, ir_type_t *types)
{
    ir_t *ret = new(ir_t);
    ret->source = source;
    ret->items = items;
    ret->blocks = blocks;
    ret->constants = constants;
    ret->types = types;
    return ret;
}

void delete_ir(ir_t *ir)
{
    if (!ir) {
        return;
    }
    delete_ir_item(ir->items);
    delete_ir_block(ir->blocks);
    delete_ir_type(ir->types);
    free(ir);
}
