#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "array.h"
#include "compiler.h"
#include "context.h"
#include "context_fwd.h"
#include "mppl_syntax.h"
#include "mppl_syntax_ext.h"
#include "syntax_kind.h"
#include "syntax_tree.h"
#include "utility.h"

typedef unsigned long    Temp;
typedef unsigned long    Label;
typedef struct Str       Str;
typedef struct Ptr       Ptr;
typedef struct Generator Generator;

#define LABEL_NULL   ((Label) 0)
#define LABEL_RETURN ((Label) 0 - 1)
#define LABEL_BREAK  ((Label) 0 - 2)

struct Str {
  Array        *chars;
  unsigned long length;
};

struct Ptr {
  int         is_temporal;
  const Type *type;
  union {
    Temp       temporal;
    const Def *def;
  } ptr;
};

struct Generator {
  Ctx  *ctx;
  FILE *file;

  Temp   temp;
  Label  block;
  Label  break_label;
  Array *strs;
};

unsigned long type_width(const Type *type)
{
  switch (type_kind(type)) {
  case TYPE_INTEGER:
    return 16;

  case TYPE_CHAR:
    return 8;

  case TYPE_BOOLEAN:
    return 1;

  default:
    unreachable();
  }
}

Str str(void)
{
  Str str;
  str.chars  = array_new(sizeof(char));
  str.length = 0;
  return str;
}

void str_free(Str *self)
{
  array_free(self->chars);
}

void str_push(Str *self, const char *string)
{
  unsigned long i;

  for (i = 0; string[i]; ++i) {
    if (string[i] == '\\') {
      i += string[i + 1] == '\\' ? 1 : 2;
    }
    self->length += 1;
  }

  array_push_count(self->chars, (void *) string, i);
}

void write(Generator *self, const char *format, ...)
{
  va_list args;
  va_start(args, format);
  vfprintf(self->file, format, args);
  va_end(args);
}

void write_inst(Generator *self, const char *format, ...)
{
  va_list args;
  va_start(args, format);
  fprintf(self->file, "  ");
  vfprintf(self->file, format, args);
  fprintf(self->file, "\n");
  va_end(args);
}

void write_label(Generator *self, Label label)
{
  fprintf(self->file, "l%lu:\n", label);
}

void write_expr(Generator *self, Temp result, const AnyMpplExpr *expr);

void write_arithmetic_expr(Generator *self, Temp result, const char *inst, const AnyMpplExpr *lhs_syntax, const AnyMpplExpr *rhs_syntax)
{
  Temp lhs_temporal = self->temp++;
  Temp rhs_temporal = self->temp++;

  write_expr(self, lhs_temporal, lhs_syntax);
  write_expr(self, rhs_temporal, rhs_syntax);
  write_inst(self, "%%.t%lu = %s i16 %%.t%lu, %%.t%lu", result, inst, lhs_temporal, rhs_temporal);
}

void write_relational_expr(Generator *self, Temp result, const char *inst, const AnyMpplExpr *lhs_syntax, const AnyMpplExpr *rhs_syntax)
{
  const Type   *type          = ctx_type_of(self->ctx, (const SyntaxTree *) rhs_syntax, NULL);
  unsigned long operand_width = type_width(type);

  Temp lhs_temporal = self->temp++;
  Temp rhs_temporal = self->temp++;

  write_expr(self, lhs_temporal, lhs_syntax);
  write_expr(self, rhs_temporal, rhs_syntax);
  write_inst(self, "%%.t%lu = icmp %s i%lu %%.t%lu, %%.t%lu", result, inst, operand_width, lhs_temporal, rhs_temporal);
}

void write_logical_expr(Generator *self, Temp result, int jumpon, const AnyMpplExpr *lhs_syntax, const AnyMpplExpr *rhs_syntax)
{
  Label then_label = self->block++;
  Label next_label = self->block++;

  Temp temporal     = self->temp++;
  Temp lhs_temporal = self->temp++;
  Temp rhs_temporal = self->temp++;

  write_inst(self, "%%.t%lu = alloca i1", temporal);
  write_expr(self, lhs_temporal, lhs_syntax);
  write_inst(self, "store i1 %%.t%lu, ptr %%.t%lu", lhs_temporal, temporal);
  if (jumpon) {
    write_inst(self, "br i1 %%.t%lu, label %%l%lu, label %%l%lu", lhs_temporal, next_label, then_label);
  } else {
    write_inst(self, "br i1 %%.t%lu, label %%l%lu, label %%l%lu", lhs_temporal, then_label, next_label);
  }
  write(self, "\n");

  write_label(self, then_label);
  write_expr(self, rhs_temporal, rhs_syntax);
  write_inst(self, "store i1 %%.t%lu, ptr %%.t%lu", rhs_temporal, temporal);
  write_inst(self, "br label %%l%lu", next_label);
  write(self, "\n");

  write_label(self, next_label);
  write_inst(self, "%%.t%lu = load i1, ptr %%.t%lu", result, temporal);
}

void write_binary_expr(Generator *self, Temp result, const MpplBinaryExpr *expr)
{
  AnyMpplExpr *lhs_syntax = mppl_binary_expr__lhs(expr);
  AnyMpplExpr *rhs_syntax = mppl_binary_expr__rhs(expr);
  MpplToken   *op_token   = mppl_binary_expr__op_token(expr);

  if (lhs_syntax) {
    switch (syntax_tree_kind((SyntaxTree *) op_token)) {
    case SYNTAX_PLUS_TOKEN:
      write_arithmetic_expr(self, result, "add", lhs_syntax, rhs_syntax);
      break;

    case SYNTAX_MINUS_TOKEN:
      write_arithmetic_expr(self, result, "sub", lhs_syntax, rhs_syntax);
      break;

    case SYNTAX_STAR_TOKEN:
      write_arithmetic_expr(self, result, "mul", lhs_syntax, rhs_syntax);
      break;

    case SYNTAX_DIV_KW:
      write_arithmetic_expr(self, result, "sdiv", lhs_syntax, rhs_syntax);
      break;

    case SYNTAX_EQUAL_TOKEN:
      write_relational_expr(self, result, "eq", lhs_syntax, rhs_syntax);
      break;

    case SYNTAX_NOTEQ_TOKEN:
      write_relational_expr(self, result, "ne", lhs_syntax, rhs_syntax);
      break;

    case SYNTAX_LESS_TOKEN:
      write_relational_expr(self, result, "slt", lhs_syntax, rhs_syntax);
      break;

    case SYNTAX_LESSEQ_TOKEN:
      write_relational_expr(self, result, "sle", lhs_syntax, rhs_syntax);
      break;

    case SYNTAX_GREATER_TOKEN:
      write_relational_expr(self, result, "sgt", lhs_syntax, rhs_syntax);
      break;

    case SYNTAX_GREATEREQ_TOKEN:
      write_relational_expr(self, result, "sge", lhs_syntax, rhs_syntax);
      break;

    case SYNTAX_AND_KW:
      write_logical_expr(self, result, 0, lhs_syntax, rhs_syntax);
      break;

    case SYNTAX_OR_KW:
      write_logical_expr(self, result, 1, lhs_syntax, rhs_syntax);
      break;

    default:
      unreachable();
    }
  } else {
    switch (syntax_tree_kind((SyntaxTree *) op_token)) {
    case SYNTAX_PLUS_TOKEN:
      write_expr(self, result, rhs_syntax);
      break;

    case SYNTAX_MINUS_TOKEN: {
      Temp rhs_temporal = self->temp++;
      write_expr(self, rhs_temporal, rhs_syntax);
      write_inst(self, "%%.t%lu = sub i16 0, %%.t%lu", result, rhs_temporal);
      break;
    }

    default:
      unreachable();
    }
  }

  mppl_unref(op_token);
  mppl_unref(lhs_syntax);
  mppl_unref(rhs_syntax);
}

void write_not_expr(Generator *self, Temp result, const MpplNotExpr *expr)
{
  AnyMpplExpr *operand_syntax   = mppl_not_expr__expr(expr);
  Temp         operand_temporal = self->temp++;

  write_expr(self, operand_temporal, operand_syntax);
  write_inst(self, "%%.t%lu = xor i1 %%.t%lu, 1", result, operand_temporal);

  mppl_unref(operand_syntax);
}

void write_paren_expr(Generator *self, Temp result, const MpplParenExpr *expr)
{
  AnyMpplExpr *operand_syntax = mppl_paren_expr__expr(expr);
  write_expr(self, result, operand_syntax);
  mppl_unref(operand_syntax);
}

void write_cast_expr(Generator *self, Temp result, const MpplCastExpr *expr)
{
  AnyMpplExpr *operand_syntax = mppl_cast_expr__expr(expr);
  const Type  *expr_type      = ctx_type_of(self->ctx, (const SyntaxTree *) expr, NULL);
  const Type  *operand_type   = ctx_type_of(self->ctx, (const SyntaxTree *) operand_syntax, NULL);

  if (expr_type == operand_type) {
    write_expr(self, result, operand_syntax);
  } else {
    Temp operand_temporal = self->temp++;
    write_expr(self, operand_temporal, operand_syntax);

    switch (type_kind(expr_type)) {
    case TYPE_BOOLEAN: {
      write_inst(self, "%%.t%lu = icmp ne i16 %%.t%lu, 0", result, operand_temporal);
      break;
    }

    case TYPE_INTEGER: {
      unsigned long operand_width = type_width(operand_type);
      write_inst(self, "%%.t%lu = zext i%lu %%.t%lu to i16", result, operand_width, operand_temporal);
      break;
    }

    case TYPE_CHAR: {
      switch (type_kind(operand_type)) {
      case TYPE_BOOLEAN:
        write_inst(self, "%%.t%lu = zext i1 %%.t%lu to i8", result, operand_temporal);
        break;

      case TYPE_INTEGER:
        write_inst(self, "%%.t%lu = trunc i16 %%.t%lu to i8", result, operand_temporal);
        break;

      default:
        unreachable();
      }
      break;
    }

    default:
      unreachable();
    }
  }

  mppl_unref(operand_syntax);
}

void write_var_expr(Generator *self, Temp result, const AnyMpplVar *var)
{
  switch (mppl_var__kind(var)) {
  case MPPL_VAR_ENTIRE: {
    const MpplEntireVar  *entire_var_syntax = (const MpplEntireVar *) var;
    MpplToken            *name_token        = mppl_entire_var__name(entire_var_syntax);
    const RawSyntaxToken *raw_name_token    = (const RawSyntaxToken *) syntax_tree_raw((const SyntaxTree *) name_token);
    const Def            *def               = ctx_resolve(self->ctx, (const SyntaxTree *) name_token, NULL);
    const Type           *type              = ctx_type_of(self->ctx, def_syntax(def), NULL);

    switch (def_kind(def)) {
    case DEF_LOCAL:
    case DEF_PARAM:
      write_inst(self, "%%.t%lu = load i%lu, ptr %%%s", result, type_width(type), string_data(raw_name_token->string));
      break;

    case DEF_VAR:
      write_inst(self, "%%.t%lu = load i%lu, ptr @%s", result, type_width(type), string_data(raw_name_token->string));
      break;

    default:
      unreachable();
    }

    mppl_unref(name_token);
    break;
  }

  case MPPL_VAR_INDEXED: {
    const MpplIndexedVar *indexed_var_syntax = (const MpplIndexedVar *) var;
    MpplToken            *name_token         = mppl_indexed_var__name(indexed_var_syntax);
    AnyMpplExpr          *index_syntax       = mppl_indexed_var__expr(indexed_var_syntax);
    const RawSyntaxToken *raw_name_token     = (const RawSyntaxToken *) syntax_tree_raw((const SyntaxTree *) name_token);

    const Def       *def      = ctx_resolve(self->ctx, (const SyntaxTree *) name_token, NULL);
    const ArrayType *def_type = (const ArrayType *) ctx_type_of(self->ctx, def_syntax(def), NULL);
    unsigned long    length   = array_type_length(def_type);

    Temp index_temporal = self->temp++;
    Temp ptr_temporal   = self->temp++;

    write_expr(self, index_temporal, index_syntax);
    write_inst(self, "%%.t%lu = getelementptr inbounds [%lu x i16], ptr @%s, i16 %%.t%lu",
      ptr_temporal, length, string_data(raw_name_token->string), index_temporal);
    write_inst(self, "%%.t%lu = load i16, %%.t%lu", result, ptr_temporal);

    mppl_unref(name_token);
    mppl_unref(index_syntax);
    break;
  }

  default:
    unreachable();
  }
}

Label write_lit_expr(Generator *self, Temp result, const AnyMpplLit *lit)
{
  switch (mppl_lit__kind(lit)) {
  case MPPL_LIT_NUMBER: {
    const MpplNumberLit *number_lit_syntax = (const MpplNumberLit *) lit;

    long value = mppl_lit_number__to_long(number_lit_syntax);
    write_inst(self, "%%.t%lu = add i16 0, %ld", result, value);
    break;
  }

  case MPPL_LIT_STRING: {
    const MpplStringLit *string_lit_syntax = (const MpplStringLit *) lit;

    char *value = mppl_lit_string__to_string(string_lit_syntax);
    write_inst(self, "%%.t%lu = add i8 0, %d", result, (int) value[0]);
    free(value);
    break;
  }

  case MPPL_LIT_BOOLEAN: {
    const MpplBooleanLit *boolean_lit_syntax = (const MpplBooleanLit *) lit;

    int value = mppl_lit_boolean__to_int(boolean_lit_syntax);
    write_inst(self, "%%.t%lu = add i1 0, %d", result, value);
    break;
  }

  default:
    unreachable();
  }
  return LABEL_NULL;
}

void write_expr(Generator *self, Temp result, const AnyMpplExpr *expr)
{
  switch (mppl_expr__kind(expr)) {
  case MPPL_EXPR_BINARY:
    write_binary_expr(self, result, (const MpplBinaryExpr *) expr);
    break;

  case MPPL_EXPR_NOT:
    write_not_expr(self, result, (const MpplNotExpr *) expr);
    break;

  case MPPL_EXPR_PAREN:
    write_paren_expr(self, result, (const MpplParenExpr *) expr);
    break;

  case MPPL_EXPR_CAST:
    write_cast_expr(self, result, (const MpplCastExpr *) expr);
    break;

  case MPPL_EXPR_VAR:
    write_var_expr(self, result, (const AnyMpplVar *) expr);
    break;

  case MPPL_EXPR_LIT:
    write_lit_expr(self, result, (const AnyMpplLit *) expr);
    break;

  default:
    unreachable();
  }
}

Ptr write_expr_ref(Generator *self, const AnyMpplExpr *expr)
{
  Ptr ref;

  ref.type = ctx_type_of(self->ctx, (const SyntaxTree *) expr, NULL);
  if (mppl_expr__kind(expr) == MPPL_EXPR_VAR) {
    const AnyMpplVar *var_syntax = (const AnyMpplVar *) expr;
    switch (mppl_var__kind(var_syntax)) {
    case MPPL_VAR_ENTIRE: {
      const MpplEntireVar *entire_var_syntax = (const MpplEntireVar *) var_syntax;

      MpplToken *name_token = mppl_entire_var__name(entire_var_syntax);
      const Def *def        = ctx_resolve(self->ctx, (const SyntaxTree *) name_token, NULL);

      ref.is_temporal = 0;
      ref.ptr.def     = def;

      mppl_unref(name_token);
      break;
    }

    case MPPL_VAR_INDEXED: {
      const MpplIndexedVar *indexed_var_syntax = (const MpplIndexedVar *) var_syntax;
      MpplToken            *name_token         = mppl_indexed_var__name(indexed_var_syntax);
      AnyMpplExpr          *index_syntax       = mppl_indexed_var__expr(indexed_var_syntax);
      const RawSyntaxToken *raw_name_token     = (const RawSyntaxToken *) syntax_tree_raw((const SyntaxTree *) name_token);

      const Def       *def      = ctx_resolve(self->ctx, (const SyntaxTree *) name_token, NULL);
      const ArrayType *def_type = (const ArrayType *) ctx_type_of(self->ctx, def_syntax(def), NULL);
      unsigned long    length   = array_type_length(def_type);

      Temp index_temporal = self->temp++;
      Temp ptr_temporal   = self->temp++;

      write_expr(self, index_temporal, index_syntax);
      write_inst(self, "%%.t%lu = getelementptr inbounds [%lu x i%lu], ptr @%s, i16 %%.t%lu",
        ptr_temporal, length, type_width(ref.type), string_data(raw_name_token->string), index_temporal);

      ref.is_temporal  = 1;
      ref.ptr.temporal = ptr_temporal;

      mppl_unref(name_token);
      mppl_unref(index_syntax);
      break;
    }

    default:
      unreachable();
    }
  } else {
    Temp temporal    = self->temp++;
    ref.is_temporal  = 1;
    ref.ptr.temporal = self->temp++;
    write_expr(self, temporal, expr);
    write_inst(self, "%%.t%lu = alloca i%lu", ref.ptr.temporal, type_width(ref.type));
    write_inst(self, "store i%lu %%.t%lu, ptr %%.t%lu", type_width(ref.type), temporal, ref.ptr.temporal);
  }
  return ref;
}

Label write_stmt(Generator *self, const AnyMpplStmt *stmt, Label source, Label sink);

Label write_assign_stmt(Generator *self, const MpplAssignStmt *syntax)
{
  AnyMpplVar   *lhs_syntax = mppl_assign_stmt__lhs(syntax);
  AnyMpplExpr  *rhs_syntax = mppl_assign_stmt__rhs(syntax);
  Temp          value      = self->temp++;
  Label         result     = LABEL_NULL;
  const Type   *lhs_type   = ctx_type_of(self->ctx, (const SyntaxTree *) lhs_syntax, NULL);
  unsigned long width      = type_width(lhs_type);

  Ptr ref = write_expr_ref(self, (AnyMpplExpr *) lhs_syntax);
  write_expr(self, value, rhs_syntax);
  if (ref.is_temporal) {
    write_inst(self, "store i%lu %%.t%lu, ptr %%.t%lu", width, value, ref.ptr.temporal);
  } else {
    const char *prefix = def_kind(ref.ptr.def) == DEF_VAR ? "@" : "%";
    write_inst(self, "store i%lu %%.t%lu, ptr %s%s", width, value, prefix, string_data(def_name(ref.ptr.def)));
  }

  mppl_unref(lhs_syntax);
  mppl_unref(rhs_syntax);
  return result;
}

Label write_if_stmt(Generator *self, const MpplIfStmt *syntax, Label sink)
{
  AnyMpplExpr *cond_syntax = mppl_if_stmt__cond(syntax);
  AnyMpplStmt *then_syntax = mppl_if_stmt__then_stmt(syntax);
  AnyMpplStmt *else_syntax = mppl_if_stmt__else_stmt(syntax);

  Temp  cond_temporal = self->temp++;
  Label next_label    = sink ? sink : self->block++;
  Label label;
  write_expr(self, cond_temporal, cond_syntax);

  if (else_syntax) {
    Label then_label = self->block++;
    Label else_label = self->block++;

    write_inst(self, "br i1 %%.t%lu, label %%l%lu, label %%l%lu", cond_temporal, then_label, else_label);
    write(self, "\n");

    write_label(self, then_label);
    label = write_stmt(self, then_syntax, then_label, next_label);
    if (label != LABEL_RETURN && label != LABEL_BREAK) {
      write_inst(self, "br label %%l%lu", next_label);
    }
    write(self, "\n");

    write_label(self, else_label);
    label = write_stmt(self, else_syntax, else_label, next_label);
  } else {
    Label then_label = self->block++;
    write_inst(self, "br i1 %%.t%lu, label %%l%lu, label %%l%lu", cond_temporal, then_label, next_label);
    write(self, "\n");

    write_label(self, then_label);
    label = write_stmt(self, then_syntax, then_label, next_label);
  }

  if (!sink) {
    if (label != LABEL_RETURN && label != LABEL_BREAK) {
      write_inst(self, "br label %%l%lu", next_label);
    }
    write(self, "\n");

    write_label(self, next_label);
  }

  mppl_unref(cond_syntax);
  mppl_unref(then_syntax);
  mppl_unref(else_syntax);
  return next_label;
}

Label write_while_stmt(Generator *self, const MpplWhileStmt *syntax, Label source, Label sink)
{
  AnyMpplExpr *cond_syntax = mppl_while_stmt__cond(syntax);
  AnyMpplStmt *stmt_syntax = mppl_while_stmt__do_stmt(syntax);

  Label cond_label = source ? source : self->block++;
  Label body_label = self->block++;
  Label next_label = sink ? sink : self->block++;
  Label label;

  Temp cond_temporal = self->temp++;

  self->break_label = next_label;

  write_inst(self, "br label %%l%lu", cond_label);
  write(self, "\n");

  write_label(self, cond_label);
  write_expr(self, cond_temporal, cond_syntax);
  write_inst(self, "br i1 %%.t%lu, label %%l%lu, label %%l%lu", cond_temporal, body_label, next_label);
  write(self, "\n");

  write_label(self, body_label);
  label = write_stmt(self, stmt_syntax, body_label, cond_label);
  if (label != LABEL_RETURN && label != LABEL_BREAK) {
    write_inst(self, "br label %%l%lu", cond_label);
  }
  write(self, "\n");

  if (!sink) {
    write_label(self, next_label);
  }

  mppl_unref(cond_syntax);
  mppl_unref(stmt_syntax);
  return next_label;
}

Label write_comp_stmt(Generator *self, const MpplCompStmt *syntax, Label source, Label sink)
{
  unsigned long i;

  Label current = source;

  for (i = 0; i < mppl_comp_stmt__stmt_count(syntax); ++i) {
    AnyMpplStmt *stmt = mppl_comp_stmt__stmt(syntax, i);

    if (stmt) {
      Label next = i + 1 < mppl_comp_stmt__stmt_count(syntax) ? LABEL_NULL : sink;
      current    = write_stmt(self, stmt, current, next);
    }

    mppl_unref(stmt);
    if (current == LABEL_RETURN || current == LABEL_BREAK) {
      break;
    }
  }

  return current;
}

Label write_call_stmt(Generator *self, const MpplCallStmt *syntax)
{
  MpplToken            *name_token        = mppl_call_stmt__name(syntax);
  const RawSyntaxToken *raw_name_token    = (const RawSyntaxToken *) syntax_tree_raw((const SyntaxTree *) name_token);
  MpplActParamList     *param_list_syntax = mppl_call_stmt__act_param_list(syntax);

  Array *ptrs = array_new(sizeof(Ptr));

  unsigned long i;
  for (i = 0; i < mppl_act_param_list__expr_count(param_list_syntax); ++i) {
    AnyMpplExpr *expr_syntax = mppl_act_param_list__expr(param_list_syntax, i);
    Ptr          ref         = write_expr_ref(self, expr_syntax);

    array_push(ptrs, &ref);
    mppl_unref(expr_syntax);
  }

  write(self, "  call void @%s(", string_data(raw_name_token->string));
  for (i = 0; i < array_count(ptrs); ++i) {
    AnyMpplExpr *expr_syntax = mppl_act_param_list__expr(param_list_syntax, i);
    Ptr         *ref         = array_at(ptrs, i);

    if (i > 0) {
      write(self, ", ");
    }

    if (ref->is_temporal) {
      write(self, "ptr %%.t%lu", ref->ptr.temporal);
    } else {
      const char *prefix = def_kind(ref->ptr.def) == DEF_VAR ? "@" : "%";
      write(self, "ptr %s%s", prefix, string_data(def_name(ref->ptr.def)));
    }

    mppl_unref(expr_syntax);
  }
  write(self, ")\n");

  mppl_unref(name_token);
  mppl_unref(param_list_syntax);
  array_free(ptrs);
  return LABEL_NULL;
}

Label write_input_stmt(Generator *self, const MpplInputStmt *syntax)
{
  unsigned long  i;
  MpplToken     *read_token        = mppl_input_stmt__read_token(syntax);
  MpplInputList *input_list_syntax = mppl_input_stmt__input_list(syntax);

  Array *ptrs   = array_new(sizeof(Ptr));
  Str    format = str();

  if (input_list_syntax) {
    for (i = 0; i < mppl_input_list__var_count(input_list_syntax); ++i) {
      AnyMpplVar *var_syntax = mppl_input_list__var(input_list_syntax, i);

      Ptr ref = write_expr_ref(self, (AnyMpplExpr *) var_syntax);
      array_push(ptrs, &ref);

      switch (type_kind(ref.type)) {
      case TYPE_INTEGER:
        str_push(&format, "%hd");
        break;

      case TYPE_CHAR:
        str_push(&format, "%c");
        break;

      default:
        unreachable();
      }

      mppl_unref(var_syntax);
    }
  }

  if (syntax_tree_kind((SyntaxTree *) read_token) == SYNTAX_READLN_KW) {
    str_push(&format, "%*[^\\0A]");
  }

  {
    unsigned long id = array_count(self->strs);
    array_push(self->strs, &format);
    write(self, "  call i32 @scanf(ptr @.str%lu", id);
  }
  if (input_list_syntax) {
    for (i = 0; i < mppl_input_list__var_count(input_list_syntax); ++i) {
      AnyMpplVar *var_syntax = mppl_input_list__var(input_list_syntax, i);
      Ptr        *ref        = array_at(ptrs, i);

      if (ref->is_temporal) {
        write(self, ", ptr %%.t%lu", ref->ptr.temporal);
      } else {
        const char *prefix = def_kind(ref->ptr.def) == DEF_VAR ? "@" : "%";
        write(self, ", ptr %s%s", prefix, string_data(def_name(ref->ptr.def)));
      }

      mppl_unref(var_syntax);
    }
  }
  write(self, ")\n");

  if (syntax_tree_kind((SyntaxTree *) read_token) == SYNTAX_READLN_KW) {
    write_inst(self, "call i32 @getchar()");
  }

  array_free(ptrs);
  mppl_unref(read_token);
  mppl_unref(input_list_syntax);
  return LABEL_NULL;
}

Label write_output_stmt(Generator *self, const MpplOutputStmt *syntax)
{
  unsigned long i;
  MpplToken    *write_token     = mppl_output_stmt__write_token(syntax);
  MpplOutList  *out_list_syntax = mppl_output_stmt__output_list(syntax);

  Array *values = array_new(sizeof(Temp));
  Str    format = str();

  if (out_list_syntax) {
    for (i = 0; i < mppl_out_list__out_value_count(out_list_syntax); ++i) {
      MpplOutValue *out_value_syntax = mppl_out_list__out_value(out_list_syntax, i);
      AnyMpplExpr  *expr_syntax      = mppl_out_value__expr(out_value_syntax);
      const Type   *type             = ctx_type_of(self->ctx, (const SyntaxTree *) expr_syntax, NULL);

      if (type_kind(type) == TYPE_STRING) {
        const MpplStringLit *string_lit_syntax = (MpplStringLit *) expr_syntax;

        char *value  = mppl_lit_string__to_string(string_lit_syntax);
        Temp  ignore = -1ul;
        str_push(&format, value);
        array_push(values, &ignore);
        free(value);
      } else {
        MpplNumberLit *number_lit_syntax = mppl_out_value__width(out_value_syntax);

        Temp value = self->temp++;
        write_expr(self, value, expr_syntax);
        array_push(values, &value);

        str_push(&format, "%");
        if (number_lit_syntax) {
          const RawSyntaxToken *raw_token = (const RawSyntaxToken *) syntax_tree_raw((const SyntaxTree *) number_lit_syntax);
          str_push(&format, string_data(raw_token->string));
        }
        switch (type_kind(type)) {
        case TYPE_INTEGER:
          str_push(&format, "hd");
          break;

        case TYPE_CHAR:
          str_push(&format, "c");
          break;

        case TYPE_BOOLEAN:
          str_push(&format, "d");
          break;

        default:
          unreachable();
        }

        mppl_unref(number_lit_syntax);
      }

      mppl_unref(expr_syntax);
      mppl_unref(out_value_syntax);
    }
  }

  if (syntax_tree_kind((SyntaxTree *) write_token) == SYNTAX_WRITELN_KW) {
    str_push(&format, "\\0A");
  }

  {
    unsigned long id = array_count(self->strs);
    array_push(self->strs, &format);
    write(self, "  call i32 @printf(ptr @.str%lu", id);
  }
  if (out_list_syntax) {
    for (i = 0; i < mppl_out_list__out_value_count(out_list_syntax); ++i) {
      MpplOutValue *out_value_syntax = mppl_out_list__out_value(out_list_syntax, i);
      AnyMpplExpr  *expr_syntax      = mppl_out_value__expr(out_value_syntax);
      const Type   *type             = ctx_type_of(self->ctx, (const SyntaxTree *) expr_syntax, NULL);

      if (type_kind(type) != TYPE_STRING) {
        Temp value = *(Temp *) array_at(values, i);
        write(self, ", i%lu %%.t%lu", type_width(type), value);
      }

      mppl_unref(expr_syntax);
      mppl_unref(out_value_syntax);
    }
  }
  write(self, ")\n");

  array_free(values);
  mppl_unref(write_token);
  mppl_unref(out_list_syntax);
  return LABEL_NULL;
}

Label write_stmt(Generator *self, const AnyMpplStmt *stmt, Label source, Label sink)
{
  switch (mppl_stmt__kind(stmt)) {
  case MPPL_STMT_ASSIGN:
    return write_assign_stmt(self, (const MpplAssignStmt *) stmt);

  case MPPL_STMT_IF:
    return write_if_stmt(self, (const MpplIfStmt *) stmt, sink);

  case MPPL_STMT_WHILE:
    return write_while_stmt(self, (const MpplWhileStmt *) stmt, source, sink);

  case MPPL_STMT_BREAK:
    write_inst(self, "br label %%l%lu", self->break_label);
    return LABEL_BREAK;

  case MPPL_STMT_CALL:
    return write_call_stmt(self, (const MpplCallStmt *) stmt);

  case MPPL_STMT_RETURN:
    write_inst(self, "ret void");
    return LABEL_RETURN;

  case MPPL_STMT_INPUT:
    return write_input_stmt(self, (const MpplInputStmt *) stmt);

  case MPPL_STMT_OUTPUT:
    return write_output_stmt(self, (const MpplOutputStmt *) stmt);

  case MPPL_STMT_COMP:
    return write_comp_stmt(self, (const MpplCompStmt *) stmt, source, sink);

  default:
    return -1;
  }
}

void visit_var_decl(const MpplAstWalker *self, const MpplVarDecl *syntax, void *generator)
{
  unsigned long i;
  Generator    *gen  = generator;
  const Type   *type = ctx_type_of(gen->ctx, (const SyntaxTree *) syntax, NULL);

  for (i = 0; i < mppl_var_decl__name_count(syntax); ++i) {
    MpplToken            *name_token     = mppl_var_decl__name(syntax, i);
    const RawSyntaxToken *raw_name_token = (const RawSyntaxToken *) syntax_tree_raw((const SyntaxTree *) name_token);

    if (type_kind(type) == TYPE_ARRAY) {
      const ArrayType *array_type = (const ArrayType *) type;
      write(gen, "@%s = common global [%lu x i%lu] zeroinitializer\n",
        string_data(raw_name_token->string), array_type_length(array_type), type_width(array_type_base(array_type)));
    } else {
      write(gen, "@%s = common global i%lu 0\n", string_data(raw_name_token->string), type_width(type));
    }

    mppl_unref(name_token);
  }

  (void) self;
}

void visit_proc_decl(const MpplAstWalker *self, const MpplProcDecl *syntax, void *generator)
{
  unsigned long i, j;

  Generator            *gen                  = generator;
  MpplToken            *name_token           = mppl_proc_decl__name(syntax);
  MpplFmlParamList     *param_list_syntax    = mppl_proc_decl__fml_param_list(syntax);
  MpplVarDeclPart      *var_decl_part_syntax = mppl_proc_decl__var_decl_part(syntax);
  MpplCompStmt         *stmt_syntax          = mppl_proc_decl__comp_stmt(syntax);
  const RawSyntaxToken *raw_name_token       = (const RawSyntaxToken *) syntax_tree_raw((const SyntaxTree *) name_token);
  Label                 label;

  write(gen, "define void @%s", string_data(raw_name_token->string));
  if (param_list_syntax) {
    write(gen, "(");
    for (i = 0; i < mppl_fml_param_list__sec_count(param_list_syntax); ++i) {
      MpplFmlParamSec *sec_syntax = mppl_fml_param_list__sec(param_list_syntax, i);

      if (i > 0) {
        write(gen, ", ");
      }

      for (j = 0; j < mppl_fml_param_sec__name_count(sec_syntax); ++j) {
        MpplToken            *name_token     = mppl_fml_param_sec__name(sec_syntax, j);
        const RawSyntaxToken *raw_name_token = (const RawSyntaxToken *) syntax_tree_raw((const SyntaxTree *) name_token);

        if (j > 0) {
          write(gen, ", ");
        }

        write(gen, "ptr %%%s", string_data(raw_name_token->string));
        mppl_unref(name_token);
      }

      mppl_unref(sec_syntax);
    }
    write(gen, ")");
  }
  write(gen, " {\n");

  if (var_decl_part_syntax) {
    for (i = 0; i < mppl_var_decl_part__var_decl_count(var_decl_part_syntax); ++i) {
      MpplVarDecl *var_decl = mppl_var_decl_part__var_decl(var_decl_part_syntax, i);
      const Type  *type     = ctx_type_of(gen->ctx, (const SyntaxTree *) var_decl, NULL);
      for (j = 0; j < mppl_var_decl__name_count(var_decl); ++j) {
        MpplToken            *name_token     = mppl_var_decl__name(var_decl, j);
        const RawSyntaxToken *raw_name_token = (const RawSyntaxToken *) syntax_tree_raw((const SyntaxTree *) name_token);

        if (type_kind(type) == TYPE_ARRAY) {
          const ArrayType *array_type = (const ArrayType *) type;
          write_inst(gen, "%%%s = alloca [%lu x i%lu]",
            string_data(raw_name_token->string), array_type_length(array_type), type_width(array_type_base(array_type)));
        } else {
          write_inst(gen, "%%%s = alloca i%lu",
            string_data(raw_name_token->string), type_width(type));
        }

        mppl_unref(name_token);
      }
      mppl_unref(var_decl);
    }
  }
  label = write_stmt(gen, (AnyMpplStmt *) stmt_syntax, LABEL_NULL, LABEL_NULL);
  if (label != LABEL_RETURN) {
    write_inst(gen, "ret void");
  }
  write(gen, "}\n");

  mppl_unref(name_token);
  mppl_unref(param_list_syntax);
  mppl_unref(var_decl_part_syntax);
  mppl_unref(stmt_syntax);

  (void) self;
}

void visit_program(const MpplAstWalker *self, const MpplProgram *syntax, void *generator)
{
  unsigned long i;
  Generator    *gen         = generator;
  MpplCompStmt *stmt_syntax = mppl_program__stmt(syntax);
  Label         label;

  write(gen, "declare i32 @getchar()\n");
  write(gen, "declare i32 @printf(ptr, ...)\n");
  write(gen, "declare i32 @scanf(ptr, ...)\n\n");

  for (i = 0; i < mppl_program__decl_part_count(syntax); ++i) {
    AnyMpplDeclPart *decl_part = mppl_program__decl_part(syntax, i);

    mppl_ast__walk_decl_part(self, decl_part, generator);
    write(gen, "\n");

    mppl_unref(decl_part);
  }

  write(gen, "define i32 @main() {\n");
  label = write_stmt(gen, (const AnyMpplStmt *) stmt_syntax, LABEL_NULL, LABEL_NULL);
  if (label != LABEL_RETURN) {
    write_inst(gen, "ret i32 0");
  }
  write(gen, "}\n");

  mppl_unref(stmt_syntax);
}

int mpplc_codegen_llvm_ir(const Source *source, const MpplProgram *syntax, Ctx *ctx)
{
  unsigned long i;
  MpplAstWalker walker;

  Generator self;
  self.ctx   = ctx;
  self.temp  = 1;
  self.block = 1;
  self.strs  = array_new(sizeof(Str));
  {
    char *output_filename = xmalloc(sizeof(char) * (source->file_name_length + 1));
    sprintf(output_filename, "%.*s.ll", (int) source->file_name_length - 4, source->file_name);
    self.file = fopen(output_filename, "w");
    free(output_filename);
  }

  if (!self.file) {
    fprintf(stderr, "error: failed to open output file\n");
    return 0;
  }

  mppl_ast_walker__setup(&walker);
  walker.visit_program   = &visit_program;
  walker.visit_var_decl  = &visit_var_decl;
  walker.visit_proc_decl = &visit_proc_decl;
  mppl_ast_walker__travel(&walker, syntax, &self);

  write(&self, "\n");
  for (i = 0; i < array_count(self.strs); ++i) {
    Str *str = array_at(self.strs, i);
    array_push_count(str->chars, "\0", 1);
    write(&self, "@.str%lu = private unnamed_addr constant [%lu x i8] c\"%s\\00\"\n", i, str->length + 1, array_data(str->chars));
    str_free(str);
  }

  array_free(self.strs);
  fclose(self.file);
  return 1;
}
