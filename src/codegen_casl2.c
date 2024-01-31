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

#define ADR_KIND_OFFSET (sizeof(unsigned long) * CHAR_BIT - 2)

typedef enum {
  ADR_NULL,
  ADR_NORMAL,
  ADR_VAR,
  ADR_PROC
} AdrKind;

typedef unsigned long Adr;

typedef struct Generator Generator;

struct Generator {
  FILE *file;
  Ctx  *ctx;
  Map  *symbols;
  Adr   current_label;
  Adr   label_count;
  Adr   var_label_count;
  Adr   proc_label_count;
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

  fprintf(generator->file, "%-10s%-6s", label(generator->current_label), inst);
  for (i = 0; i < count; ++i) {
    if (i) {
      fprintf(generator->file, ", ");
    }
    fprintf(generator->file, "%s", args[i]);
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

static void visit_var_decl(const MpplAstWalker *walker, const MpplVarDecl *syntax, void *generator)
{
  Generator    *self = generator;
  unsigned long i;

  for (i = 0; i < mppl_var_decl__name_count(syntax); ++i) {
    MpplToken *name = mppl_var_decl__name(syntax, i);
    const Def *def  = ctx_resolve(self->ctx, (const SyntaxTree *) name, NULL);

    self->current_label = place(self, def, self->var_label_count++);
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
    MpplToken *name = mppl_fml_param_sec__name(syntax, i);
    const Def *def  = ctx_resolve(self->ctx, (const SyntaxTree *) name, NULL);

    self->current_label = place(self, def, self->var_label_count++);
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

  mppl_ast__walk_fml_param_list(walker, params, generator);
  mppl_ast__walk_var_decl_part(walker, vars, generator);

  self->current_label = place(self, NULL, self->proc_label_count++);
  for (i = 0; i < mppl_fml_param_list__sec_count(params); ++i) {
    MpplFmlParamSec *sec = mppl_fml_param_list__sec(params, i);
    for (j = 0; j < mppl_fml_param_sec__name_count(sec); ++j) {
      MpplToken *name  = mppl_fml_param_sec__name(sec, j);
      const Def *def   = ctx_resolve(self->ctx, (const SyntaxTree *) name, NULL);
      Adr        label = place(self, def, ADR_NULL);

      write_inst1(self, "POP", r(GR1));
      write_inst2(self, "ST", r(GR1), adr(label));
      mppl_unref(name);
    }
    mppl_unref(sec);
  }
  mppl_ast__walk_comp_stmt(walker, body, generator);
  write_inst0(self, "RET");

  mppl_unref(params);
  mppl_unref(vars);
  mppl_unref(body);
}

int mpplc_codegen_casl2(const Source *source, const MpplProgram *syntax, Ctx *ctx)
{
  MpplAstWalker walker;

  Generator generator;
  generator.ctx              = ctx;
  generator.symbols          = map_new(NULL, NULL);
  generator.label_count      = (unsigned long) ADR_NORMAL << ADR_KIND_OFFSET;
  generator.var_label_count  = (unsigned long) ADR_VAR << ADR_KIND_OFFSET;
  generator.proc_label_count = (unsigned long) ADR_PROC << ADR_KIND_OFFSET;
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
