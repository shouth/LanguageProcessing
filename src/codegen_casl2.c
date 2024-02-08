#include <limits.h>
#include <stdio.h>
#include <string.h>

#include "compiler.h"
#include "context.h"
#include "context_fwd.h"
#include "map.h"
#include "mppl_syntax.h"
#include "mppl_syntax_ext.h"
#include "source.h"
#include "syntax_kind.h"
#include "syntax_tree.h"
#include "utility.h"

typedef enum {
  GR0,
  GR1,
  GR2,
  GR3,
  GR4,
  GR5,
  GR6,
  GR7
} Reg;

#define ADR_KIND_OFFSET (sizeof(Adr) * CHAR_BIT - 2)
#define ADR_NULL        ((Adr) 0)
#define ADR_CALL        ((Adr) 0 - 1)

typedef enum {
  ADR_NORMAL = 1,
  ADR_VAR,
  ADR_PROC
} AdrKind;

typedef unsigned long     Adr;
typedef struct Expr       Expr;
typedef struct ExprWriter ExprWriter;
typedef struct Generator  Generator;

struct Expr {
  const AnyMpplExpr *syntax;
  Expr              *child[2];
  unsigned long      id;
  unsigned long      priority;
  Reg                reg;
  int                spill;
};

struct Generator {
  FILE *file;
  Ctx  *ctx;
  Map  *symbols;
  Adr   current_label;
  Adr   label_count;
  Adr   var_label_count;
  Adr   proc_label_count;
  Adr   break_label;
};

static void fmt_reg(char *buf, Reg reg)
{
  sprintf(buf, "GR%d", reg);
}

static void fmt_adr(char *buf, Adr a)
{
  AdrKind kind = a >> ADR_KIND_OFFSET;
  a &= (1ul << ADR_KIND_OFFSET) - 1ul;
  switch (kind) {
  case ADR_NORMAL:
    sprintf(buf, "L%lu", a);
    break;

  case ADR_VAR:
    sprintf(buf, "V%lu", a);
    break;

  case ADR_PROC:
    sprintf(buf, "P%lu", a);
    break;

  default:
    unreachable();
  }
}

static const char *label(Adr label)
{
  static char buf[16];
  fmt_adr(buf, label);
  return buf;
}

static const char *r1(Reg reg)
{
  static char buf[16];
  fmt_reg(buf, reg);
  return buf;
}

static const char *r2(Reg reg)
{
  static char buf[16];
  fmt_reg(buf, reg);
  return buf;
}

static const char *r(Reg reg)
{
  static char buf[16];
  fmt_reg(buf, reg);
  return buf;
}

static const char *x(Reg reg)
{
  static char buf[16];
  fmt_reg(buf, reg);
  return buf;
}

static const char *adr(Adr a)
{
  static char buf[16];
  fmt_adr(buf, a);
  return buf;
}

static Expr *expr_create_tree(const AnyMpplExpr *syntax)
{
  Expr *self     = xmalloc(sizeof(Expr));
  self->syntax   = syntax;
  self->child[0] = NULL;
  self->child[1] = NULL;

  switch (mppl_expr__kind(syntax)) {
  case MPPL_EXPR_BINARY: {
    const MpplBinaryExpr *binary_syntax = (const MpplBinaryExpr *) syntax;
    self->child[0]                      = expr_create_tree(mppl_binary_expr__lhs(binary_syntax));
    self->child[1]                      = expr_create_tree(mppl_binary_expr__rhs(binary_syntax));
    break;
  }

  case MPPL_EXPR_PAREN: {
    const MpplParenExpr *paren_syntax = (const MpplParenExpr *) syntax;
    self->child[0]                    = expr_create_tree(mppl_paren_expr__expr(paren_syntax));
    break;
  }

  case MPPL_EXPR_NOT: {
    const MpplNotExpr *not_syntax = (const MpplNotExpr *) syntax;
    self->child[0]                = expr_create_tree(mppl_not_expr__expr(not_syntax));
    break;
  }

  case MPPL_EXPR_CAST: {
    const MpplCastExpr *cast_syntax = (const MpplCastExpr *) syntax;
    self->child[0]                  = expr_create_tree(mppl_cast_expr__expr(cast_syntax));
    break;
  }

  case MPPL_EXPR_VAR: {
    const AnyMpplVar *var_syntax = (const AnyMpplVar *) syntax;
    switch (mppl_var__kind(var_syntax)) {
    case MPPL_VAR_ENTIRE:
      /* do nothing */
      break;

    case MPPL_VAR_INDEXED: {
      const MpplIndexedVar *indexed_syntax = (const MpplIndexedVar *) var_syntax;
      self->child[0]                       = expr_create_tree(mppl_indexed_var__expr(indexed_syntax));
      break;
    }
    }
    break;
  }

  case MPPL_EXPR_LIT:
    /* do nothing */
    break;
  }

  return self;
}

static unsigned long expr_assign_id(Expr *self, unsigned long id)
{
  if (self) {
    id       = expr_assign_id(self->child[0], id);
    id       = expr_assign_id(self->child[1], id);
    self->id = id++;
  }
  return id;
}

static unsigned long expr_assign_priority(Expr *self)
{
  switch (mppl_expr__kind(self->syntax)) {
  case MPPL_EXPR_BINARY: {
    unsigned long         lhs_priority  = expr_assign_priority(self->child[0]);
    unsigned long         rhs_priority  = expr_assign_priority(self->child[1]);
    const MpplBinaryExpr *binary_syntax = (const MpplBinaryExpr *) self->syntax;
    MpplToken            *op_syntax     = mppl_binary_expr__op_token(binary_syntax);
    SyntaxKind            op_kind       = syntax_tree_kind((const SyntaxTree *) op_syntax);

    if (lhs_priority != rhs_priority) {
      self->priority = lhs_priority > rhs_priority ? lhs_priority : rhs_priority;
    } else if (op_kind == SYNTAX_AND_KW || op_kind == SYNTAX_OR_KW) {
      self->priority = lhs_priority;
    } else {
      self->priority = lhs_priority + 1;
    }

    mppl_unref(op_syntax);
    return self->priority;
  }

  case MPPL_EXPR_PAREN:
    return self->priority = expr_assign_priority(self->child[0]);

  case MPPL_EXPR_NOT:
    return self->priority = expr_assign_priority(self->child[0]);

  case MPPL_EXPR_CAST:
    return self->priority = expr_assign_priority(self->child[0]);

  case MPPL_EXPR_VAR: {
    const AnyMpplVar *var_syntax = (const AnyMpplVar *) self->syntax;
    switch (mppl_var__kind(var_syntax)) {
    case MPPL_VAR_ENTIRE:
      return self->priority = 1;

    case MPPL_VAR_INDEXED:
      return self->priority = expr_assign_priority(self->child[0]);

    default:
      unreachable();
    }
  }

  case MPPL_EXPR_LIT:
    return self->priority = 1;

  default:
    unreachable();
  }
}

static Reg reserve_reg(Expr *usage[8], Expr *expr, int hint)
{
  if (hint >= 0) {
    if (!usage[hint]) {
      usage[hint] = expr;
      expr->reg   = (Reg) hint;
      return (Reg) hint;
    } else {
      unreachable();
    }
  } else {
    unsigned long i;
    unsigned long target = -1ul;

    for (i = 1; i < 8; ++i) {
      if (!usage[i]) {
        target = i;
        break;
      }
    }

    if (target == -1ul) {
      for (i = 1; i < 8; ++i) {
        if (target == -1ul || usage[i]->id < usage[target]->id) {
          target = i;
        }
      }
      usage[target]->spill = 1;
    }

    usage[target] = expr;
    expr->reg     = (Reg) target;
    return expr->reg;
  }
}

static void release_reg(Expr *usage[8], Expr *expr)
{
  usage[expr->reg] = NULL;
}

static Reg expr_assign_reg(Expr *self, int hint, Expr *usage[8])
{
  self->spill = 0;
  switch (mppl_expr__kind(self->syntax)) {
  case MPPL_EXPR_BINARY: {
    const MpplBinaryExpr *binary_syntax = (const MpplBinaryExpr *) self->syntax;
    MpplToken            *op_syntax     = mppl_binary_expr__op_token(binary_syntax);
    SyntaxKind            op_kind       = syntax_tree_kind((const SyntaxTree *) op_syntax);

    if (op_kind == SYNTAX_AND_KW || op_kind == SYNTAX_OR_KW) {
      hint = expr_assign_reg(self->child[0], hint, usage);
      release_reg(usage, self->child[0]);
      expr_assign_reg(self->child[1], hint, usage);
      release_reg(usage, self->child[1]);
    } else if (self->child[0]->priority > self->child[1]->priority) {
      hint = expr_assign_reg(self->child[0], hint, usage);
      expr_assign_reg(self->child[1], -1, usage);
      release_reg(usage, self->child[1]);
      release_reg(usage, self->child[0]);
    } else {
      hint = expr_assign_reg(self->child[1], hint, usage);
      expr_assign_reg(self->child[0], -1, usage);
      release_reg(usage, self->child[0]);
      release_reg(usage, self->child[1]);
    }

    mppl_unref(op_syntax);
    return reserve_reg(usage, self, hint);
  }

  case MPPL_EXPR_PAREN:
    hint = expr_assign_reg(self->child[0], hint, usage);
    release_reg(usage, self->child[0]);
    return reserve_reg(usage, self, hint);

  case MPPL_EXPR_NOT:
    hint = expr_assign_reg(self->child[0], hint, usage);
    release_reg(usage, self->child[0]);
    return reserve_reg(usage, self, hint);

  case MPPL_EXPR_CAST:
    hint = expr_assign_reg(self->child[0], hint, usage);
    release_reg(usage, self->child[0]);
    return reserve_reg(usage, self, hint);

  case MPPL_EXPR_VAR: {
    const AnyMpplVar *var_syntax = (const AnyMpplVar *) self->syntax;
    switch (mppl_var__kind(var_syntax)) {
    case MPPL_VAR_ENTIRE:
      return reserve_reg(usage, self, hint);

    case MPPL_VAR_INDEXED:
      hint = expr_assign_reg(self->child[0], hint, usage);
      release_reg(usage, self->child[0]);
      return reserve_reg(usage, self, hint);

    default:
      unreachable();
    }
  }

  case MPPL_EXPR_LIT:
    return reserve_reg(usage, self, hint);

  default:
    unreachable();
  }
}

static void expr_new(const AnyMpplExpr *syntax)
{
  Expr         *self = expr_create_tree(mppl_ref(syntax));
  Expr         *usage[8];
  unsigned long i;
  expr_assign_id(self, 0);
  expr_assign_priority(self);
  for (i = 0; i < 8; ++i) {
    usage[i] = NULL;
  }
  expr_assign_reg(self, -1, usage);
}

static void expr_free(Expr *self)
{
  if (self) {
    mppl_unref(self->syntax);
    expr_free(self->child[0]);
    expr_free(self->child[1]);
    free(self);
  }
}

static Adr locate(Generator *self, const Def *def, Adr label)
{
  MapIndex index;
  if (label) {
    if (map_entry(self->symbols, (void *) def, &index)) {
      return *(Adr *) map_value(self->symbols, &index) = label;
    } else {
      Adr *p = dup(&label, sizeof(Adr), 1);
      map_update(self->symbols, &index, (void *) def, p);
      return label;
    }
  } else {
    if (map_entry(self->symbols, (void *) def, &index)) {
      return *(Adr *) map_value(self->symbols, &index);
    } else {
      unreachable();
    }
  }
}

static Reg reg_reserve(Generator *self)
{
  return GR0;
}

static void reg_release(Generator *self, Reg reg)
{
  (void) self;
  (void) reg;
}

static void write_inst(Generator *self, const char *inst, const char *args[], int count)
{
  int i;

  fprintf(self->file, "%-10s", self->current_label ? label(self->current_label) : "");
  if (count) {
    fprintf(self->file, "%-6s", inst);
    for (i = 0; i < count; ++i) {
      if (i) {
        fprintf(self->file, ", ");
      }
      fprintf(self->file, "%s", args[i]);
    }
  } else {
    fprintf(self->file, "%s", inst);
  }
  fprintf(self->file, "\n");

  self->current_label = ADR_NULL;
}

static void write_inst0(Generator *self, const char *inst)
{
  write_inst(self, inst, NULL, 0);
}

static void write_inst1(Generator *self, const char *inst, const char *arg1)
{
  const char *args[1];
  args[0] = arg1;
  write_inst(self, inst, args, 1);
}

static void write_inst2(Generator *self, const char *inst, const char *arg1, const char *arg2)
{
  const char *args[2];
  args[0] = arg1;
  args[1] = arg2;
  write_inst(self, inst, args, 2);
}

static void write_inst3(Generator *self, const char *inst, const char *arg1, const char *arg2, const char *arg3)
{
  const char *args[3];
  args[0] = arg1;
  args[1] = arg2;
  args[2] = arg3;
  write_inst(self, inst, args, 3);
}

static void write_label(Generator *self, Adr a)
{
  if (self->current_label >> ADR_KIND_OFFSET != ADR_NULL) {
    write_inst0(self, "NOP");
  }
  self->current_label = a;
}

static void write_expr(Generator *self, Reg reg, const AnyMpplExpr *syntax);

static void write_relational_expr(Generator *self, Reg reg, const char *inst, const AnyMpplExpr *lhs_syntax, const AnyMpplExpr *rhs_syntax)
{
  Reg rhs_reg     = reg_reserve(self);
  Adr false_block = self->label_count++;
  Adr next_block  = self->label_count++;

  write_expr(self, reg, lhs_syntax);
  write_expr(self, rhs_reg, rhs_syntax);
  write_inst2(self, "CPA", r1(reg), r2(rhs_reg));
  reg_release(self, rhs_reg);
  write_inst1(self, inst, adr(false_block));
  write_inst2(self, "LAD", r(reg), "1");
  write_inst1(self, "JUMP", adr(next_block));
  write_label(self, false_block);
  write_inst2(self, "LAD", r(reg), "0");
  write_label(self, next_block);
}

static void write_arithmetic_expr(Generator *self, Reg reg, const char *inst, const AnyMpplExpr *lhs_syntax, const AnyMpplExpr *rhs_syntax)
{
  Reg rhs_reg = reg_reserve(self);

  write_expr(self, reg, lhs_syntax);
  write_expr(self, rhs_reg, rhs_syntax);
  write_inst2(self, inst, r1(reg), r2(rhs_reg));
  reg_release(self, rhs_reg);
}

static void write_logical_expr(Generator *self, Reg reg, const char *jump, const char *early, const AnyMpplExpr *lhs_syntax, const AnyMpplExpr *rhs_syntax)
{
  Adr false_block = self->label_count++;
  Adr next_block  = self->label_count++;

  write_expr(self, reg, lhs_syntax);
  write_inst2(self, "CPA", r(reg), "1");
  write_inst1(self, jump, adr(false_block));
  write_inst1(self, "LAD", early);
  write_inst1(self, "JUMP", adr(next_block));
  write_label(self, false_block);
  write_expr(self, reg, rhs_syntax);
  write_label(self, next_block);
}

static void write_binary_expr(Generator *self, Reg reg, const MpplBinaryExpr *syntax)
{
  AnyMpplExpr *lhs_syntax = mppl_binary_expr__lhs(syntax);
  MpplToken   *op_syntax  = mppl_binary_expr__op_token(syntax);
  AnyMpplExpr *rhs_syntax = mppl_binary_expr__rhs(syntax);
  SyntaxKind   op_kind    = syntax_tree_kind((const SyntaxTree *) op_syntax);

  if (lhs_syntax) {
    switch (op_kind) {
    case SYNTAX_EQUAL_TOKEN:
      write_relational_expr(self, reg, "JNZ", lhs_syntax, rhs_syntax);
      break;

    case SYNTAX_NOTEQ_TOKEN:
      write_relational_expr(self, reg, "JZE", lhs_syntax, rhs_syntax);
      break;

    case SYNTAX_LESS_TOKEN:
      write_relational_expr(self, reg, "JMI", lhs_syntax, rhs_syntax);
      break;

    case SYNTAX_LESSEQ_TOKEN:
      write_relational_expr(self, reg, "JPL", rhs_syntax, lhs_syntax);
      break;

    case SYNTAX_GREATER_TOKEN:
      write_relational_expr(self, reg, "JPL", lhs_syntax, rhs_syntax);
      break;

    case SYNTAX_GREATEREQ_TOKEN:
      write_relational_expr(self, reg, "JMI", rhs_syntax, lhs_syntax);
      break;

    case SYNTAX_PLUS_TOKEN:
      write_arithmetic_expr(self, reg, "ADDA", lhs_syntax, rhs_syntax);
      break;

    case SYNTAX_MINUS_TOKEN:
      write_arithmetic_expr(self, reg, "SUBA", lhs_syntax, rhs_syntax);
      break;

    case SYNTAX_STAR_TOKEN:
      write_arithmetic_expr(self, reg, "MULA", lhs_syntax, rhs_syntax);
      break;

    case SYNTAX_DIV_KW:
      write_arithmetic_expr(self, reg, "DIVA", lhs_syntax, rhs_syntax);
      break;

    case SYNTAX_AND_KW:
      write_logical_expr(self, reg, "JZE", "0", lhs_syntax, rhs_syntax);
      break;

    case SYNTAX_OR_KW:
      write_logical_expr(self, reg, "JNZ", "1", lhs_syntax, rhs_syntax);
      break;

    default:
      unreachable();
    }
  } else {
    switch (op_kind) {
    case SYNTAX_PLUS_TOKEN:
      write_expr(self, reg, rhs_syntax);
      break;

    case SYNTAX_MINUS_TOKEN: {
      Reg rhs_reg = reg_reserve(self);

      write_expr(self, rhs_reg, rhs_syntax);
      write_inst2(self, "LAD", r(reg), "0");
      write_inst2(self, "SUBA", r1(reg), r2(rhs_reg));

      reg_release(self, rhs_reg);
      break;
    }

    default:
      unreachable();
    }
  }

  mppl_unref(lhs_syntax);
  mppl_unref(op_syntax);
  mppl_unref(rhs_syntax);
}

static void write_paren_expr(Generator *self, Reg reg, const MpplParenExpr *syntax)
{
  AnyMpplExpr *expr_syntax = mppl_paren_expr__expr(syntax);
  write_expr(self, reg, expr_syntax);
  mppl_unref(expr_syntax);
}

static void write_not_expr(Generator *self, Reg reg, const MpplNotExpr *syntax)
{
  AnyMpplExpr *expr_syntax = mppl_not_expr__expr(syntax);
  write_expr(self, reg, expr_syntax);
  write_inst2(self, "XOR", r(reg), "1");
  mppl_unref(expr_syntax);
}

static void write_cast_expr(Generator *self, Reg reg, const MpplCastExpr *syntax)
{
  AnyMpplExpr *expr_syntax = mppl_cast_expr__expr(syntax);
  write_expr(self, reg, expr_syntax);
  mppl_unref(expr_syntax);
}

static void write_var(Generator *self, Reg reg, const AnyMpplVar *syntax)
{
  switch (mppl_var__kind(syntax)) {
  case MPPL_VAR_ENTIRE: {
    MpplEntireVar *entire_syntax = (MpplEntireVar *) syntax;
    MpplToken     *name_syntax   = mppl_entire_var__name(entire_syntax);
    const Def     *def           = ctx_resolve(self->ctx, (const SyntaxTree *) name_syntax, NULL);
    Adr            label         = locate(self, def, ADR_NULL);

    write_inst2(self, "LD", r(reg), adr(label));
    mppl_unref(name_syntax);
    break;
  }

  case MPPL_VAR_INDEXED: {
    MpplIndexedVar *indexed_syntax = (MpplIndexedVar *) syntax;
    MpplToken      *name_syntax    = mppl_indexed_var__name(indexed_syntax);
    AnyMpplExpr    *index_syntax   = mppl_indexed_var__expr(indexed_syntax);
    const Def      *def            = ctx_resolve(self->ctx, (const SyntaxTree *) name_syntax, NULL);
    Adr             label          = locate(self, def, ADR_NULL);

    write_expr(self, reg, index_syntax);
    write_inst3(self, "LD", r(reg), adr(label), x(reg));
    mppl_unref(name_syntax);
    mppl_unref(index_syntax);
    break;
  }

  default:
    unreachable();
  }
}

static void write_lit(Generator *self, Reg reg, const AnyMpplLit *syntax)
{
  char buf[16];
  switch (mppl_lit__kind(syntax)) {
  case MPPL_LIT_BOOLEAN: {
    int raw_value = mppl_lit_boolean__to_int((const MpplBooleanLit *) syntax);
    sprintf(buf, "%d", raw_value);
    break;
  }

  case MPPL_LIT_NUMBER: {
    long raw_value = mppl_lit_number__to_long((const MpplNumberLit *) syntax);
    sprintf(buf, "%ld", raw_value);
    break;
  }

  case MPPL_LIT_STRING: {
    char *raw_value = mppl_lit_string__to_string((const MpplStringLit *) syntax);
    sprintf(buf, "#%04x", (int) raw_value[0]);
    free(raw_value);
    break;
  }

  default:
    unreachable();
  }
  write_inst2(self, "LAD", r(reg), buf);
}

static void write_expr(Generator *self, Reg reg, const AnyMpplExpr *syntax)
{
  switch (mppl_expr__kind(syntax)) {
  case MPPL_EXPR_BINARY:
    write_binary_expr(self, reg, (const MpplBinaryExpr *) syntax);
    break;

  case MPPL_EXPR_PAREN:
    write_paren_expr(self, reg, (const MpplParenExpr *) syntax);
    break;

  case MPPL_EXPR_NOT:
    write_not_expr(self, reg, (const MpplNotExpr *) syntax);
    break;

  case MPPL_EXPR_CAST:
    write_cast_expr(self, reg, (const MpplCastExpr *) syntax);
    break;

  case MPPL_EXPR_VAR:
    write_var(self, reg, (const AnyMpplVar *) syntax);
    break;

  case MPPL_EXPR_LIT:
    write_lit(self, reg, (const AnyMpplLit *) syntax);
    break;

  default:
    unreachable();
  }
}

static Adr write_stmt(Generator *self, const AnyMpplStmt *syntax, Adr source, Adr sink);

static Adr write_if_stmt(Generator *self, const MpplIfStmt *syntax, Adr sink)
{
  AnyMpplExpr *cond_syntax = mppl_if_stmt__cond(syntax);
  AnyMpplStmt *then_syntax = mppl_if_stmt__then_stmt(syntax);
  AnyMpplStmt *else_syntax = mppl_if_stmt__else_stmt(syntax);
  Adr          next_block  = sink ? sink : self->label_count++;
  Adr          false_block = else_syntax ? self->label_count++ : next_block;
  Reg          reg         = reg_reserve(self);

  write_expr(self, reg, cond_syntax);
  write_inst2(self, "CPA", r(reg), "0");
  reg_release(self, reg);
  write_inst1(self, "JNZ", adr(false_block));
  write_stmt(self, then_syntax, ADR_NULL, ADR_NULL);

  if (else_syntax) {
    write_inst1(self, "JUMP", adr(next_block));
    write_label(self, false_block);
    write_stmt(self, else_syntax, false_block, next_block);
  }

  if (!sink) {
    write_label(self, next_block);
  }

  mppl_unref(cond_syntax);
  mppl_unref(then_syntax);
  mppl_unref(else_syntax);

  return next_block;
}

static Adr write_while_stmt(Generator *self, const MpplWhileStmt *syntax, Adr source, Adr sink)
{
  AnyMpplExpr *cond_syntax          = mppl_while_stmt__cond(syntax);
  AnyMpplStmt *do_syntax            = mppl_while_stmt__do_stmt(syntax);
  Adr          cond_block           = source ? source : self->label_count++;
  Adr          next_block           = sink ? sink : self->label_count++;
  Adr          previous_break_label = self->break_label;
  Reg          reg                  = reg_reserve(self);

  self->break_label = next_block;

  write_label(self, cond_block);
  write_expr(self, reg, cond_syntax);
  write_inst2(self, "CPA", r(reg), "0");
  reg_release(self, reg);
  write_inst1(self, "JNZ", adr(next_block));
  write_stmt(self, do_syntax, ADR_NULL, ADR_NULL);
  write_inst1(self, "JUMP", adr(cond_block));

  if (!sink) {
    write_label(self, next_block);
  }

  self->break_label = previous_break_label;

  mppl_unref(cond_syntax);
  mppl_unref(do_syntax);

  return next_block;
}

static Adr write_comp_stmt(Generator *self, const MpplCompStmt *syntax, Adr source, Adr sink)
{
  unsigned long i;

  Adr current = source;

  for (i = 0; i < mppl_comp_stmt__stmt_count(syntax); ++i) {
    AnyMpplStmt *stmt = mppl_comp_stmt__stmt(syntax, i);

    Adr next = i + 1 < mppl_comp_stmt__stmt_count(syntax) ? ADR_NULL : sink;
    current  = write_stmt(self, stmt, current, next);
    mppl_unref(stmt);
  }

  return current;
}

static Adr write_call_stmt(Generator *self, const MpplCallStmt *syntax)
{
  MpplToken *name  = mppl_call_stmt__name(syntax);
  const Def *def   = ctx_resolve(self->ctx, (const SyntaxTree *) name, NULL);
  Adr        label = locate(self, def, ADR_NULL);

  write_inst1(self, "CALL", adr(label));

  mppl_unref(name);
  return ADR_CALL;
}

static Adr write_input_stmt(Generator *self, const MpplInputStmt *syntax)
{
  write_inst0(self, "NOP");
  return ADR_NULL;
}

static Adr write_output_stmt(Generator *self, const MpplOutputStmt *syntax)
{
  write_inst0(self, "NOP");
  return ADR_NULL;
}

static Adr write_stmt(Generator *self, const AnyMpplStmt *syntax, Adr source, Adr sink)
{
  switch (mppl_stmt__kind(syntax)) {
  case MPPL_STMT_ASSIGN:
    write_inst0(self, "NOP");
    return ADR_NULL;

  case MPPL_STMT_IF:
    return write_if_stmt(self, (const MpplIfStmt *) syntax, sink);

  case MPPL_STMT_WHILE:
    return write_while_stmt(self, (const MpplWhileStmt *) syntax, source, sink);

  case MPPL_STMT_BREAK:
    write_inst1(self, "JUMP", adr(self->break_label));
    return ADR_NULL;

  case MPPL_STMT_CALL:
    return write_call_stmt(self, (const MpplCallStmt *) syntax);

  case MPPL_STMT_RETURN:
    write_inst0(self, "RET");
    return ADR_NULL;

  case MPPL_STMT_INPUT:
    return write_input_stmt(self, (const MpplInputStmt *) syntax);

  case MPPL_STMT_OUTPUT:
    return write_output_stmt(self, (const MpplOutputStmt *) syntax);

  case MPPL_STMT_COMP:
    return write_comp_stmt(self, (const MpplCompStmt *) syntax, source, sink);

  default:
    unreachable();
  }
}

static void visit_var_decl(const MpplAstWalker *walker, const MpplVarDecl *syntax, void *generator)
{
  Generator    *self = generator;
  unsigned long i;

  for (i = 0; i < mppl_var_decl__name_count(syntax); ++i) {
    MpplToken *name  = mppl_var_decl__name(syntax, i);
    const Def *def   = ctx_resolve(self->ctx, (const SyntaxTree *) name, NULL);
    Adr        label = locate(self, def, self->var_label_count++);

    write_label(self, label);
    write_inst1(self, "DC", "1");
    mppl_unref(name);
  }

  (void) walker;
}

static void visit_fml_param_sec(const MpplAstWalker *walker, const MpplFmlParamSec *syntax, void *generator)
{
  Generator    *self = generator;
  unsigned long i;

  for (i = 0; i < mppl_fml_param_sec__name_count(syntax); ++i) {
    MpplToken *name  = mppl_fml_param_sec__name(syntax, i);
    const Def *def   = ctx_resolve(self->ctx, (const SyntaxTree *) name, NULL);
    Adr        label = locate(self, def, self->var_label_count++);

    write_label(self, label);
    write_inst1(self, "DC", "1");
    mppl_unref(name);
  }

  (void) walker;
}

static void visit_proc_decl(const MpplAstWalker *walker, const MpplProcDecl *syntax, void *generator)
{
  unsigned long     i, j;
  Generator        *self   = generator;
  MpplFmlParamList *params = mppl_proc_decl__fml_param_list(syntax);
  MpplVarDeclPart  *vars   = mppl_proc_decl__var_decl_part(syntax);
  MpplCompStmt     *body   = mppl_proc_decl__comp_stmt(syntax);
  MpplToken        *name   = mppl_proc_decl__name(syntax);
  const Def        *def    = ctx_resolve(self->ctx, (const SyntaxTree *) name, NULL);
  Adr               label  = locate(self, def, self->proc_label_count++);

  mppl_ast__walk_fml_param_list(walker, params, generator);
  mppl_ast__walk_var_decl_part(walker, vars, generator);

  write_label(self, label);
  for (i = 0; i < mppl_fml_param_list__sec_count(params); ++i) {
    MpplFmlParamSec *sec = mppl_fml_param_list__sec(params, i);
    for (j = 0; j < mppl_fml_param_sec__name_count(sec); ++j) {
      MpplToken *name = mppl_fml_param_sec__name(sec, j);
      const Def *def  = ctx_resolve(self->ctx, (const SyntaxTree *) name, NULL);

      Adr label = locate(self, def, ADR_NULL);
      write_inst1(self, "POP", r(GR0));
      write_inst2(self, "ST", r(GR0), adr(label));
      mppl_unref(name);
    }
    mppl_unref(sec);
  }

  if (write_stmt(self, (const AnyMpplStmt *) body, ADR_NULL, ADR_NULL) != ADR_CALL) {
    write_inst0(self, "RET");
  }

  mppl_unref(params);
  mppl_unref(vars);
  mppl_unref(body);
  mppl_unref(name);
}

static void visit_program(const MpplAstWalker *walker, const MpplProgram *syntax, void *generator)
{
  unsigned long i;
  Generator    *self = generator;
  MpplCompStmt *body = mppl_program__stmt(syntax);

  for (i = 0; i < mppl_program__decl_part_count(syntax); ++i) {
    AnyMpplDeclPart *decl_part_syntax = mppl_program__decl_part(syntax, i);
    mppl_ast__walk_decl_part(walker, decl_part_syntax, generator);
    mppl_unref(decl_part_syntax);
  }

  write_stmt(self, (const AnyMpplStmt *) body, ADR_NULL, ADR_NULL);

  mppl_unref(body);
}

int mpplc_codegen_casl2(const Source *source, const MpplProgram *syntax, Ctx *ctx)
{
  MpplAstWalker walker;

  Generator generator;
  generator.ctx              = ctx;
  generator.symbols          = map_new(NULL, NULL);
  generator.current_label    = 0;
  generator.label_count      = (unsigned long) ADR_NORMAL << ADR_KIND_OFFSET;
  generator.var_label_count  = (unsigned long) ADR_VAR << ADR_KIND_OFFSET;
  generator.proc_label_count = (unsigned long) ADR_PROC << ADR_KIND_OFFSET;
  generator.break_label      = ADR_NULL;
  {
    char *output_filename = xmalloc(sizeof(char) * (source->file_name_length + 1));
    sprintf(output_filename, "%.*s.csl", (int) source->file_name_length - 4, source->file_name);
    printf("output: %s\n", output_filename);
    generator.file = fopen(output_filename, "w");
    free(output_filename);
  }

  if (generator.file == NULL) {
    fprintf(stderr, "error: failed to open output file\n");
    return 0;
  }

  mppl_ast_walker__setup(&walker);
  walker.visit_program       = &visit_program;
  walker.visit_proc_decl     = &visit_proc_decl;
  walker.visit_fml_param_sec = &visit_fml_param_sec;
  walker.visit_var_decl      = &visit_var_decl;
  mppl_ast_walker__travel(&walker, syntax, &generator);

  {
    MapIndex index;
    for (map_iterator(generator.symbols, &index); map_next(generator.symbols, &index);) {
      free(map_value(generator.symbols, &index));
    }
    map_free(generator.symbols);
  }

  fclose(generator.file);
  return 1;
}
