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
typedef struct RegUsage   RegUsage;
typedef struct RegState   RegState;
typedef struct BinaryExpr BinaryExpr;
typedef struct NotExpr    NotExpr;
typedef struct CastExpr   CastExpr;
typedef struct VarExpr    VarExpr;
typedef struct LitExpr    LitExpr;
typedef struct Expr       Expr;

struct RegUsage {
  Expr         *user;
  unsigned long order;
};

struct RegState {
  RegUsage      user[8];
  unsigned long order;
};

typedef enum {
  EXPR_BINARY,
  EXPR_NOT,
  EXPR_CAST,
  EXPR_VAR,
  EXPR_LIT
} ExprKind;

typedef enum {
  BINARY_ADD,
  BINARY_SUB,
  BINARY_MUL,
  BINARY_DIV,
  BINARY_EQ,
  BINARY_NE,
  BINARY_LT,
  BINARY_LE,
  BINARY_GT,
  BINARY_GE,
  BINARY_AND,
  BINARY_OR
} BinaryExprKind;

struct Expr {
  ExprKind kind;
  Reg      reg;
  int      spill;
};

struct BinaryExpr {
  ExprKind       kind;
  Reg            reg;
  int            spill;
  BinaryExprKind op;
  Expr          *lhs;
  Expr          *rhs;
};

struct NotExpr {
  ExprKind kind;
  Reg      reg;
  int      spill;
  Expr    *expr;
};

struct CastExpr {
  ExprKind kind;
  Reg      reg;
  int      spill;
  Expr    *expr;
};

struct VarExpr {
  ExprKind   kind;
  Reg        reg;
  int        spill;
  const Def *def;
  Expr      *index;
};

struct LitExpr {
  ExprKind      kind;
  Reg           reg;
  int           spill;
  unsigned long value;
  int           hex;
};

typedef struct Generator Generator;

struct Generator {
  FILE *file;
  Ctx  *ctx;
  Map  *symbols;
  Adr   current_label;
  Adr   label_count;
  Adr   var_label_count;
  Adr   proc_label_count;
  Adr   break_label;

  int builtin_write_integer;
  int builtin_write_boolean;
  int builtin_write_string;
  int builtin_write_char;
  int builtin_read_integer;
  int builtin_read_char;
  int builtin_read_newline;
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

static RegState reg_state(void)
{
  RegState      self;
  unsigned long i;
  for (i = 0; i < 8; ++i) {
    self.user[i].user  = NULL;
    self.user[i].order = -1ul;
  }
  return self;
}

static void reg_state_use(RegState *self, Reg reg, Expr *user)
{
  self->user[reg].user  = user;
  self->user[reg].order = self->order++;
}

static void reg_state_release(RegState *self, Reg reg)
{
  self->user[reg].user  = NULL;
  self->user[reg].order = -1ul;
}

static Reg reg_state_vacant(RegState *self)
{
  unsigned long i;
  unsigned long oldest = -1ul;

  for (i = 1; i < 8; ++i) {
    if (!self->user[i].user) {
      return (Reg) i;
    }
  }

  for (i = 1; i < 8; ++i) {
    if (self->user[i].order < oldest) {
      oldest = self->user[i].order;
    }
  }

  self->user[oldest].user->spill = 1;
  reg_state_release(self, (Reg) oldest);
  return (Reg) oldest;
}

static Expr *expr_create_tree(Ctx *ctx, const AnyMpplExpr *syntax)
{
  switch (mppl_expr__kind(syntax)) {
  case MPPL_EXPR_BINARY: {
    const MpplBinaryExpr *binary_syntax = (const MpplBinaryExpr *) syntax;
    AnyMpplExpr          *lhs_syntax    = mppl_binary_expr__lhs(binary_syntax);
    AnyMpplExpr          *rhs_syntax    = mppl_binary_expr__rhs(binary_syntax);
    MpplToken            *op_syntax     = mppl_binary_expr__op_token(binary_syntax);
    Expr                 *result;

    if (lhs_syntax) {
      BinaryExpr *self = malloc(sizeof(BinaryExpr));
      self->kind       = EXPR_BINARY;
      self->spill      = 0;
      self->lhs        = expr_create_tree(ctx, lhs_syntax);
      self->rhs        = expr_create_tree(ctx, rhs_syntax);

      switch (syntax_tree_kind((const SyntaxTree *) op_syntax)) {
      case SYNTAX_PLUS_TOKEN:
        self->op = BINARY_ADD;
        break;

      case SYNTAX_MINUS_TOKEN:
        self->op = BINARY_SUB;
        break;

      case SYNTAX_STAR_TOKEN:
        self->op = BINARY_MUL;
        break;

      case SYNTAX_DIV_KW:
        self->op = BINARY_DIV;
        break;

      case SYNTAX_EQUAL_TOKEN:
        self->op = BINARY_EQ;
        break;

      case SYNTAX_NOTEQ_TOKEN:
        self->op = BINARY_NE;
        break;

      case SYNTAX_LESS_TOKEN:
        self->op = BINARY_LT;
        break;

      case SYNTAX_LESSEQ_TOKEN:
        self->op = BINARY_LE;
        break;

      case SYNTAX_GREATER_TOKEN:
        self->op = BINARY_GT;
        break;

      case SYNTAX_GREATEREQ_TOKEN:
        self->op = BINARY_GE;
        break;

      case SYNTAX_AND_KW:
        self->op = BINARY_AND;
        break;

      case SYNTAX_OR_KW:
        self->op = BINARY_OR;
        break;

      default:
        unreachable();
      }

      result = (Expr *) self;
    } else {
      switch (syntax_tree_kind((const SyntaxTree *) op_syntax)) {
      case SYNTAX_PLUS_TOKEN:
        result = expr_create_tree(ctx, rhs_syntax);
        break;

      case SYNTAX_MINUS_TOKEN: {
        BinaryExpr *self = malloc(sizeof(BinaryExpr));
        self->kind       = EXPR_BINARY;
        self->spill      = 0;
        {
          LitExpr *zero = malloc(sizeof(LitExpr));
          zero->kind    = EXPR_LIT;
          zero->spill   = 0;
          zero->value   = 0;
          zero->hex     = 0;
          self->lhs     = (Expr *) zero;
        }
        self->rhs = expr_create_tree(ctx, rhs_syntax);
        self->op  = BINARY_SUB;

        result = (Expr *) self;
        break;
      }

      default:
        unreachable();
      }
    }
    mppl_unref(lhs_syntax);
    mppl_unref(rhs_syntax);
    mppl_unref(op_syntax);
    return result;
  }

  case MPPL_EXPR_PAREN: {
    const MpplParenExpr *paren_syntax = (const MpplParenExpr *) syntax;
    AnyMpplExpr         *expr_syntax  = mppl_paren_expr__expr(paren_syntax);
    Expr                *result       = expr_create_tree(ctx, expr_syntax);

    mppl_unref(expr_syntax);
    return result;
  }

  case MPPL_EXPR_NOT: {
    const MpplNotExpr *not_syntax  = (const MpplNotExpr *) syntax;
    AnyMpplExpr       *expr_syntax = mppl_not_expr__expr(not_syntax);

    NotExpr *self = malloc(sizeof(NotExpr));
    self->kind    = EXPR_NOT;
    self->reg     = GR0;
    self->spill   = 0;
    self->expr    = expr_create_tree(ctx, expr_syntax);

    mppl_unref(expr_syntax);
    return (Expr *) self;
  }

  case MPPL_EXPR_CAST: {
    const MpplCastExpr *cast_syntax = (const MpplCastExpr *) syntax;
    AnyMpplExpr        *expr_syntax = mppl_cast_expr__expr(cast_syntax);

    CastExpr *self = malloc(sizeof(CastExpr));
    self->kind     = EXPR_CAST;
    self->reg      = GR0;
    self->spill    = 0;
    self->expr     = expr_create_tree(ctx, expr_syntax);

    mppl_unref(expr_syntax);
    return (Expr *) self;
  }

  case MPPL_EXPR_VAR: {
    const AnyMpplVar *var_syntax = (const AnyMpplVar *) syntax;

    VarExpr *self = malloc(sizeof(VarExpr));
    self->kind    = EXPR_VAR;
    self->reg     = GR0;
    self->spill   = 0;

    switch (mppl_var__kind(var_syntax)) {
    case MPPL_VAR_ENTIRE: {
      MpplEntireVar *entire_syntax = (MpplEntireVar *) var_syntax;
      MpplToken     *name_syntax   = mppl_entire_var__name(entire_syntax);

      self->def   = ctx_resolve(ctx, (const SyntaxTree *) name_syntax, NULL);
      self->index = NULL;

      mppl_unref(name_syntax);
      break;
    }

    case MPPL_VAR_INDEXED: {
      MpplIndexedVar *indexed_syntax = (MpplIndexedVar *) var_syntax;
      MpplToken      *name_syntax    = mppl_indexed_var__name(indexed_syntax);
      AnyMpplExpr    *index_syntax   = mppl_indexed_var__expr(indexed_syntax);

      self->def   = ctx_resolve(ctx, (const SyntaxTree *) name_syntax, NULL);
      self->index = expr_create_tree(ctx, index_syntax);

      mppl_unref(name_syntax);
      mppl_unref(index_syntax);
      break;
    }

    default:
      unreachable();
    }

    return (Expr *) self;
  }

  case MPPL_EXPR_LIT: {
    const AnyMpplLit *lit_syntax = (const AnyMpplLit *) syntax;

    LitExpr *self = malloc(sizeof(LitExpr));
    self->kind    = EXPR_LIT;
    self->reg     = GR0;
    self->spill   = 0;

    switch (mppl_lit__kind(lit_syntax)) {
    case MPPL_LIT_BOOLEAN:
      self->value = mppl_lit_boolean__to_int((const MpplBooleanLit *) lit_syntax);
      self->hex   = 0;
      break;

    case MPPL_LIT_NUMBER:
      self->value = mppl_lit_number__to_long((const MpplNumberLit *) lit_syntax);
      self->hex   = 0;
      break;

    case MPPL_LIT_STRING: {
      char *string = mppl_lit_string__to_string((const MpplStringLit *) lit_syntax);
      self->value  = (unsigned long) string[0];
      self->hex    = 1;
      free(string);
      break;
    }

    default:
      unreachable();
    }

    return (Expr *) self;
  }

  default:
    unreachable();
  }
}

static unsigned long expr_optimize_order(Expr *self)
{
  switch (self->kind) {
  case EXPR_BINARY: {
    BinaryExpr   *expr         = (BinaryExpr *) self;
    unsigned long lhs_priority = expr_optimize_order(expr->lhs);
    unsigned long rhs_priority = expr_optimize_order(expr->rhs);

    if (expr->op == BINARY_AND || expr->op == BINARY_OR) {
      return lhs_priority > rhs_priority ? lhs_priority : rhs_priority;
    } else if (lhs_priority == rhs_priority) {
      return lhs_priority + 1;
    } else if (lhs_priority > rhs_priority) {
      return lhs_priority;
    } else {
      Expr *tmp = expr->lhs;
      expr->lhs = expr->rhs;
      expr->rhs = tmp;
      return rhs_priority;
    }
  }

  case EXPR_NOT: {
    NotExpr *expr = (NotExpr *) self;
    return expr_optimize_order(expr->expr);
  }

  case EXPR_CAST: {
    CastExpr *expr = (CastExpr *) self;
    return expr_optimize_order(expr->expr);
  }

  case EXPR_VAR: {
    VarExpr *expr = (VarExpr *) self;
    if (expr->index) {
      return expr_optimize_order(expr->index);
    } else {
      return 1;
    }
  }

  case EXPR_LIT:
    return 1;

  default:
    unreachable();
  }
}

static void expr_assign_reg(Expr *self, Reg reg, RegState *state)
{
  switch (self->kind) {
  case EXPR_BINARY: {
    BinaryExpr *expr = (BinaryExpr *) self;
    if (expr->op == BINARY_AND || expr->op == BINARY_OR) {
      expr_assign_reg(expr->lhs, reg, state);
      reg_state_release(state, reg);
      expr_assign_reg(expr->rhs, reg, state);
      reg_state_release(state, reg);
    } else {
      Reg rhs;
      expr_assign_reg(expr->lhs, reg, state);
      rhs = reg_state_vacant(state);
      expr_assign_reg(expr->rhs, rhs, state);
      reg_state_release(state, reg);
      reg_state_release(state, rhs);
    }
    break;
  }

  case EXPR_NOT: {
    NotExpr *expr = (NotExpr *) self;
    expr_assign_reg(expr->expr, reg, state);
    reg_state_release(state, reg);
    break;
  }

  case EXPR_CAST: {
    CastExpr *expr = (CastExpr *) self;
    expr_assign_reg(expr->expr, reg, state);
    reg_state_release(state, reg);
    break;
  }

  case EXPR_VAR: {
    VarExpr *expr = (VarExpr *) self;
    if (expr->index) {
      expr_assign_reg(expr->index, reg, state);
      reg_state_release(state, reg);
    }
  }

  case EXPR_LIT:
    /* do nothing */
    break;

  default:
    unreachable();
  }

  reg_state_use(state, self->reg = reg, self);
}

static Expr *expr_new(Ctx *ctx, const AnyMpplExpr *syntax)
{
  Expr *self = expr_create_tree(ctx, syntax);
  expr_optimize_order(self);
  return self;
}

static void expr_free(Expr *self)
{
  if (self) {
    switch (self->kind) {
    case EXPR_BINARY: {
      BinaryExpr *expr = (BinaryExpr *) self;
      expr_free(expr->lhs);
      expr_free(expr->rhs);
      break;
    }

    case EXPR_NOT: {
      NotExpr *expr = (NotExpr *) self;
      expr_free(expr->expr);
      break;
    }

    case EXPR_CAST: {
      CastExpr *expr = (CastExpr *) self;
      expr_free(expr->expr);
      break;
    }

    case EXPR_VAR: {
      VarExpr *expr = (VarExpr *) self;
      if (expr->index) {
        expr_free(expr->index);
      }
      break;
    }

    case EXPR_LIT:
      /* do nothing */
      break;

    default:
      unreachable();
    }
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
  if (self->current_label && self->current_label != a) {
    write_inst0(self, "NOP ; emit for unused label");
  }
  self->current_label = a;
}

static void write_expr_core(Generator *self, const Expr *expr, Adr sink);

static void write_relational_expr(Generator *self, const char *inst, const BinaryExpr *expr, int reverse, Adr sink)
{
  Adr else_block = self->label_count++;
  Adr next_block = sink ? sink : self->label_count++;

  write_expr_core(self, expr->lhs, ADR_NULL);
  write_expr_core(self, expr->rhs, ADR_NULL);
  if (expr->lhs->spill) {
    write_inst1(self, "POP", r(expr->lhs->reg));
  }
  if (reverse) {
    write_inst2(self, "CPA", r1(expr->lhs->reg), r2(expr->rhs->reg));
  } else {
    write_inst2(self, "CPA", r1(expr->lhs->reg), r2(expr->rhs->reg));
  }
  write_inst1(self, inst, adr(else_block));
  write_inst2(self, "LAD", r(expr->reg), "1");
  write_inst1(self, "JUMP", adr(next_block));
  write_label(self, else_block);
  write_inst2(self, "LAD", r(expr->reg), "0");

  if (!sink) {
    write_label(self, next_block);
  }
}

static void write_arithmetic_expr(Generator *self, const char *inst, const BinaryExpr *expr)
{
  write_expr_core(self, expr->lhs, ADR_NULL);
  write_expr_core(self, expr->rhs, ADR_NULL);
  if (expr->lhs->spill) {
    write_inst1(self, "POP", r(expr->lhs->reg));
  }
  write_inst2(self, inst, r1(expr->lhs->reg), r2(expr->rhs->reg));
  if (expr->reg != expr->lhs->reg) {
    write_inst3(self, "LAD", r(expr->reg), "0", x(expr->lhs->reg));
  }
}

static void write_logical_expr(Generator *self, const char *inst, const BinaryExpr *expr, Adr sink)
{
  Adr next_block = sink ? sink : self->label_count++;
  if (expr->reg != expr->lhs->reg) {
    Adr else_block = self->label_count++;
    write_expr_core(self, expr->lhs, ADR_NULL);
    write_inst2(self, "CPA", r(expr->lhs->reg), "1");
    write_inst1(self, inst, adr(else_block));
    write_expr_core(self, expr->rhs, ADR_NULL);
    if (expr->reg != expr->rhs->reg) {
      write_inst3(self, "LAD", r(expr->reg), "0", x(expr->rhs->reg));
    }
    write_inst1(self, "JUMP", adr(next_block));
    write_label(self, else_block);
    write_inst3(self, "LAD", r(expr->reg), "0", x(expr->lhs->reg));
  } else {
    write_expr_core(self, expr->lhs, ADR_NULL);
    write_inst2(self, "CPA", r(expr->lhs->reg), "1");
    write_inst1(self, inst, adr(next_block));
    if (expr->reg != expr->rhs->reg) {
      write_expr_core(self, expr->rhs, ADR_NULL);
      write_inst3(self, "LAD", r(expr->reg), "0", x(expr->rhs->reg));
    } else {
      write_expr_core(self, expr->rhs, next_block);
    }
  }

  if (!sink) {
    write_label(self, next_block);
  }
}

static void write_binary_expr(Generator *self, const BinaryExpr *expr, Adr sink)
{
  switch (expr->op) {
  case BINARY_EQ:
    write_relational_expr(self, "JNZ", expr, 0, sink);
    break;

  case BINARY_NE:
    write_relational_expr(self, "JZE", expr, 0, sink);
    break;

  case BINARY_LT:
    write_relational_expr(self, "JMI", expr, 0, sink);
    break;

  case BINARY_LE:
    write_relational_expr(self, "JPL", expr, 1, sink);
    break;

  case BINARY_GT:
    write_relational_expr(self, "JPL", expr, 0, sink);
    break;

  case BINARY_GE:
    write_relational_expr(self, "JMI", expr, 1, sink);
    break;

  case BINARY_ADD:
    write_arithmetic_expr(self, "ADDA", expr);
    break;

  case BINARY_SUB:
    write_arithmetic_expr(self, "SUBA", expr);
    break;

  case BINARY_MUL:
    write_arithmetic_expr(self, "MULA", expr);
    break;

  case BINARY_DIV:
    write_arithmetic_expr(self, "DIVA", expr);
    break;

  case BINARY_AND:
    write_logical_expr(self, "JNZ", expr, sink);
    break;

  case BINARY_OR:
    write_logical_expr(self, "JZE", expr, sink);
    break;

  default:
    unreachable();
  }
}

static void write_not_expr(Generator *self, const NotExpr *expr)
{
  write_expr_core(self, expr->expr, ADR_NULL);
  write_inst2(self, "XOR", r(expr->expr->reg), "1");
  if (expr->reg != expr->expr->reg) {
    write_inst3(self, "LAD", r(expr->reg), "0", x(expr->expr->reg));
  }
}

static void write_cast_expr(Generator *self, const CastExpr *expr, Adr sink)
{
  if (expr->reg != expr->expr->reg) {
    write_expr_core(self, expr->expr, ADR_NULL);
    write_inst3(self, "LAD", r(expr->reg), "0", x(expr->expr->reg));
  } else {
    write_expr_core(self, expr->expr, sink);
  }
}

static void write_var(Generator *self, const VarExpr *expr)
{
  Adr label = locate(self, expr->def, ADR_NULL);
  if (expr->index) {
    write_expr_core(self, expr->index, ADR_NULL);
    write_inst3(self, "LD", r(expr->reg), adr(label), x(expr->index->reg));
  } else if (def_kind(expr->def) == DEF_PARAM) {
    write_inst2(self, "LD", r(expr->reg), adr(label));
    write_inst3(self, "LD", r(expr->reg), "0", x(expr->reg));
  } else {
    write_inst2(self, "LD", r(expr->reg), adr(label));
  }
}

static void write_lit(Generator *self, const LitExpr *expr)
{
  char buf[16];
  if (expr->hex) {
    sprintf(buf, "#%04lX", expr->value);
  } else {
    sprintf(buf, "%lu", expr->value);
  }
  write_inst2(self, "LAD", r(expr->reg), buf);
}

static void write_expr_core(Generator *self, const Expr *expr, Adr sink)
{
  switch (expr->kind) {
  case EXPR_BINARY:
    write_binary_expr(self, (const BinaryExpr *) expr, sink);
    break;

  case EXPR_NOT:
    write_not_expr(self, (const NotExpr *) expr);
    break;

  case EXPR_CAST:
    write_cast_expr(self, (const CastExpr *) expr, sink);
    break;

  case EXPR_VAR:
    write_var(self, (const VarExpr *) expr);
    break;

  case EXPR_LIT:
    write_lit(self, (const LitExpr *) expr);
    break;

  default:
    unreachable();
  }

  if (expr->spill) {
    write_inst1(self, "PUSH", r(expr->reg));
  }
}

static Reg write_expr(Generator *self, const AnyMpplExpr *syntax, Adr sink)
{
  Expr    *expr  = expr_new(self->ctx, syntax);
  RegState state = reg_state();
  Reg      reg   = reg_state_vacant(&state);

  expr_assign_reg(expr, reg, &state);
  write_expr_core(self, expr, sink);
  expr_free(expr);
  return reg;
}

static Adr write_stmt(Generator *self, const AnyMpplStmt *syntax, Adr source, Adr sink);

static Adr write_assign_stmt(Generator *self, const MpplAssignStmt *syntax)
{
  AnyMpplVar  *lhs_syntax = mppl_assign_stmt__lhs(syntax);
  AnyMpplExpr *rhs_syntax = mppl_assign_stmt__rhs(syntax);

  switch (mppl_var__kind(lhs_syntax)) {
  case MPPL_VAR_ENTIRE: {
    MpplEntireVar *entire_syntax = (MpplEntireVar *) lhs_syntax;
    MpplToken     *name_syntax   = mppl_entire_var__name(entire_syntax);
    const Def     *def           = ctx_resolve(self->ctx, (const SyntaxTree *) name_syntax, NULL);
    Adr            label         = locate(self, def, ADR_NULL);

    Reg reg = write_expr(self, rhs_syntax, ADR_NULL);
    write_inst2(self, "ST", r(reg), adr(label));
    mppl_unref(name_syntax);
    break;
  }

  case MPPL_VAR_INDEXED: {
    MpplIndexedVar *indexed_syntax = (MpplIndexedVar *) lhs_syntax;
    MpplToken      *name_syntax    = mppl_indexed_var__name(indexed_syntax);
    AnyMpplExpr    *index_syntax   = mppl_indexed_var__expr(indexed_syntax);
    const Def      *def            = ctx_resolve(self->ctx, (const SyntaxTree *) name_syntax, NULL);
    Adr             label          = locate(self, def, ADR_NULL);

    RegState state = reg_state();
    Expr    *value = expr_new(self->ctx, rhs_syntax);
    Expr    *index = expr_new(self->ctx, index_syntax);
    Reg      value_reg, index_reg;

    value_reg = reg_state_vacant(&state);
    expr_assign_reg(value, value_reg, &state);
    index_reg = reg_state_vacant(&state);
    expr_assign_reg(index, index_reg, &state);
    reg_state_release(&state, value_reg);
    reg_state_release(&state, index_reg);

    write_expr_core(self, value, ADR_NULL);
    write_expr_core(self, index, ADR_NULL);

    write_inst3(self, "ST", r(value_reg), adr(label), x(index_reg));
    mppl_unref(name_syntax);
    mppl_unref(index_syntax);
    break;
  }

  default:
    unreachable();
  }

  mppl_unref(lhs_syntax);
  mppl_unref(rhs_syntax);

  return ADR_NULL;
}

static Adr write_if_stmt(Generator *self, const MpplIfStmt *syntax, Adr sink)
{
  AnyMpplExpr *cond_syntax = mppl_if_stmt__cond(syntax);
  AnyMpplStmt *then_syntax = mppl_if_stmt__then_stmt(syntax);
  AnyMpplStmt *else_syntax = mppl_if_stmt__else_stmt(syntax);
  Adr          next_block  = sink ? sink : self->label_count++;
  Adr          false_block = else_syntax ? self->label_count++ : next_block;
  Reg          reg;

  reg = write_expr(self, cond_syntax, ADR_NULL);
  write_inst2(self, "CPA", r(reg), "0");
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
  Reg          reg;

  self->break_label = next_block;

  write_label(self, cond_block);
  reg = write_expr(self, cond_syntax, ADR_NULL);
  write_inst2(self, "CPA", r(reg), "0");
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
  long              i;
  MpplToken        *name   = mppl_call_stmt__name(syntax);
  MpplActParamList *params = mppl_call_stmt__act_param_list(syntax);
  const Def        *def    = ctx_resolve(self->ctx, (const SyntaxTree *) name, NULL);
  Adr               label  = locate(self, def, ADR_NULL);

  for (i = mppl_act_param_list__expr_count(params) - 1; i >= 0; --i) {
    AnyMpplExpr *expr_syntax = mppl_act_param_list__expr(params, i);
    Reg          reg         = write_expr(self, expr_syntax, ADR_NULL);
    write_inst1(self, "PUSH", r(reg));
    mppl_unref(expr_syntax);
  }
  write_inst1(self, "CALL", adr(label));

  mppl_unref(name);
  mppl_unref(params);
  return ADR_NULL;
}

static Adr write_input_stmt(Generator *self, const MpplInputStmt *syntax)
{
  write_inst0(self, "NOP ; placeholder for input statement");
  return ADR_NULL;
}

static Adr write_output_stmt(Generator *self, const MpplOutputStmt *syntax)
{
  write_inst0(self, "NOP ; placeholder for output statement");
  return ADR_NULL;
}

static Adr write_stmt(Generator *self, const AnyMpplStmt *syntax, Adr source, Adr sink)
{
  switch (mppl_stmt__kind(syntax)) {
  case MPPL_STMT_ASSIGN:
    return write_assign_stmt(self, (const MpplAssignStmt *) syntax);

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
    return ADR_CALL;

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
