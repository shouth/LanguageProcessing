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

#define ADR_KIND_OFFSET (sizeof(Adr) * CHAR_BIT - 4)
#define ADR_NULL        ((Adr) 0)
#define ADR_CALL        ((Adr) 0 - 1)

typedef enum {
  ADR_NORMAL = 1,
  ADR_VAR,
  ADR_ARG,
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
  Adr   arg_label_count;
  Adr   proc_label_count;
  Adr   break_label;

  int builtin_error_overflow;
  int builtin_error_zero_division;
  int builtin_error_range;
  int builtin_write_integer;
  int builtin_write_boolean;
  int builtin_write_string;
  int builtin_write_char;
  int builtin_write_newline;
  int builtin_flush;
  int builtin_read_integer;
  int builtin_read_char;
  int builtin_read_line;
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

  case ADR_ARG:
    sprintf(buf, "A%lu", a);
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
  return expr_create_tree(ctx, syntax);
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
    write_inst0(self, "NOP");
  }
  self->current_label = a;
}

static void write_builtin(Generator *self, const char **lines, unsigned long count)
{
  unsigned long i;
  for (i = 0; i < count; ++i) {
    fprintf(self->file, "%s\n", lines[i]);
  }
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

static void write_arithmetic_expr(Generator *self, const char *inst, const BinaryExpr *expr, int check_overflow, int check_zero_division)
{
  write_expr_core(self, expr->lhs, ADR_NULL);
  write_expr_core(self, expr->rhs, ADR_NULL);
  if (check_zero_division) {
    write_inst2(self, "CPA", r1(expr->rhs->reg), "=0");
    write_inst1(self, "JZE", "E0DIV");
    self->builtin_error_zero_division = 1;
  }
  if (expr->lhs->spill) {
    write_inst1(self, "POP", r(expr->lhs->reg));
  }
  write_inst2(self, inst, r1(expr->lhs->reg), r2(expr->rhs->reg));
  if (check_overflow) {
    write_inst1(self, "JOV", "EOVF");
    self->builtin_error_overflow = 1;
  }
  if (expr->reg != expr->lhs->reg) {
    write_inst2(self, "LD", r1(expr->reg), r2(expr->lhs->reg));
  }
}

static void write_logical_expr(Generator *self, const char *inst, const BinaryExpr *expr, Adr sink)
{
  Adr next_block = sink ? sink : self->label_count++;
  if (expr->reg != expr->lhs->reg) {
    Adr else_block = self->label_count++;
    write_expr_core(self, expr->lhs, ADR_NULL);
    write_inst2(self, "CPA", r(expr->lhs->reg), "=1");
    write_inst1(self, inst, adr(else_block));
    write_expr_core(self, expr->rhs, ADR_NULL);
    if (expr->reg != expr->rhs->reg) {
      write_inst2(self, "LD", r1(expr->reg), r2(expr->rhs->reg));
    }
    write_inst1(self, "JUMP", adr(next_block));
    write_label(self, else_block);
    write_inst2(self, "LD", r1(expr->reg), r2(expr->lhs->reg));
  } else {
    write_expr_core(self, expr->lhs, ADR_NULL);
    write_inst2(self, "CPA", r(expr->lhs->reg), "=1");
    write_inst1(self, inst, adr(next_block));
    if (expr->reg != expr->rhs->reg) {
      write_expr_core(self, expr->rhs, ADR_NULL);
      write_inst2(self, "LD", r1(expr->reg), r2(expr->rhs->reg));
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
    write_arithmetic_expr(self, "ADDA", expr, 1, 0);
    break;

  case BINARY_SUB:
    write_arithmetic_expr(self, "SUBA", expr, 0, 0);
    break;

  case BINARY_MUL:
    write_arithmetic_expr(self, "MULA", expr, 1, 0);
    break;

  case BINARY_DIV:
    write_arithmetic_expr(self, "DIVA", expr, 0, 1);
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
  write_inst2(self, "XOR", r(expr->expr->reg), "=1");
  if (expr->reg != expr->expr->reg) {
    write_inst2(self, "LD", r1(expr->reg), r2(expr->expr->reg));
  }
}

static void write_cast_expr(Generator *self, const CastExpr *expr, Adr sink)
{
  if (expr->reg != expr->expr->reg) {
    write_expr_core(self, expr->expr, ADR_NULL);
    write_inst2(self, "LD", r1(expr->reg), r2(expr->expr->reg));
  } else {
    write_expr_core(self, expr->expr, sink);
  }
}

static void write_var(Generator *self, const VarExpr *expr)
{
  Adr label = locate(self, expr->def, ADR_NULL);
  if (expr->index) {
    const Type   *type   = ctx_type_of(self->ctx, def_syntax(expr->def), NULL);
    unsigned long length = array_type_length((const ArrayType *) type);

    if (length > 0) {
      char buf[16];
      sprintf(buf, "=%lu", length - 1);
      write_expr_core(self, expr->index, ADR_NULL);
      write_inst2(self, "CPA", r(expr->index->reg), buf);
      write_inst1(self, "JPL", "ERNG");
      write_inst3(self, "LD", r(expr->reg), adr(label), x(expr->index->reg));
    } else {
      write_inst1(self, "JPL", "ERNG");
    }
    self->builtin_error_range = 1;
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
    write_inst2(self, "PUSH", "0", r(expr->reg));
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

    RegState state     = reg_state();
    Expr    *value     = expr_new(self->ctx, rhs_syntax);
    Reg      value_reg = reg_state_vacant(&state);
    expr_assign_reg(value, value_reg, &state);

    write_expr_core(self, value, ADR_NULL);
    if (def_kind(def) == DEF_PARAM) {
      Reg reg = reg_state_vacant(&state);
      write_inst2(self, "LD", r(reg), adr(label));
      write_inst3(self, "ST", r(value_reg), "0", x(reg));
    } else {
      write_inst2(self, "ST", r(value_reg), adr(label));
    }

    expr_free(value);
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

    expr_free(value);
    expr_free(index);
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
  write_inst2(self, "CPA", r(reg), "=1");
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
  write_inst2(self, "CPA", r(reg), "=1");
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

    if (stmt) {
      Adr next = i + 1 < mppl_comp_stmt__stmt_count(syntax) ? ADR_NULL : sink;
      current  = write_stmt(self, stmt, current, next);
    }
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

  if (params) {
    for (i = mppl_act_param_list__expr_count(params) - 1; i >= 0; --i) {
      AnyMpplExpr *expr_syntax = mppl_act_param_list__expr(params, i);

      if (mppl_expr__kind(expr_syntax) == MPPL_EXPR_VAR) {
        const AnyMpplVar *var_syntax = (AnyMpplVar *) expr_syntax;
        switch (mppl_var__kind(var_syntax)) {
        case MPPL_VAR_ENTIRE: {
          MpplEntireVar *entire_syntax = (MpplEntireVar *) var_syntax;
          MpplToken     *name_syntax   = mppl_entire_var__name(entire_syntax);
          const Def     *def           = ctx_resolve(self->ctx, (const SyntaxTree *) name_syntax, NULL);
          Adr            label         = locate(self, def, ADR_NULL);

          if (def_kind(def) == DEF_PARAM) {
            write_inst2(self, "LD", r(GR1), adr(label));
            write_inst2(self, "PUSH", "0", r(GR1));
          } else {
            write_inst1(self, "PUSH", adr(label));
          }

          mppl_unref(name_syntax);
          break;
        }

        case MPPL_VAR_INDEXED: {
          MpplIndexedVar *indexed_syntax = (MpplIndexedVar *) var_syntax;
          MpplToken      *name_syntax    = mppl_indexed_var__name(indexed_syntax);
          AnyMpplExpr    *index_syntax   = mppl_indexed_var__expr(indexed_syntax);
          const Def      *def            = ctx_resolve(self->ctx, (const SyntaxTree *) name_syntax, NULL);
          Adr             label          = locate(self, def, ADR_NULL);

          Reg index = write_expr(self, index_syntax, ADR_NULL);
          write_inst2(self, "PUSH", adr(label), x(index));

          mppl_unref(name_syntax);
          mppl_unref(index_syntax);
          break;
        }

        default:
          unreachable();
        }
      } else {
        RegState state = reg_state();
        Expr    *expr  = expr_new(self->ctx, expr_syntax);
        Reg      reg, tmp;

        reg = reg_state_vacant(&state);
        expr_assign_reg(expr, reg, &state);
        tmp = reg_state_vacant(&state);

        write_expr_core(self, expr, ADR_NULL);
        write_inst2(self, "LAD", r(tmp), "=0");
        write_inst3(self, "ST", r(reg), "0", x(tmp));
        write_inst2(self, "PUSH", "0", x(tmp));

        expr_free(expr);
      }

      mppl_unref(expr_syntax);
    }
  }
  write_inst1(self, "CALL", adr(label));

  mppl_unref(name);
  mppl_unref(params);
  return ADR_NULL;
}

static Adr write_input_stmt(Generator *self, const MpplInputStmt *syntax)
{
  unsigned long  i;
  MpplToken     *read_syntax       = mppl_input_stmt__read_token(syntax);
  MpplInputList *input_list_syntax = mppl_input_stmt__input_list(syntax);

  if (input_list_syntax) {
    for (i = 0; i < mppl_input_list__var_count(input_list_syntax); ++i) {
      AnyMpplVar *var_syntax = mppl_input_list__var(input_list_syntax, i);
      const Type *type       = ctx_type_of(self->ctx, (const SyntaxTree *) var_syntax, NULL);

      switch (mppl_var__kind(var_syntax)) {
      case MPPL_VAR_ENTIRE: {
        MpplEntireVar *entire_syntax = (MpplEntireVar *) var_syntax;
        MpplToken     *name_syntax   = mppl_entire_var__name(entire_syntax);
        const Def     *def           = ctx_resolve(self->ctx, (const SyntaxTree *) name_syntax, NULL);
        Adr            label         = locate(self, def, ADR_NULL);

        switch (def_kind(def)) {
        case DEF_PARAM:
          write_inst2(self, "LD", r(GR1), adr(label));
          break;

        case DEF_VAR:
          write_inst2(self, "LAD", r(GR1), adr(label));
          break;

        default:
          unreachable();
        }

        mppl_unref(name_syntax);
        break;
      }

      case MPPL_VAR_INDEXED: {
        MpplIndexedVar *indexed_syntax = (MpplIndexedVar *) var_syntax;
        MpplToken      *name_syntax    = mppl_indexed_var__name(indexed_syntax);
        AnyMpplExpr    *index_syntax   = mppl_indexed_var__expr(indexed_syntax);
        const Def      *def            = ctx_resolve(self->ctx, (const SyntaxTree *) name_syntax, NULL);
        Adr             label          = locate(self, def, ADR_NULL);

        Reg index = write_expr(self, index_syntax, ADR_NULL);
        write_inst3(self, "LAD", r(GR1), adr(label), x(index));

        mppl_unref(name_syntax);
        mppl_unref(index_syntax);
        break;
      }
      }

      switch (type_kind(type)) {
      case TYPE_CHAR:
        write_inst1(self, "CALL", "RCHAR");
        self->builtin_read_char = 1;
        break;

      case TYPE_INTEGER:
        write_inst1(self, "CALL", "RINT");
        self->builtin_read_integer = 1;
        break;

      default:
        unreachable();
      }
      mppl_unref(var_syntax);
    }
  }

  if (syntax_tree_kind((const SyntaxTree *) read_syntax) == SYNTAX_READLN_KW) {
    write_inst1(self, "CALL", "RLINE");
    self->builtin_read_line = 1;
  }

  mppl_unref(read_syntax);
  mppl_unref(input_list_syntax);
  return ADR_NULL;
}

static Adr write_output_stmt(Generator *self, const MpplOutputStmt *syntax)
{
  unsigned long i;
  MpplToken    *write_syntax       = mppl_output_stmt__write_token(syntax);
  MpplOutList  *output_list_syntax = mppl_output_stmt__output_list(syntax);

  if (output_list_syntax) {
    for (i = 0; i < mppl_out_list__out_value_count(output_list_syntax); ++i) {
      MpplOutValue  *out_value_syntax = mppl_out_list__out_value(output_list_syntax, i);
      AnyMpplExpr   *expr_syntax      = mppl_out_value__expr(out_value_syntax);
      MpplNumberLit *width_syntax     = mppl_out_value__width(out_value_syntax);
      const Type    *type             = ctx_type_of(self->ctx, (const SyntaxTree *) expr_syntax, NULL);

      if (type_kind(type) == TYPE_STRING) {
        const MpplStringLit  *string_syntax = (MpplStringLit *) expr_syntax;
        const RawSyntaxToken *token         = (const RawSyntaxToken *) syntax_tree_raw((const SyntaxTree *) string_syntax);

        char *buf = xmalloc(string_length(token->string) + 2);
        sprintf(buf, "=%s", string_data(token->string));

        write_inst2(self, "LAD", r(GR1), buf);
        write_inst2(self, "LAD", r(GR2), "0");
        write_inst1(self, "CALL", "WSTR");

        free(buf);
      } else {
        Expr    *value_expr = expr_new(self->ctx, expr_syntax);
        Expr    *width_expr = width_syntax ? expr_new(self->ctx, (const AnyMpplExpr *) width_syntax) : NULL;
        RegState state      = reg_state();

        expr_assign_reg(value_expr, GR1, &state);
        if (width_expr) {
          expr_assign_reg(width_expr, GR2, &state);
        }
        reg_state_release(&state, GR1);
        reg_state_release(&state, GR2);

        write_expr_core(self, value_expr, ADR_NULL);
        if (width_expr) {
          write_expr_core(self, width_expr, ADR_NULL);
        } else {
          write_inst2(self, "LAD", r(GR2), "0");
        }

        switch (type_kind(type)) {
        case TYPE_CHAR:
          write_inst1(self, "CALL", "WCHAR");
          self->builtin_write_char = 1;
          break;

        case TYPE_INTEGER:
          write_inst1(self, "CALL", "WINT");
          self->builtin_write_integer = 1;
          break;

        case TYPE_BOOLEAN:
          write_inst1(self, "CALL", "WBOOL");
          self->builtin_write_boolean = 1;
          break;

        default:
          unreachable();
        }

        expr_free(value_expr);
        expr_free(width_expr);
      }

      mppl_unref(expr_syntax);
      mppl_unref(width_syntax);
      mppl_unref(out_value_syntax);
    }
  }

  if (syntax_tree_kind((const SyntaxTree *) write_syntax) == SYNTAX_WRITELN_KW) {
    write_inst1(self, "CALL", "WLINE");
  }
  write_inst1(self, "CALL", "FLUSH");

  mppl_unref(write_syntax);
  mppl_unref(output_list_syntax);
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
    Adr        label = locate(self, def, self->arg_label_count++);

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
  if (params) {
    write_inst1(self, "POP", r(GR1));
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
    write_inst2(self, "PUSH", "0", r(GR1));
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
  Generator    *self  = generator;
  MpplCompStmt *body  = mppl_program__stmt(syntax);
  Adr           start = self->label_count++;
  Adr           main  = self->label_count++;

  write_label(self, start);
  write_inst1(self, "START", adr(main));
  for (i = 0; i < mppl_program__decl_part_count(syntax); ++i) {
    AnyMpplDeclPart *decl_part_syntax = mppl_program__decl_part(syntax, i);
    mppl_ast__walk_decl_part(walker, decl_part_syntax, generator);
    mppl_unref(decl_part_syntax);
  }
  write_label(self, main);
  if (write_stmt(self, (const AnyMpplStmt *) body, ADR_NULL, ADR_NULL) != ADR_CALL) {
    write_inst0(self, "RET");
  }

  if (self->builtin_error_overflow) {
    const char *csl[] = {
      "EOVF      CALL  WLINE",
      "          LAD   GR1, EOVF1",
      "          LAD   GR2, 0",
      "          CALL  WSTR",
      "          CALL  WLINE",
      "          SVC   1",
      "EOVF1     DC    '***** Run-Time Error : Overflow *****'",
    };
    write_builtin(self, csl, sizeof(csl) / sizeof(csl[0]));
    self->builtin_write_string  = 1;
    self->builtin_write_newline = 1;
  }

  if (self->builtin_error_zero_division) {
    const char *csl[] = {
      "E0DIV     CALL  WLINE",
      "          LAD   GR1, E0DIV1",
      "          LAD   GR2, 0",
      "          CALL  WSTR",
      "          CALL  WLINE",
      "          SVC   2",
      "E0DIV1    DC    '***** Run-Time Error : Zero Division *****'",
    };
    write_builtin(self, csl, sizeof(csl) / sizeof(csl[0]));
    self->builtin_write_string  = 1;
    self->builtin_write_newline = 1;
  }

  if (self->builtin_error_range) {
    const char *csl[] = {
      "ERNG      CALL  WLINE",
      "          LAD   GR1, ERNG1",
      "          LAD   GR2, 0",
      "          CALL  WSTR",
      "          CALL  WLINE",
      "          SVC   3",
      "ERNG1     DC    '***** Run-Time Error : Range *****'",
    };
    write_builtin(self, csl, sizeof(csl) / sizeof(csl[0]));
    self->builtin_write_string  = 1;
    self->builtin_write_newline = 1;
  }

  if (self->builtin_write_integer) {
    const char *csl[] = {
      "WINT      LAD   GR7, 0",
      "          CPA   GR1, =0",
      "          JPL   WI1",
      "          JZE   WI1",
      "          LAD   GR4, 0",
      "          SUBA  GR4, GR1",
      "          CPA   GR4, GR1",
      "          JZE   WI6",
      "          LD    GR1, GR4",
      "          LAD   GR7, 1",
      "WI1       LAD   GR6, 6",
      "          LAD   GR0, 0",
      "          ST    GR0, INTBUF, GR6",
      "          SUBA  GR6, =1",
      "          CPA   GR1, =0",
      "          JNZ   WI2",
      "          LAD   GR4, #0030",
      "          ST    GR4, INTBUF, GR6",
      "          JUMP  WI5",
      "WI2       CPA   GR1, =0",
      "          JZE   WI3",
      "          LD    GR5, GR1",
      "          DIVA  GR1, =10",
      "          LD    GR4, GR1",
      "          MULA  GR4, =10",
      "          SUBA  GR5, GR4",
      "          ADDA  GR5, =#0030",
      "          ST    GR5, INTBUF, GR6",
      "          SUBA  GR6, =1",
      "          JUMP  WI2",
      "WI3       CPA   GR7, =0",
      "          JZE   WI4",
      "          LAD   GR4, #002D",
      "          ST    GR4, INTBUF, GR6",
      "          JUMP  WI5",
      "WI4       ADDA  GR6, =1",
      "WI5       LAD   GR1, INTBUF, GR6",
      "          CALL  WSTR",
      "          RET",
      "WI6       LAD   GR1, MMINT",
      "          CALL  WSTR",
      "          RET",
      "MMINT     DC    '-32768'",
      "INTBUF    DS    8",
    };
    write_builtin(self, csl, sizeof(csl) / sizeof(csl[0]));
    self->builtin_write_string = 1;
  }

  if (self->builtin_write_boolean) {
    const char *csl[] = {
      "WBOOL     CPA   GR1, =0",
      "          JZE   WB1",
      "          LAD   GR1, WT",
      "          JUMP  WB2",
      "WB1       LAD   GR1, WF",
      "WB2       CALL  WSTR",
      "          RET",
      "WT        DC    'TRUE'",
      "WF        DC    'FALSE'",
    };
    write_builtin(self, csl, sizeof(csl) / sizeof(csl[0]));
    self->builtin_write_string = 1;
  }

  if (self->builtin_write_char) {
    const char *csl[] = {
      "WCHAR     LAD   GR6, #0020",
      "          LD    GR7, OBUFSZ",
      "WC1       SUBA  GR2, =1",
      "          JZE   WC2",
      "          JMI   WC2",
      "          ST    GR6, OBUF, GR7",
      "          CALL  BOVFCHK",
      "          JUMP  WC1",
      "WC2       ST    GR1, OBUF, GR7",
      "          CALL  BOVFCHK",
      "          ST    GR7, OBUFSZ",
      "          RET",
    };
    write_builtin(self, csl, sizeof(csl) / sizeof(csl[0]));
  }

  if (self->builtin_write_string) {
    const char *csl[] = {
      "WSTR      LD    GR6, GR1",
      "WS1       LD    GR4, 0, GR6",
      "          JZE   WS2",
      "          ADDA  GR6, =1",
      "          SUBA  GR2, =1",
      "          JUMP  WS1",
      "WS2       LD    GR7, OBUFSZ",
      "          LAD   GR5, #0020",
      "WS3       SUBA  GR2, =1",
      "          JMI   WS4",
      "          ST    GR5, OBUF, GR7",
      "          CALL  BOVFCHK",
      "          JUMP  WS3",
      "WS4       LD    GR4, 0, GR1",
      "          JZE   WS5",
      "          ST    GR4, OBUF, GR7",
      "          ADDA  GR1, =1",
      "          CALL  BOVFCHK",
      "          JUMP  WS4",
      "WS5       ST    GR7, OBUFSZ",
      "          RET",
    };
    write_builtin(self, csl, sizeof(csl) / sizeof(csl[0]));
  }

  if (self->builtin_write_newline) {
    const char *csl[] = {
      "WLINE     LD    GR7, OBUFSZ",
      "          LAD   GR6, #000A",
      "          ST    GR6, OBUF, GR7",
      "          CALL  BOVFCHK",
      "          RET",
    };
    write_builtin(self, csl, sizeof(csl) / sizeof(csl[0]));
  }

  if (self->builtin_write_char || self->builtin_write_string || self->builtin_write_newline) {
    const char *csl[] = {
      "BOVFCHK   ADDA  GR7, =1",
      "          CPA   GR7, =256",
      "          JMI   BOVF1",
      "          CALL  FLUSH",
      "          LD    GR7, OBUFSZ",
      "BOVF1     RET",
    };
    write_builtin(self, csl, sizeof(csl) / sizeof(csl[0]));
    self->builtin_flush = 1;
  }

  if (self->builtin_flush) {
    const char *csl[] = {
      "FLUSH     OUT   OBUF, OBUFSZ",
      "          LAD   GR0, 0",
      "          ST    GR0, OBUFSZ",
      "          RET",
      "OBUF      DS    256",
      "OBUFSZ    DC    0",
    };
    write_builtin(self, csl, sizeof(csl) / sizeof(csl[0]));
  }

  if (self->builtin_read_integer) {
    const char *csl[] = {
      "RINT      NOP",
      "RI1       CALL  RCHAR",
      "          LD    GR7, 0, GR1",
      "          CPA   GR7, =#0020",
      "          JZE   RI1",
      "          CPA   GR7, =#0009",
      "          JZE   RI1",
      "          CPA   GR7, =#0010",
      "          JZE   RI1",
      "          LAD   GR5, 1",
      "          CPA   GR7, =#002D",
      "          JNZ   RI4",
      "          LAD   GR5, 0",
      "          CALL  RCHAR",
      "          LD    GR7, 0, GR1",
      "RI4       LAD   GR6, 0",
      "RI2       CPA   GR7, =0",
      "          JMI   RI3",
      "          CPA   GR7, =9",
      "          JPL   RI3",
      "          MULA  GR6, =10",
      "          ADDA  GR6, GR7",
      "          SUBA  GR6, =0",
      "          CALL  RCHAR",
      "          LD    GR7, 0, GR1",
      "          JUMP  RI2",
      "RI3       ST    GR7, RPBBUF",
      "          ST    GR6, 0, GR1",
      "          CPA   GR5, =0",
      "          JNZ   RI5",
      "          SUBA  GR5, GR6",
      "          ST    GR5, 0, GR1",
      "RI5       RET",
    };
    write_builtin(self, csl, sizeof(csl) / sizeof(csl[0]));
    self->builtin_read_char = 1;
  }

  if (self->builtin_read_char) {
    const char *csl[] = {
      "RCHAR     LD    GR5, RPBBUF",
      "          JZE   RC0",
      "          ST    GR5, 0, GR1",
      "          ST    GR0, RPBBUF",
      "          JUMP  RC3",
      "RC0       LD    GR7, INP",
      "          LD    GR6, IBUFSZ",
      "          JNZ   RC1",
      "          IN    IBUF, IBUFSZ",
      "          LD    GR7, GR0",
      "RC1       CPA   GR7, IBUFSZ",
      "          JNZ   RC2",
      "          LAD   GR5, #0010",
      "          ST    GR5, 0, GR1",
      "          ST    GR0, IBUFSZ",
      "          ST    GR0, INP",
      "          JUMP  RC3",
      "RC2       LD    GR5, IBUF, GR7",
      "          ADDA  GR7, =1",
      "          ST    GR5, 0, GR1",
      "          ST    GR7, INP",
      "RC3       RET",
    };
    write_builtin(self, csl, sizeof(csl) / sizeof(csl[0]));
  }

  if (self->builtin_read_line) {
    const char *csl[] = {
      "RLINE     LAD   GR0, 0",
      "          ST    GR0, IBUFSZ",
      "          ST    GR0, INP",
      "          ST    GR0, RPBBUF",
      "          RET",
    };
    write_builtin(self, csl, sizeof(csl) / sizeof(csl[0]));
  }

  if (self->builtin_read_char || self->builtin_read_line) {
    const char *csl[] = {
      "IBUF      DS    257",
      "IBUFSZ    DC    0",
      "INP       DC    0",
      "RPBBUF    DC    0",
    };
    write_builtin(self, csl, sizeof(csl) / sizeof(csl[0]));
  }

  write_inst0(self, "END");

  mppl_unref(body);
}

int mpplc_codegen_casl2(const Source *source, const MpplProgram *syntax, Ctx *ctx)
{
  MpplAstWalker walker;

  Generator self;
  self.ctx              = ctx;
  self.symbols          = map_new(NULL, NULL);
  self.current_label    = 0;
  self.label_count      = (unsigned long) ADR_NORMAL << ADR_KIND_OFFSET;
  self.var_label_count  = (unsigned long) ADR_VAR << ADR_KIND_OFFSET;
  self.arg_label_count  = (unsigned long) ADR_ARG << ADR_KIND_OFFSET;
  self.proc_label_count = (unsigned long) ADR_PROC << ADR_KIND_OFFSET;
  self.break_label      = ADR_NULL;
  {
    char *output_filename = xmalloc(sizeof(char) * (source->file_name_length + 1));
    sprintf(output_filename, "%.*s.csl", (int) source->file_name_length - 4, source->file_name);
    printf("output: %s\n", output_filename);
    self.file = fopen(output_filename, "w");
    free(output_filename);
  }

  if (self.file == NULL) {
    fprintf(stderr, "error: failed to open output file\n");
    return 0;
  }

  self.builtin_error_overflow      = 0;
  self.builtin_error_zero_division = 0;
  self.builtin_error_range         = 0;
  self.builtin_read_char           = 0;
  self.builtin_read_integer        = 0;
  self.builtin_read_line           = 0;
  self.builtin_write_char          = 0;
  self.builtin_write_string        = 0;
  self.builtin_write_integer       = 0;
  self.builtin_write_boolean       = 0;
  self.builtin_write_newline       = 0;

  mppl_ast_walker__setup(&walker);
  walker.visit_program       = &visit_program;
  walker.visit_proc_decl     = &visit_proc_decl;
  walker.visit_fml_param_sec = &visit_fml_param_sec;
  walker.visit_var_decl      = &visit_var_decl;
  mppl_ast_walker__travel(&walker, syntax, &self);

  {
    MapIndex index;
    for (map_iterator(self.symbols, &index); map_next(self.symbols, &index);) {
      free(map_value(self.symbols, &index));
    }
    map_free(self.symbols);
  }

  fclose(self.file);
  return 1;
}
