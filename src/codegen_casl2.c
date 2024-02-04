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

typedef unsigned long Adr;

#define ADR_KIND_OFFSET (sizeof(Adr) * CHAR_BIT - 2)
#define ADR_NULL        ((Adr) 0)
#define ADR_CALL        ((Adr) 0 - 1)

typedef enum {
  ADR_NORMAL = 1,
  ADR_VAR,
  ADR_PROC
} AdrKind;

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
  case ADR_NULL:
    buf[0] = '\0';
    break;

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

static Adr place(Generator *generator, const Def *def, Adr label)
{
  MapIndex index;
  if (label) {
    if (map_entry(generator->symbols, (void *) def, &index)) {
      return *(Adr *) map_value(generator->symbols, &index) = label;
    } else {
      Adr *p = dup(&label, sizeof(Adr), 1);
      map_update(generator->symbols, &index, (void *) def, p);
      return label;
    }
  } else {
    if (map_entry(generator->symbols, (void *) def, &index)) {
      return *(Adr *) map_value(generator->symbols, &index);
    } else {
      unreachable();
    }
  }
}

static void write_inst(Generator *generator, const char *inst, const char *args[], int count)
{
  int i;

  fprintf(generator->file, "%-10s", label(generator->current_label));
  if (count) {
    fprintf(generator->file, "%-6s", inst);
    for (i = 0; i < count; ++i) {
      if (i) {
        fprintf(generator->file, ", ");
      }
      fprintf(generator->file, "%s", args[i]);
    }
  } else {
    fprintf(generator->file, "%s", inst);
  }
  fprintf(generator->file, "\n");

  generator->current_label = ADR_NULL;
}

static void write_inst0(Generator *generator, const char *inst)
{
  write_inst(generator, inst, NULL, 0);
}

static void write_inst1(Generator *generator, const char *inst, const char *arg1)
{
  const char *args[1];
  args[0] = arg1;
  write_inst(generator, inst, args, 1);
}

static void write_inst2(Generator *generator, const char *inst, const char *arg1, const char *arg2)
{
  const char *args[2];
  args[0] = arg1;
  args[1] = arg2;
  write_inst(generator, inst, args, 2);
}

static void write_inst3(Generator *generator, const char *inst, const char *arg1, const char *arg2, const char *arg3)
{
  const char *args[3];
  args[0] = arg1;
  args[1] = arg2;
  args[2] = arg3;
  write_inst(generator, inst, args, 3);
}

static void write_label(Generator *generator, Adr a)
{
  if (generator->current_label >> ADR_KIND_OFFSET != ADR_NULL) {
    write_inst0(generator, "NOP");
  }
  generator->current_label = a;
}

static Reg write_expr(Generator *self, const AnyMpplExpr *syntax)
{
  write_inst0(self, "NOP");
  return GR0;
}

static Adr write_stmt(Generator *self, const AnyMpplStmt *syntax, Adr source, Adr sink);

static Adr write_if_stmt(Generator *self, const MpplIfStmt *syntax, Adr sink)
{
  AnyMpplExpr *cond_syntax = mppl_if_stmt__cond(syntax);
  AnyMpplStmt *then_syntax = mppl_if_stmt__then_stmt(syntax);
  AnyMpplStmt *else_syntax = mppl_if_stmt__else_stmt(syntax);
  Adr          next_block  = sink ? sink : self->label_count++;
  Adr          false_block = else_syntax ? self->label_count++ : next_block;
  Reg          reg;

  reg = write_expr(self, cond_syntax);
  write_inst2(self, "CPA", r(reg), "#0");
  write_inst1(self, "JZE", adr(false_block));
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

  self->break_label = next_block;

  write_label(self, cond_block);
  write_expr(self, cond_syntax);
  write_inst2(self, "CPA", r(GR1), "#0");
  write_inst1(self, "JZE", adr(next_block));
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
  Adr        label = place(self, def, ADR_NULL);

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
    Adr        label = place(self, def, self->var_label_count++);

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
    Adr        label = place(self, def, self->var_label_count++);

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
  Adr               label  = place(self, def, self->proc_label_count++);

  mppl_ast__walk_fml_param_list(walker, params, generator);
  mppl_ast__walk_var_decl_part(walker, vars, generator);

  write_label(self, label);
  for (i = 0; i < mppl_fml_param_list__sec_count(params); ++i) {
    MpplFmlParamSec *sec = mppl_fml_param_list__sec(params, i);
    for (j = 0; j < mppl_fml_param_sec__name_count(sec); ++j) {
      MpplToken *name = mppl_fml_param_sec__name(sec, j);
      const Def *def  = ctx_resolve(self->ctx, (const SyntaxTree *) name, NULL);

      Adr label = place(self, def, ADR_NULL);
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
