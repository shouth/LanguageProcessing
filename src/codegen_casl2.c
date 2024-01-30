#include <stdio.h>

#include "compiler.h"
#include "context.h"
#include "context_fwd.h"
#include "map.h"
#include "mppl_syntax.h"
#include "mppl_syntax_ext.h"
#include "source.h"
#include "utility.h"

typedef unsigned long    LabelCount;
typedef struct Label     Label;
typedef struct Generator Generator;

struct Label {
  const char *prefix;
  LabelCount  number;
};

struct Generator {
  FILE      *file;
  const Ctx *ctx;
  Map       *symbols;
  LabelCount var_label;
  LabelCount proc_label;
  LabelCount arg_label;
  LabelCount label;
  int        has_label;
};

static const Label *symbol_for(Generator *writer, const Def *def)
{
  MapIndex index;
  if (!map_entry(writer->symbols, (void *) def, &index)) {
    Label *label = xmalloc(sizeof(Label));
    switch (def_kind(def)) {
    case DEF_VAR:
      label->prefix = "V";
      label->number = writer->var_label++;
      break;

    case DEF_PARAM:
      label->prefix = "A";
      label->number = writer->arg_label++;
      break;

    case DEF_PROC:
      label->prefix = "P";
      label->number = writer->proc_label++;
      break;

    default:
      unreachable();
    }
    map_update(writer->symbols, &index, (void *) def, label);
  }
  return map_value(writer->symbols, &index);
}

static int write_label_for(Generator *writer, const Def *def)
{
  const Label *symbol = symbol_for(writer, def);
  return fprintf(writer->file, "%s%lu", symbol->prefix, symbol->number);
}

static LabelCount create_label(Generator *writer)
{
  return writer->label++;
}

static int write_label(Generator *writer, LabelCount label)
{
  return fprintf(writer->file, "L%lu", label);
}

static void write_stmt(Generator *writer, const AnyMpplStmt *stmt)
{
}

static void visit_var_decl(const MpplAstWalker *walker, const MpplVarDecl *syntax, void *writer)
{
  Generator *w = writer;
}

static void visit_proc_decl(const MpplAstWalker *walker, const MpplProcDecl *syntax, void *writer)
{
  Generator *w = writer;
}

static void visit_program(const MpplAstWalker *walker, const MpplProgram *syntax, void *writer)
{
  Generator *w = writer;
}

int mpplc_codegen_casl2(const Source *source, const MpplProgram *syntax, const Ctx *ctx)
{
  MpplAstWalker walker;

  Generator writer;
  writer.ctx        = ctx;
  writer.symbols    = map_new(NULL, NULL);
  writer.var_label  = 0;
  writer.proc_label = 0;
  writer.arg_label  = 0;
  writer.label      = 0;
  {
    char         *output_filename = xmalloc(sizeof(char) * (source->file_name_length + 1));
    unsigned long offset          = sscanf(output_filename, "%s.mpl", source->file_name);
    sprintf(output_filename + offset, ".mpl");
    writer.file = fopen(output_filename, "w");
    free(output_filename);
  }

  if (writer.file == NULL) {
    fprintf(stderr, "error: failed to open output file\n");
    return 0;
  }

  mppl_ast_walker__setup(&walker);
  mppl_ast_walker__travel(&walker, syntax, &writer);

  {
    MapIndex index;
    for (map_iterator(writer.symbols, &index); map_next(writer.symbols, &index);) {
      free(map_value(writer.symbols, &index));
    }
    map_free(writer.symbols);
  }

  fclose(writer.file);
  return 1;
}
