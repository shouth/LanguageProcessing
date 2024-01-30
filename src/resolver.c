#include <stdio.h>
#include <string.h>

#include "array.h"
#include "compiler.h"
#include "context.h"
#include "context_fwd.h"
#include "map.h"
#include "mppl_syntax.h"
#include "mppl_syntax_ext.h"
#include "report.h"
#include "source.h"
#include "string.h"
#include "syntax_kind.h"
#include "syntax_tree.h"
#include "utility.h"

typedef struct Scope    Scope;
typedef struct Resolver Resolver;

struct Scope {
  Map   *defs;
  Scope *parent;
};

struct Resolver {
  Scope *scope;
  Ctx   *ctx;
  Array *errors;
};

static void error_def_conflict(Resolver *resolver, const String *name, const SyntaxTree *previous, const SyntaxTree *conflict)
{
  unsigned long previous_offset = syntax_tree_offset(previous);
  unsigned long conflict_offset = syntax_tree_offset(conflict);

  Report *report = report_new(REPORT_KIND_ERROR, conflict_offset, "conflicting definition of `%s`", string_data(name));
  report_annotation(report, previous_offset, previous_offset + string_length(name),
    "previous definition of `%s`", string_data(name));
  report_annotation(report, conflict_offset, conflict_offset + string_length(name), "redefinition of `%s`", string_data(name));
  array_push(resolver->errors, &report);
}

static void error_var_res_failure(Resolver *resolver, const String *name, const SyntaxTree *use)
{
  unsigned long offset = syntax_tree_offset(use);

  Report *report = report_new(REPORT_KIND_ERROR, offset, "failed to resolve `%s`", string_data(name));
  report_annotation(report, offset, offset + string_length(name),
    "use of undeclared variable or parameter `%s`", string_data(name));
  array_push(resolver->errors, &report);
}

static void error_proc_res_failure(Resolver *resolver, const String *name, const SyntaxTree *use)
{
  unsigned long offset = syntax_tree_offset(use);

  Report *report = report_new(REPORT_KIND_ERROR, offset, "failed to resolve `%s`", string_data(name));
  report_annotation(report, offset, offset + string_length(name),
    "use of undeclared procedure `%s`", string_data(name));
  array_push(resolver->errors, &report);
}

static void push_scope(Resolver *resolver)
{
  Scope *scope    = xmalloc(sizeof(Scope));
  scope->defs     = map_new(NULL, NULL);
  scope->parent   = resolver->scope;
  resolver->scope = scope;
}

static void pop_scope(Resolver *resolver)
{
  Scope *scope    = resolver->scope;
  resolver->scope = scope->parent;
  map_free(scope->defs);
  free(scope);
}

static void try_define(Resolver *resolver, DefKind kind, const SyntaxTree *item_syntax, const SyntaxTree *name_syntax)
{
  MapIndex              index;
  const RawSyntaxToken *name_token = (const RawSyntaxToken *) syntax_tree_raw(name_syntax);

  if (resolver->scope && map_entry(resolver->scope->defs, (void *) name_token->string, &index)) {
    const Def *previous = map_value(resolver->scope->defs, &index);
    error_def_conflict(resolver, def_name(previous), def_syntax(previous), item_syntax);
  } else {
    const Def *def = ctx_define(resolver->ctx, kind, name_token->string, item_syntax);
    if (resolver->scope) {
      map_update(resolver->scope->defs, &index, (void *) name_token->string, (void *) def);
    }
  }
}

static const Def *get_def(Resolver *resolver, const String *name)
{
  Scope *scope;
  for (scope = resolver->scope; scope; scope = scope->parent) {
    MapIndex index;
    if (map_entry(scope->defs, (void *) name, &index)) {
      return map_value(scope->defs, &index);
    }
  }
  return NULL;
}

static const Def *try_resolve(Resolver *resolver, const SyntaxTree *syntax, int is_proc)
{
  const RawSyntaxToken *node = (const RawSyntaxToken *) syntax_tree_raw(syntax);
  const Def            *def  = get_def(resolver, node->string);
  if (resolver->scope && def) {
    ctx_resolve(resolver->ctx, syntax, def);
    return def;
  } else {
    if (is_proc) {
      error_proc_res_failure(resolver, node->string, syntax);
    } else {
      error_var_res_failure(resolver, node->string, syntax);
    }
    return NULL;
  }
}

static void error_call_stmt_recursion(Resolver *resolver, const Def *proc, const SyntaxTree *name_syntax)
{
  unsigned long offset = syntax_tree_offset(name_syntax);
  unsigned long length = syntax_tree_text_length(name_syntax);

  Report *report = report_new(REPORT_KIND_ERROR, offset, "recursion is prohibited");
  report_annotation(report, offset, offset + length, "recursive call to `%s`", string_data(def_name(proc)));
  array_push(resolver->errors, &report);
}

static void visit_program(const MpplAstWalker *walker, const MpplProgram *syntax, void *resolver)
{
  MpplToken *name_syntax = mppl_program__name(syntax);
  try_define(resolver, DEF_PROGRAM, (const SyntaxTree *) syntax, (SyntaxTree *) name_syntax);
  mppl_unref(name_syntax);
  push_scope(resolver);
  mppl_ast__walk_program(walker, syntax, resolver);
  pop_scope(resolver);
}

static void visit_proc_decl(const MpplAstWalker *walker, const MpplProcDecl *syntax, void *resolver)
{
  MpplToken *name_syntax = mppl_proc_decl__name(syntax);
  try_define(resolver, DEF_PROC, (const SyntaxTree *) syntax, (SyntaxTree *) name_syntax);
  mppl_unref(name_syntax);
  push_scope(resolver);
  mppl_ast__walk_proc_decl(walker, syntax, resolver);
  pop_scope(resolver);
}

static void visit_var_decl(const MpplAstWalker *walker, const MpplVarDecl *syntax, void *resolver)
{
  unsigned long i;
  for (i = 0; i < mppl_var_decl__name_count(syntax); ++i) {
    MpplToken *name_syntax = mppl_var_decl__name(syntax, i);
    try_define(resolver, DEF_VAR, (const SyntaxTree *) syntax, (SyntaxTree *) name_syntax);
    mppl_unref(name_syntax);
  }
  (void) walker;
}

static void visit_fml_param_sec(const MpplAstWalker *walker, const MpplFmlParamSec *syntax, void *resolver)
{
  unsigned long i;
  for (i = 0; i < mppl_fml_param_sec__name_count(syntax); ++i) {
    SyntaxTree *name_syntax = (SyntaxTree *) mppl_fml_param_sec__name(syntax, i);
    try_define(resolver, DEF_PARAM, (const SyntaxTree *) syntax, name_syntax);
    mppl_unref(name_syntax);
  }
  (void) walker;
}

static void visit_entire_var(const MpplAstWalker *walker, const MpplEntireVar *syntax, void *resolver)
{
  MpplToken *name_syntax = mppl_entire_var__name(syntax);
  try_resolve(resolver, (const SyntaxTree *) name_syntax, 0);
  mppl_unref(name_syntax);
  (void) walker;
}

static void visit_indexed_var(const MpplAstWalker *walker, const MpplIndexedVar *syntax, void *resolver)
{
  MpplToken *name_syntax = mppl_indexed_var__name(syntax);
  try_resolve(resolver, (const SyntaxTree *) name_syntax, 0);
  mppl_unref(name_syntax);
  (void) walker;
}

static void visit_call_stmt(const MpplAstWalker *walker, const MpplCallStmt *syntax, void *resolver)
{
  MpplToken *name_syntax = mppl_call_stmt__name(syntax);
  const Def *proc        = try_resolve(resolver, (const SyntaxTree *) name_syntax, 1);
  if (proc) {
    const SyntaxTree *node;
    for (node = (const SyntaxTree *) syntax; node; node = syntax_tree_parent(node)) {
      if (syntax_tree_kind(node) == SYNTAX_PROC_DECL && syntax_tree_raw(node) == syntax_tree_raw(def_syntax(proc))) {
        error_call_stmt_recursion(resolver, proc, (const SyntaxTree *) name_syntax);
        break;
      }
    }
  }
  mppl_unref(name_syntax);
  mppl_ast__walk_call_stmt(walker, syntax, resolver);
}

int mpplc_resolve(const Source *source, const MpplProgram *syntax, Ctx *ctx)
{
  MpplAstWalker walker;
  Resolver      resolver;
  int           result = 1;
  resolver.scope       = NULL;
  resolver.ctx         = ctx;
  resolver.errors      = array_new(sizeof(Report *));

  mppl_ast_walker__setup(&walker);
  walker.visit_program       = &visit_program;
  walker.visit_proc_decl     = &visit_proc_decl;
  walker.visit_var_decl      = &visit_var_decl;
  walker.visit_fml_param_sec = &visit_fml_param_sec;
  walker.visit_entire_var    = &visit_entire_var;
  walker.visit_indexed_var   = &visit_indexed_var;
  walker.visit_call_stmt     = &visit_call_stmt;
  mppl_ast_walker__travel(&walker, syntax, &resolver);

  if (array_count(resolver.errors)) {
    unsigned long i;
    for (i = 0; i < array_count(resolver.errors); ++i) {
      report_emit(*(Report **) array_at(resolver.errors, i), source);
    }
    result = 0;
  }
  array_free(resolver.errors);
  return result;
}
