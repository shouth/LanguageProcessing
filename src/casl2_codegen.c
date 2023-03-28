#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "mppl.h"
#include "utility.h"

typedef size_t codegen_addr_t;

typedef struct {
  FILE *file;
  struct {
    codegen_addr_t cnt;
    hash_t        *table;
  } addr;
  char label[16];
  struct {
    int e_ov, e_rng, e_div0;
    int r_int, r_char, r_ln;
    int w_int, w_char, w_bool, w_str;
  } builtin;
} codegen_t;

codegen_addr_t codegen_addr_lookup(codegen_t *codegen, const void *ptr)
{
  const hash_entry_t *entry;
  assert(codegen);
  if (ptr && (entry = hash_find(codegen->addr.table, ptr))) {
    return (codegen_addr_t) entry->value;
  } else {
    return 0;
  }
}

codegen_addr_t codegen_addr(codegen_t *codegen, const void *ptr)
{
  codegen_addr_t addr;
  assert(codegen);
  if (!(addr = codegen_addr_lookup(codegen, ptr))) {
    addr = ++codegen->addr.cnt;
    if (ptr) {
      hash_insert_unsafe(codegen->addr.table, (void *) ptr, (void *) addr);
    }
  }
  return addr;
}

const char *codegen_label(codegen_addr_t addr)
{
  static char buf[16];
  sprintf(buf, "L%ld", addr);
  return buf;
}

const char *codegen_addr_label(codegen_t *codegen, const void *ptr)
{
  return codegen_label(codegen_addr(codegen, ptr));
}

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

void codegen_set_label(codegen_t *codegen, const char *label)
{
  if (codegen->label[0]) {
    codegen_print(codegen, "DS", "0");
  }
  strcpy(codegen->label, label);
}

void codegen_constant(codegen_t *codegen, const ir_constant_t *constant)
{
  assert(codegen);

  while (constant) {
    switch (constant->kind) {
    case IR_CONSTANT_STRING: {
      const symbol_t *symbol = constant->u.string_constant.value;
      codegen_set_label(codegen, codegen_addr_label(codegen, constant));
      codegen_print(codegen, "DC", "\'%.*s\'", (int) symbol->len, symbol->ptr);
      break;
    }
    }
    constant = constant->next;
  }
}

void codegen_check_range(codegen_t *codegen, const char *reg, const ir_local_t *local)
{
  const ir_type_t *type = ir_local_type(local);
  ++codegen->builtin.e_rng;
  if (local->kind == IR_LOCAL_VAR
    && ir_type_is_kind(type, IR_TYPE_ARRAY)
    && ir_type_is_std(type->u.array_type.base_type->u.ref)) {
    if (type->u.array_type.size) {
      codegen_print(codegen, "LD", "GR0, %s", reg);
      codegen_print(codegen, "JMI", "ERNG");
      codegen_print(codegen, "LAD", "GR0, %ld", type->u.array_type.size - 1);
      codegen_print(codegen, "CPA", "%s, GR0", reg);
      codegen_print(codegen, "JPL", "ERNG");
    } else {
      codegen_print(codegen, "JPL", "ERNG");
    }
  } else {
    unreachable();
  }
}

void codegen_load(codegen_t *codegen, const char *reg, const ir_operand_t *operand);

void codegen_load_constant(codegen_t *codegen, const char *reg, const ir_constant_t *constant)
{
  assert(codegen && reg && constant);
  switch (constant->kind) {
  case IR_CONSTANT_NUMBER:
    codegen_print(codegen, "LAD", "%s, %ld", reg, constant->u.number_constant.value);
    break;
  case IR_CONSTANT_CHAR:
    codegen_print(codegen, "LAD", "%s, #%04X", reg, constant->u.char_constant.value);
    break;
  case IR_CONSTANT_BOOLEAN:
    codegen_print(codegen, "LAD", "%s, %d", reg, constant->u.boolean_constant.value);
    break;
  default:
    unreachable();
  }
}

void codegen_load_place(codegen_t *codegen, const char *reg, const ir_place_t *place)
{
  assert(codegen && reg && place);
  if (place->place_access) {
    switch (place->place_access->kind) {
    case IR_PLACE_ACCESS_INDEX:
      codegen_load(codegen, "GR7", place->place_access->u.index_place_access.index);
      codegen_check_range(codegen, "GR7", place->local);
      switch (place->local->kind) {
      case IR_LOCAL_VAR:
        codegen_print(codegen, "LD", "%s, %s, GR7", reg, codegen_addr_label(codegen, place->local->u.var.item));
        break;
      default:
        unreachable();
      }
      break;
    default:
      unreachable();
    }
  } else {
    switch (place->local->kind) {
    case IR_LOCAL_VAR:
      codegen_print(codegen, "LD", "%s, %s", reg, codegen_addr_label(codegen, place->local->u.var.item));
      break;
    case IR_LOCAL_ARG:
      codegen_print(codegen, "LD", "GR7, %s", codegen_addr_label(codegen, place->local->u.arg.item));
      codegen_print(codegen, "LD", "%s, 0, GR7", reg);
      break;
    case IR_LOCAL_TEMP:
      codegen_print(codegen, "POP", "%s", reg);
      break;
    default:
      unreachable();
    }
  }
}

void codegen_load(codegen_t *codegen, const char *reg, const ir_operand_t *operand)
{
  assert(codegen && reg && operand);

  switch (operand->kind) {
  case IR_OPERAND_CONSTANT: {
    codegen_load_constant(codegen, reg, operand->u.constant_operand.constant);
    break;
  }
  case IR_OPERAND_PLACE: {
    codegen_load_place(codegen, reg, operand->u.place_operand.place);
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
    switch (place->place_access->kind) {
    case IR_PLACE_ACCESS_INDEX:
      codegen_load(codegen, "GR7", place->place_access->u.index_place_access.index);
      codegen_check_range(codegen, "GR7", place->local);
      switch (local->kind) {
      case IR_LOCAL_VAR:
        codegen_print(codegen, "ST", "%s, %s, GR7", reg, codegen_addr_label(codegen, local->u.var.item));
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
      codegen_print(codegen, "ST", "%s, %s", reg, codegen_addr_label(codegen, local->u.var.item));
      break;
    case IR_LOCAL_ARG:
      codegen_print(codegen, "LD", "GR7, %s", codegen_addr_label(codegen, local->u.arg.item));
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

void codegen_stmt_assign(codegen_t *codegen, const ir_stmt_assign_t *stmt)
{
  assert(codegen && stmt);

  switch (stmt->rhs->kind) {
  case IR_RVALUE_USE:
    codegen_load(codegen, "GR1", stmt->rhs->u.use_rvalue.operand);
    break;
  case IR_RVALUE_EXPR_BINARY_KIND:
    codegen_load(codegen, "GR2", stmt->rhs->u.binary_op_rvalue.rhs);
    codegen_load(codegen, "GR1", stmt->rhs->u.binary_op_rvalue.lhs);
    switch (stmt->rhs->u.binary_op_rvalue.kind) {
    case AST_EXPR_BINARY_KIND_PLUS:
      ++codegen->builtin.e_ov;
      codegen_print(codegen, "ADDA", "GR1, GR2");
      codegen_print(codegen, "JOV", "EOV");
      break;
    case AST_EXPR_BINARY_KIND_MINUS:
      ++codegen->builtin.e_ov;
      codegen_print(codegen, "SUBA", "GR1, GR2");
      codegen_print(codegen, "JOV", "EOV");
      break;
    case AST_EXPR_BINARY_KIND_STAR:
      ++codegen->builtin.e_ov;
      codegen_print(codegen, "MULA", "GR1, GR2");
      codegen_print(codegen, "JOV", "EOV");
      break;
    case AST_EXPR_BINARY_KIND_DIV:
      ++codegen->builtin.e_div0;
      codegen_print(codegen, "LD", "GR2, GR2");
      codegen_print(codegen, "JZE", "EDIV0");
      codegen_print(codegen, "DIVA", "GR1, GR2");
      break;
    case AST_EXPR_BINARY_KIND_AND:
      codegen_print(codegen, "AND", "GR1, GR2");
      break;
    case AST_EXPR_BINARY_KIND_OR:
      codegen_print(codegen, "OR", "GR1, GR2");
      break;
    case AST_EXPR_BINARY_KIND_EQUAL: {
      const char *jmp = codegen_addr_label(codegen, NULL);
      codegen_print(codegen, "CPA", "GR1, GR2");
      codegen_print(codegen, "LAD", "GR1, 1");
      codegen_print(codegen, "JZE", "%s", jmp);
      codegen_print(codegen, "XOR", "GR1, GR1");
      codegen_set_label(codegen, jmp);
      break;
    }
    case AST_EXPR_BINARY_KIND_NOTEQ: {
      const char *jmp = codegen_addr_label(codegen, NULL);
      codegen_print(codegen, "SUBA", "GR1, GR2");
      codegen_print(codegen, "JZE", "%s", jmp);
      codegen_print(codegen, "LAD", "GR1, 1");
      codegen_set_label(codegen, jmp);
      break;
    }
    case AST_EXPR_BINARY_KIND_LE: {
      const char *jmp = codegen_addr_label(codegen, NULL);
      codegen_print(codegen, "CPA", "GR1, GR2");
      codegen_print(codegen, "LAD", "GR1, 1");
      codegen_print(codegen, "JMI", "%s", jmp);
      codegen_print(codegen, "XOR", "GR1, GR1");
      codegen_set_label(codegen, jmp);
      break;
    }
    case AST_EXPR_BINARY_KIND_LEEQ: {
      const char *jmp = codegen_addr_label(codegen, NULL);
      codegen_print(codegen, "CPA", "GR2, GR1");
      codegen_print(codegen, "XOR", "GR1, GR1");
      codegen_print(codegen, "JMI", "%s", jmp);
      codegen_print(codegen, "LAD", "GR1, 1");
      codegen_set_label(codegen, jmp);
      break;
    }
    case AST_EXPR_BINARY_KIND_GR: {
      const char *jmp = codegen_addr_label(codegen, NULL);
      codegen_print(codegen, "CPA", "GR2, GR1");
      codegen_print(codegen, "LAD", "GR1, 1");
      codegen_print(codegen, "JMI", "%s", jmp);
      codegen_print(codegen, "XOR", "GR1, GR1");
      codegen_set_label(codegen, jmp);
      break;
    }
    case AST_EXPR_BINARY_KIND_GREQ: {
      const char *jmp = codegen_addr_label(codegen, NULL);
      codegen_print(codegen, "CPA", "GR1, GR2");
      codegen_print(codegen, "XOR", "GR1, GR1");
      codegen_print(codegen, "JMI", "%s", jmp);
      codegen_print(codegen, "LAD", "GR1, 1");
      codegen_set_label(codegen, jmp);
      break;
    }
    default:
      unreachable();
    }
    break;
  case IR_RVALUE_UNARY_OP:
    switch (stmt->rhs->u.unary_op_rvalue.kind) {
    case AST_EXPR_UNARY_KIND_NOT:
      codegen_load(codegen, "GR1", stmt->rhs->u.unary_op_rvalue.value);
      codegen_print(codegen, "XOR", "GR1, BC1");
      break;
    default:
      unreachable();
    }
    break;
  case IR_RVALUE_CAST:
    codegen_load(codegen, "GR1", stmt->rhs->u.cast_rvalue.value);
    switch (ir_operand_type(stmt->rhs->u.cast_rvalue.value)->kind) {
    case IR_TYPE_INTEGER:
      switch (stmt->rhs->u.cast_rvalue.type->kind) {
      case IR_TYPE_BOOLEAN: {
        const char *jmp = codegen_addr_label(codegen, NULL);
        codegen_print(codegen, "LD", "GR1, GR1");
        codegen_print(codegen, "JZE", "%s", jmp);
        codegen_print(codegen, "LAD", "GR1, 1");
        codegen_set_label(codegen, jmp);
        break;
      }
      case IR_TYPE_INTEGER:
      case IR_TYPE_CHAR:
        codegen_print(codegen, "LAD", "GR2, #007f");
        codegen_print(codegen, "AND", "GR1, GR2");
        break;
      default:
        unreachable();
      }
      break;
    case IR_TYPE_CHAR:
      switch (stmt->rhs->u.cast_rvalue.type->kind) {
      case IR_TYPE_BOOLEAN: {
        const char *jmp = codegen_addr_label(codegen, NULL);
        codegen_print(codegen, "LD", "GR1, GR1");
        codegen_print(codegen, "JZE", "%s", jmp);
        codegen_print(codegen, "LAD", "GR1, 1");
        codegen_set_label(codegen, jmp);
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
  codegen_store(codegen, "GR1", stmt->lhs);
}

void codegen_stmt_call(codegen_t *codegen, const ir_stmt_call_t *stmt)
{
  assert(codegen && stmt);
  assert(!stmt->func->place_access);

  switch (stmt->func->local->kind) {
  case IR_LOCAL_VAR:
    codegen_print(codegen, "CALL", "%s", codegen_addr_label(codegen, stmt->func->local->u.var.item->body));
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
    codegen_addr_t tmp   = codegen_addr(codegen, NULL);
    codegen_addr_t label = codegen_addr(codegen, NULL);
    codegen_print(codegen, "JUMP", codegen_label(label));
    codegen_set_label(codegen, codegen_label(tmp));
    codegen_print(codegen, "DS", "1");
    codegen_set_label(codegen, codegen_label(label));
    codegen_print(codegen, "LAD", "GR0, %ld", constant->u.number_constant.value);
    codegen_print(codegen, "ST", "GR0, %s", codegen_label(tmp));
    codegen_print(codegen, "PUSH", "%s", codegen_label(tmp));
    break;
  }
  case IR_CONSTANT_CHAR: {
    codegen_addr_t tmp   = codegen_addr(codegen, NULL);
    codegen_addr_t label = codegen_addr(codegen, NULL);
    codegen_print(codegen, "JUMP", codegen_label(label));
    codegen_set_label(codegen, codegen_label(tmp));
    codegen_print(codegen, "DS", "1");
    codegen_set_label(codegen, codegen_label(label));
    codegen_print(codegen, "LAD", "GR0, #%04x", constant->u.char_constant.value);
    codegen_print(codegen, "ST", "GR0, %s", codegen_label(tmp));
    codegen_print(codegen, "PUSH", "%s", codegen_label(tmp));
    break;
  }
  case IR_CONSTANT_BOOLEAN: {
    codegen_addr_t tmp   = codegen_addr(codegen, NULL);
    codegen_addr_t label = codegen_addr(codegen, NULL);
    codegen_print(codegen, "JUMP", codegen_label(label));
    codegen_set_label(codegen, codegen_label(tmp));
    codegen_print(codegen, "DS", "1");
    codegen_set_label(codegen, codegen_label(label));
    codegen_print(codegen, "LAD", "GR0, %ld", constant->u.boolean_constant.value);
    codegen_print(codegen, "ST", "GR0, %s", codegen_label(tmp));
    codegen_print(codegen, "PUSH", "%s", codegen_label(tmp));
    break;
  }
  default:
    unreachable();
  }
}

void codegen_push_place_address(codegen_t *codegen, const ir_place_t *place)
{
  assert(codegen && place);

  if (place->place_access) {
    switch (place->place_access->kind) {
    case IR_PLACE_ACCESS_INDEX:
      codegen_load(codegen, "GR7", place->place_access->u.index_place_access.index);
      codegen_check_range(codegen, "GR7", place->local);
      switch (place->local->kind) {
      case IR_LOCAL_VAR:
        codegen_print(codegen, "PUSH", "%s, GR7", codegen_addr_label(codegen, place->local->u.var.item));
        break;
      default:
        unreachable();
      }
    default:
      unreachable();
    }
  } else {
    switch (place->local->kind) {
    case IR_LOCAL_VAR:
      codegen_print(codegen, "PUSH", "%s", codegen_addr_label(codegen, place->local->u.var.item));
      break;
    case IR_LOCAL_ARG:
      codegen_print(codegen, "LD", "GR7, %s", codegen_addr_label(codegen, place->local->u.arg.item));
      codegen_print(codegen, "PUSH", "0, GR7");
      break;
    case IR_LOCAL_TEMP: {
      codegen_addr_t tmp   = codegen_addr(codegen, NULL);
      codegen_addr_t label = codegen_addr(codegen, NULL);
      codegen_print(codegen, "JUMP", "%s", codegen_label(label));
      codegen_set_label(codegen, codegen_label(tmp));
      codegen_print(codegen, "DS", "1");
      codegen_set_label(codegen, codegen_label(label));
      codegen_print(codegen, "POP", "GR1");
      codegen_print(codegen, "ST", "GR1, %s", codegen_label(tmp));
      codegen_print(codegen, "PUSH", "%s", codegen_label(tmp));
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

void codegen_stmt_read(codegen_t *codegen, const ir_stmt_read_t *stmt)
{
  assert(codegen && stmt);

  if (stmt->ref->place_access) {
    switch (stmt->ref->local->kind) {
    case IR_LOCAL_VAR:
      codegen_load(codegen, "GR7", stmt->ref->place_access->u.index_place_access.index);
      break;
    default:
      unreachable();
    }
  } else {
    switch (stmt->ref->local->kind) {
    case IR_LOCAL_VAR:
      codegen_print(codegen, "LAD", "GR7, %s", codegen_addr_label(codegen, stmt->ref->local->u.var.item));
      break;
    case IR_LOCAL_ARG:
      codegen_print(codegen, "LD", "GR7, %s", codegen_addr_label(codegen, stmt->ref->local->u.arg.item));
      break;
    default:
      unreachable();
    }
  }

  /* call builtin `read` functions for each types */
  switch (ir_place_type(stmt->ref)->kind) {
  case IR_TYPE_INTEGER:
    codegen->builtin.r_int++;
    codegen_print(codegen, "CALL", "BRINT");
    break;
  case IR_TYPE_CHAR:
    codegen->builtin.r_char++;
    codegen_print(codegen, "CALL", "BRCHAR");
    break;
  default:
    unreachable();
  }
}

void codegen_stmt_write(codegen_t *codegen, const ir_stmt_write_t *stmt)
{
  assert(codegen && stmt);

  /* call builtin `write` functions for each types */
  switch (ir_operand_type(stmt->value)->kind) {
  case IR_TYPE_INTEGER:
    codegen->builtin.w_int++;
    codegen_load(codegen, "GR1", stmt->value);
    codegen_print(codegen, "CALL", "BSINT");
    if (stmt->len) {
      codegen_load_constant(codegen, "GR1", stmt->len);
    } else {
      codegen_print(codegen, "LD", "GR1, GR2");
    }
    codegen_print(codegen, "CALL", "BWSTR");
    break;
  case IR_TYPE_BOOLEAN:
    codegen->builtin.w_bool++;
    codegen_load(codegen, "GR1", stmt->value);
    codegen_print(codegen, "CALL", "BSBOOL");
    if (stmt->len) {
      codegen_load_constant(codegen, "GR1", stmt->len);
    } else {
      codegen_print(codegen, "LD", "GR1, GR2");
    }
    codegen_print(codegen, "CALL", "BWSTR");
    break;
  case IR_TYPE_CHAR:
    codegen->builtin.w_char++;
    codegen_load(codegen, "GR1", stmt->value);
    codegen_print(codegen, "CALL", "BSCHAR");
    if (stmt->len) {
      codegen_load_constant(codegen, "GR1", stmt->len);
    } else {
      codegen_print(codegen, "LAD", "GR1, 1");
    }
    codegen_print(codegen, "CALL", "BWSTR");
    break;
  case IR_TYPE_ARRAY: {
    const ir_constant_t *constant = stmt->value->u.constant_operand.constant;
    const ir_type_t     *type     = ir_constant_type(constant);
    codegen->builtin.w_str++;
    codegen_print(codegen, "LAD", "GR2, %ld", type->u.array_type.size);
    codegen_print(codegen, "LAD", "GR3, %s", codegen_addr_label(codegen, constant));
    codegen_print(codegen, "LD", "GR1, GR2");
    codegen_print(codegen, "CALL", "BWSTR");
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
      codegen_stmt_assign(codegen, &stmt->u.stmt_assign);
      break;
    case IR_STMT_CALL:
      codegen_stmt_call(codegen, &stmt->u.stmt_call);
      break;
    case IR_STMT_READ:
      codegen_stmt_read(codegen, &stmt->u.stmt_read);
      break;
    case IR_STMT_READLN:
      codegen->builtin.r_ln++;
      codegen_print(codegen, "CALL", "BRLN");
      break;
    case IR_STMT_WRITE:
      codegen_stmt_write(codegen, &stmt->u.stmt_write);
      break;
    case IR_STMT_WRITELN:
      codegen->builtin.w_char++;
      codegen_print(codegen, "LD", "GR1, BCLF");
      codegen_print(codegen, "CALL", "BSCHAR");
      codegen_print(codegen, "LAD", "GR1, 1");
      codegen_print(codegen, "CALL", "BWSTR");
      break;
    }
    stmt = stmt->next;
  }
}

void codegen_block(codegen_t *codegen, const ir_block_t *block)
{
  assert(codegen && block);

  codegen_set_label(codegen, codegen_addr_label(codegen, block));
  codegen_stmt(codegen, block->stmt);
  switch (block->termn.kind) {
  case IR_TERMN_GOTO: {
    const ir_block_t *next = block->termn.u.goto_termn.next;
    if (codegen_addr_lookup(codegen, next)) {
      codegen_print(codegen, "JUMP", "%s", codegen_addr_label(codegen, next));
    } else {
      codegen_block(codegen, next);
    }
    break;
  }
  case IR_TERMN_IF: {
    const ir_operand_t *cond = block->termn.u.if_termn.cond;
    const ir_block_t   *then = block->termn.u.if_termn.then;
    const ir_block_t   *els  = block->termn.u.if_termn.els;

    codegen_load(codegen, "GR1", cond);
    codegen_print(codegen, "LD", "GR1, GR1");
    if (codegen_addr_lookup(codegen, then)) {
      codegen_print(codegen, "JNZ", "%s", codegen_addr_label(codegen, then));
      if (codegen_addr_lookup(codegen, els)) {
        codegen_print(codegen, "JUMP", "%s", codegen_addr_label(codegen, els));
      } else {
        codegen_block(codegen, els);
      }
    } else {
      if (codegen_addr_lookup(codegen, els)) {
        codegen_print(codegen, "JZE", "%s", codegen_addr_label(codegen, els));
        codegen_block(codegen, then);
      } else {
        codegen_print(codegen, "JZE", "%s", codegen_addr_label(codegen, els));
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

void codegen_item(codegen_t *codegen, const ir_item_t *item)
{
  assert(codegen);

  while (item) {
    switch (item->kind) {
    case IR_ITEM_PROGRAM:
      codegen_print(codegen, "CALL", codegen_addr_label(codegen, item->body->inner));
      codegen_print(codegen, "SVC", "0");
      codegen_item(codegen, item->body->items);
      codegen_block(codegen, item->body->inner);
      break;
    case IR_ITEM_PROCEDURE:
      codegen_item(codegen, item->body->items);
      codegen_set_label(codegen, codegen_addr_label(codegen, item->body));
      {
        ir_item_t *items = item->body->items;
        codegen_print(codegen, "POP", "GR2");
        while (items) {
          if (items->kind == IR_ITEM_ARG_VAR) {
            codegen_print(codegen, "POP", "GR1");
            codegen_print(codegen, "ST", "GR1, %s", codegen_addr_label(codegen, items));
          }
          items = items->next;
        }
        codegen_print(codegen, "PUSH", "0, GR2");
        codegen_block(codegen, item->body->inner);
      }
      break;
    case IR_ITEM_VAR:
    case IR_ITEM_LOCAL_VAR:
      codegen_set_label(codegen, codegen_addr_label(codegen, item));
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
      codegen_set_label(codegen, codegen_addr_label(codegen, item));
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
  int builtin_write = 0;
  int builtin_read  = 0;
  codegen_set_label(codegen, "BCSP");
  codegen_print(codegen, "DC", "#%04X", (int) ' ');
  codegen_set_label(codegen, "BCLF");
  codegen_print(codegen, "DC", "#%04X", (int) '\n');
  codegen_set_label(codegen, "BCTAB");
  codegen_print(codegen, "DC", "#%04X", (int) '\t');
  codegen_set_label(codegen, "BC1");
  codegen_print(codegen, "DC", "1");
  codegen_set_label(codegen, "BC10");
  codegen_print(codegen, "DC", "10");
  codegen_set_label(codegen, "BCH30");
  codegen_print(codegen, "DC", "#0030");

  if (codegen->builtin.w_int) {
    builtin_write = 1;
    codegen_set_label(codegen, "BSINT");
    codegen_print(codegen, "LAD", "GR4, 6");
    codegen_print(codegen, "XOR", "GR5, GR5");
    codegen_print(codegen, "CPA", "GR1, GR5");
    codegen_print(codegen, "JPL", "BSINT0");
    codegen_print(codegen, "SUBA", "GR5, GR1");
    codegen_print(codegen, "LD", "GR1, GR5");

    codegen_set_label(codegen, "BSINT0");
    codegen_print(codegen, "LD", "GR2, GR1");
    codegen_print(codegen, "LD", "GR3, GR1");
    codegen_print(codegen, "DIVA", "GR3, BC10");
    codegen_print(codegen, "MULA", "GR3, BC10");
    codegen_print(codegen, "SUBA", "GR2, GR3");
    codegen_print(codegen, "ADDA", "GR2, BCH30");
    codegen_print(codegen, "SUBA", "GR4, BC1");
    codegen_print(codegen, "ST", "GR2, BSBUF, GR4");
    codegen_print(codegen, "DIVA", "GR1, BC10");
    codegen_print(codegen, "JNZ", "BSINT0");
    codegen_print(codegen, "LD", "GR5, GR5");
    codegen_print(codegen, "JZE", "BSINT1");
    codegen_print(codegen, "LAD", "GR2, #%04X", (int) '-');
    codegen_print(codegen, "SUBA", "GR4, BC1");
    codegen_print(codegen, "ST", "GR2, BSBUF, GR4");

    codegen_set_label(codegen, "BSINT1");
    codegen_print(codegen, "LAD", "GR2, 6");
    codegen_print(codegen, "SUBA", "GR2, GR4");
    codegen_print(codegen, "LAD", "GR3, BSBUF, GR4");
    codegen_print(codegen, "RET", NULL);
  }

  if (codegen->builtin.w_bool) {
    builtin_write = 1;
    codegen_set_label(codegen, "BCTRUE");
    codegen_print(codegen, "DC", "'TRUE'");
    codegen_set_label(codegen, "BCFALSE");
    codegen_print(codegen, "DC", "'FALSE'");

    codegen_set_label(codegen, "BSBOOL");
    codegen_print(codegen, "LD", "GR1, GR1");
    codegen_print(codegen, "JNZ", "BSBOOL0");
    codegen_print(codegen, "LAD", "GR3, BCFALSE");
    codegen_print(codegen, "LAD", "GR2, 5");
    codegen_print(codegen, "RET", NULL);
    codegen_set_label(codegen, "BSBOOL0");
    codegen_print(codegen, "LAD", "GR3, BCTRUE");
    codegen_print(codegen, "LAD", "GR2, 4");
    codegen_print(codegen, "RET", NULL);
  }

  if (codegen->builtin.w_char) {
    builtin_write = 1;
    codegen_set_label(codegen, "BSCHAR");
    codegen_print(codegen, "ST", "GR1, BSBUF");
    codegen_print(codegen, "LAD", "GR3, BSBUF");
    codegen_print(codegen, "LAD", "GR2, 1");
    codegen_print(codegen, "RET", NULL);
  }

  if (codegen->builtin.w_str) {
    builtin_write = 1;
  }

  if (builtin_write) {
    codegen_set_label(codegen, "BSBUF");
    codegen_print(codegen, "DS", "6");
    codegen_set_label(codegen, "BOBUF");
    codegen_print(codegen, "DS", "256");
    codegen_set_label(codegen, "BOCUR");
    codegen_print(codegen, "DC", "0");

    codegen_set_label(codegen, "BFLUSH");
    codegen_print(codegen, "OUT", "BOBUF, BOCUR");
    codegen_print(codegen, "XOR", "GR0, GR0");
    codegen_print(codegen, "ST", "GR0, BOCUR");
    codegen_print(codegen, "RET", NULL);

    codegen_set_label(codegen, "BWSTR");
    codegen_print(codegen, "LD", "GR1, GR1");
    codegen_print(codegen, "JPL", "BWSTR0");
    codegen_print(codegen, "RET", NULL);

    codegen_set_label(codegen, "BWSTR0");
    codegen_print(codegen, "CPA", "GR2, GR1");
    codegen_print(codegen, "JMI", "BWSTR1");
    codegen_print(codegen, "LD", "GR4, 0, GR3");
    codegen_print(codegen, "ADDA", "GR3, BC1");
    codegen_print(codegen, "JUMP", "BWSTR2");

    codegen_set_label(codegen, "BWSTR1");
    codegen_print(codegen, "LD", "GR4, BCSP");

    codegen_set_label(codegen, "BWSTR2");
    codegen_print(codegen, "SUBA", "GR1, BC1");
    codegen_print(codegen, "LD", "GR5, BOCUR");
    codegen_print(codegen, "ST", "GR4, BOBUF, GR5");
    codegen_print(codegen, "ADDA", "GR5, BC1");
    codegen_print(codegen, "ST", "GR5, BOCUR");
    codegen_print(codegen, "CPA", "GR4, BCLF");
    codegen_print(codegen, "JNZ", "BWSTR3");
    codegen_print(codegen, "CALL", "BFLUSH");
    codegen_print(codegen, "JUMP", "BWSTR4");

    codegen_set_label(codegen, "BWSTR3");
    codegen_print(codegen, "LAD", "GR4, 256");
    codegen_print(codegen, "CPA", "GR5, GR4");
    codegen_print(codegen, "JNZ", "BWSTR4");
    codegen_print(codegen, "CALL", "BFLUSH");

    codegen_set_label(codegen, "BWSTR4");
    codegen_print(codegen, "JUMP", "BWSTR");
  }

  if (codegen->builtin.r_int) {
    builtin_read = 1;
    codegen_set_label(codegen, "BRINT");
    codegen_print(codegen, "XOR", "GR0, GR0");
    codegen_print(codegen, "CALL", "BRREAD");

    codegen_set_label(codegen, "BRINT0");
    codegen_print(codegen, "CALL", "BRTOP");
    codegen_print(codegen, "CPA", "GR1, BCSP");
    codegen_print(codegen, "JZE", "BRINT1");
    codegen_print(codegen, "CPA", "GR1, BCLF");
    codegen_print(codegen, "JZE", "BRINT1");
    codegen_print(codegen, "CPA", "GR1, BCTAB");
    codegen_print(codegen, "JZE", "BRINT1");
    codegen_print(codegen, "SUBA", "GR1, BCH30");
    codegen_print(codegen, "JMI", "BRINT2");
    codegen_print(codegen, "CPA", "GR1, BC10");
    codegen_print(codegen, "JPL", "BRINT2");
    codegen_print(codegen, "MULA", "GR0, BC10");
    codegen_print(codegen, "JOV", "EOV");
    codegen_print(codegen, "ADDA", "GR0, GR1");

    codegen_set_label(codegen, "BRINT1");
    codegen_print(codegen, "ADDA", "GR2, BC1");
    codegen_print(codegen, "ST", "GR2, BICUR");
    codegen_print(codegen, "CPA", "GR2, BILEN");
    codegen_print(codegen, "JMI", "BRINT0");

    codegen_set_label(codegen, "BRINT2");
    codegen_print(codegen, "ST", "GR0, 0, GR7");
    codegen_print(codegen, "RET", NULL);
  }

  if (codegen->builtin.r_char) {
    builtin_read = 1;
    codegen_set_label(codegen, "BRCHAR");
    codegen_print(codegen, "CALL", "BRREAD");
    codegen_print(codegen, "CALL", "BRTOP");
    codegen_print(codegen, "ADDA", "GR2, BC1");
    codegen_print(codegen, "ST", "GR2, BICUR");
    codegen_print(codegen, "ST", "GR1, 0, GR7");
    codegen_print(codegen, "RET", NULL);
  }

  if (codegen->builtin.r_ln) {
    builtin_read = 1;
    codegen_set_label(codegen, "BRLN");
    codegen_print(codegen, "XOR", "GR0, GR0");
    codegen_print(codegen, "ST", "GR0, BILEN");
    codegen_print(codegen, "ST", "GR0, BICUR");
    codegen_print(codegen, "RET", NULL);
  }

  if (builtin_read) {
    codegen_set_label(codegen, "BIBUF");
    codegen_print(codegen, "DS", "256");
    codegen_set_label(codegen, "BILEN");
    codegen_print(codegen, "DC", "0");
    codegen_set_label(codegen, "BICUR");
    codegen_print(codegen, "DC", "0");

    codegen_set_label(codegen, "BRREAD");
    codegen_print(codegen, "LD", "GR1, BICUR");
    codegen_print(codegen, "LD", "GR2, BICUR");
    codegen_print(codegen, "CPA", "GR1, BILEN");
    codegen_print(codegen, "JMI", "BRREAD0");
    codegen_print(codegen, "IN", "BIBUF, BILEN");
    codegen_print(codegen, "XOR", "GR0, GR0");
    codegen_print(codegen, "ST", "GR0, BICUR");

    codegen_set_label(codegen, "BRREAD0");
    codegen_print(codegen, "RET", NULL);

    codegen_set_label(codegen, "BRTOP");
    codegen_print(codegen, "LD", "GR1, BICUR");
    codegen_print(codegen, "LD", "GR2, BICUR");
    codegen_print(codegen, "CPA", "GR1, BILEN");
    codegen_print(codegen, "JMI", "BRTOP0");
    codegen_print(codegen, "XOR", "GR1, GR1");
    codegen_print(codegen, "RET", NULL);

    codegen_set_label(codegen, "BRTOP0");
    codegen_print(codegen, "LD", "GR1, BIBUF, GR2");
    codegen_print(codegen, "RET", NULL);
  }

  if (codegen->builtin.e_ov) {
    const char *msg = "runtime error: overflow";
    codegen_set_label(codegen, "EMOV");
    codegen_print(codegen, "DC", "'%s'", msg);
    codegen_set_label(codegen, "EMLOV");
    codegen_print(codegen, "DC", "%ld", strlen(msg));
    codegen_set_label(codegen, "EOV");
    codegen_print(codegen, "CALL", "BFLUSH");
    codegen_print(codegen, "OUT", "EMOV, EMLOV");
    codegen_print(codegen, "OUT", "BCLF, BC1");
    codegen_print(codegen, "SVC", "1");
  }

  if (codegen->builtin.e_div0) {
    const char *msg = "runtime error: division by 0";
    codegen_set_label(codegen, "EMDIV0");
    codegen_print(codegen, "DC", "'%s'", msg);
    codegen_set_label(codegen, "EMLDIV0");
    codegen_print(codegen, "DC", "%ld", strlen(msg));
    codegen_set_label(codegen, "EDIV0");
    codegen_print(codegen, "CALL", "BFLUSH");
    codegen_print(codegen, "OUT", "EMDIV0, EMLDIV0");
    codegen_print(codegen, "OUT", "BCLF, BC1");
    codegen_print(codegen, "SVC", "2");
  }

  if (codegen->builtin.e_rng) {
    const char *msg = "runtime error: index out of range";
    codegen_set_label(codegen, "EMRNG");
    codegen_print(codegen, "DC", "'%s'", msg);
    codegen_set_label(codegen, "EMLRNG");
    codegen_print(codegen, "DC", "%ld", strlen(msg));
    codegen_set_label(codegen, "ERNG");
    codegen_print(codegen, "CALL", "BFLUSH");
    codegen_print(codegen, "OUT", "EMRNG, EMLRNG");
    codegen_print(codegen, "OUT", "BCLF, BC1");
    codegen_print(codegen, "SVC", "3");
  }
}

void codegen_ir(codegen_t *codegen, const ir_t *ir)
{
  codegen_set_label(codegen, "PROGRAM");
  codegen_print(codegen, "START", NULL);
  codegen_item(codegen, ir->items);
  codegen_builtin(codegen);
  codegen_constant(codegen, ir->constants);
  codegen_print(codegen, "END", NULL);
}

void codegen_casl2(const ir_t *ir)
{
  codegen_t codegen;
  assert(ir);

  codegen.file           = fopen(ir->source->out_name, "w");
  codegen.addr.cnt       = 0;
  codegen.addr.table     = hash_new(hash_default_comp, hash_default_hasher);
  codegen.label[0]       = '\0';
  codegen.builtin.e_rng  = 0;
  codegen.builtin.e_ov   = 0;
  codegen.builtin.e_div0 = 0;
  codegen.builtin.r_int  = 0;
  codegen.builtin.r_char = 0;
  codegen.builtin.r_ln   = 0;
  codegen.builtin.w_int  = 0;
  codegen.builtin.w_char = 0;
  codegen.builtin.w_bool = 0;
  codegen_ir(&codegen, ir);
  hash_delete(codegen.addr.table, NULL, NULL);
  fclose(codegen.file);
}
