#include <stdio.h>
#include <string.h>

#include "array.h"
#include "map.h"
#include "mppl_syntax.h"
#include "mppl_syntax_ext.h"
#include "report.h"
#include "resolution.h"
#include "resolver.h"
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
  Res   *res;
  Array *errors;
};

static void error_def_conflict(Resolver *resolver, const Binding *previous, const Binding *conflict)
{
  Report *report = report_new(REPORT_KIND_ERROR, conflict->offset, "conflicting definition of `%s`", previous->name);
  report_annotation(report, previous->offset, previous->offset + string_length(previous->name),
    "previous definition of `%s`", previous->name);
  report_annotation(report, conflict->offset, conflict->offset + string_length(conflict->name), "redefinition of `%s`", previous->name);
  array_push(resolver->errors, &report);
}

static void error_var_res_failure(Resolver *resolver, const Binding *missing)
{
  Report *report = report_new(REPORT_KIND_ERROR, missing->offset, "failed to resolve `%s`", missing->name);
  report_annotation(report, missing->offset, missing->offset + string_length(missing->name),
    "use of undeclared variable or parameter `%s`", missing->name);
  array_push(resolver->errors, &report);
}

static void error_proc_res_failure(Resolver *resolver, const Binding *missing)
{
  Report *report = report_new(REPORT_KIND_ERROR, missing->offset, "failed to resolve `%s`", missing->name);
  report_annotation(report, missing->offset, missing->offset + string_length(missing->name),
    "use of undeclared procedure `%s`", missing->name);
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

static void try_create_def(Resolver *resolver, DefKind kind, const SyntaxTree *item_syntax, const SyntaxTree *name_syntax)
{
  MapIndex              index;
  const RawSyntaxToken *name_token = (const RawSyntaxToken *) syntax_tree_raw(name_syntax);
  Binding               binding;
  binding.name   = name_token->string;
  binding.offset = syntax_tree_offset(name_syntax);

  if (resolver->scope && map_find(resolver->scope->defs, (void *) binding.name, &index)) {
    const Def *previous = map_value(resolver->scope->defs, &index);
    error_def_conflict(resolver, &previous->binding, &binding);
  } else {
    unsigned long offset = syntax_tree_offset(item_syntax);
    const Def    *def    = res_create_def(resolver->res, kind, &binding, name_syntax, item_syntax, offset);
    if (resolver->scope) {
      map_update(resolver->scope->defs, &index, (void *) binding.name, (void *) def);
    }
  }
}

static const Def *get_def(Resolver *resolver, const String *name)
{
  Scope *scope;
  for (scope = resolver->scope; scope; scope = scope->parent) {
    MapIndex index;
    if (map_find(scope->defs, (void *) name, &index)) {
      return map_value(scope->defs, &index);
    }
  }
  return NULL;
}

static const Def *try_record_ref(Resolver *resolver, const SyntaxTree *syntax, int is_proc)
{
  const RawSyntaxToken *node = (const RawSyntaxToken *) syntax_tree_raw(syntax);
  const Def            *def  = get_def(resolver, node->string);
  if (resolver->scope && def) {
    res_record_ref(resolver->res, (const RawSyntaxNode *) node, def);
    return def;
  } else {
    Binding binding;
    binding.name   = node->string;
    binding.offset = syntax_tree_offset(syntax);
    if (is_proc) {
      error_proc_res_failure(resolver, &binding);
    } else {
      error_var_res_failure(resolver, &binding);
    }
    return NULL;
  }
}

static void error_call_stmt_recursion(Resolver *resolver, const Def *proc, const SyntaxTree *name_syntax)
{
  unsigned long offset = syntax_tree_offset(name_syntax);
  unsigned long length = syntax_tree_text_length(name_syntax);

  Report *report = report_new(REPORT_KIND_ERROR, offset, "recursion is prohibited");
  report_annotation(report, offset, offset + length, "recursive call to `%s`", proc->binding.name);
  array_push(resolver->errors, &report);
}

static void visit_program(const MpplAstWalker *walker, const MpplProgram *syntax, void *resolver)
{
  MpplToken *name_syntax = mppl_program__name(syntax);
  try_create_def(resolver, DEF_PROGRAM, (const SyntaxTree *) syntax, (SyntaxTree *) name_syntax);
  mppl_free(name_syntax);
  push_scope(resolver);
  mppl_ast__walk_program(walker, syntax, resolver);
  pop_scope(resolver);
}

static void visit_proc_decl(const MpplAstWalker *walker, const MpplProcDecl *syntax, void *resolver)
{
  MpplToken *name_syntax = mppl_proc_decl__name(syntax);
  try_create_def(resolver, DEF_PROC, (const SyntaxTree *) syntax, (SyntaxTree *) name_syntax);
  mppl_free(name_syntax);
  push_scope(resolver);
  mppl_ast__walk_proc_decl(walker, syntax, resolver);
  pop_scope(resolver);
}

static void visit_var_decl(const MpplAstWalker *walker, const MpplVarDecl *syntax, void *resolver)
{
  unsigned long i;
  for (i = 0; i < mppl_var_decl__name_count(syntax); ++i) {
    MpplToken *name_syntax = mppl_var_decl__name(syntax, i);
    try_create_def(resolver, DEF_VAR, (const SyntaxTree *) syntax, (SyntaxTree *) name_syntax);
    mppl_free(name_syntax);
  }
  (void) walker;
}

static void visit_fml_param_sec(const MpplAstWalker *walker, const MpplFmlParamSec *syntax, void *resolver)
{
  unsigned long i;
  for (i = 0; i < mppl_fml_param_sec__name_count(syntax); ++i) {
    SyntaxTree *name_syntax = (SyntaxTree *) mppl_fml_param_sec__name(syntax, i);
    try_create_def(resolver, DEF_PARAM, (const SyntaxTree *) syntax, name_syntax);
    mppl_free(name_syntax);
  }
  (void) walker;
}

static void visit_entire_var(const MpplAstWalker *walker, const MpplEntireVar *syntax, void *resolver)
{
  MpplToken *name_syntax = mppl_entire_var__name(syntax);
  try_record_ref(resolver, (const SyntaxTree *) name_syntax, 0);
  mppl_free(name_syntax);
  (void) walker;
}

static void visit_indexed_var(const MpplAstWalker *walker, const MpplIndexedVar *syntax, void *resolver)
{
  MpplToken *name_syntax = mppl_indexed_var__name(syntax);
  try_record_ref(resolver, (const SyntaxTree *) name_syntax, 0);
  mppl_free(name_syntax);
  (void) walker;
}

static void visit_call_stmt(const MpplAstWalker *walker, const MpplCallStmt *syntax, void *resolver)
{
  MpplToken *name_syntax = mppl_call_stmt__name(syntax);
  const Def *proc        = try_record_ref(resolver, (const SyntaxTree *) name_syntax, 1);
  if (proc) {
    const SyntaxTree *node;
    for (node = (const SyntaxTree *) syntax; node; node = syntax_tree_parent(node)) {
      if (syntax_tree_kind(node) == SYNTAX_PROC_DECL && node == proc->body) {
        error_call_stmt_recursion(resolver, proc, (const SyntaxTree *) name_syntax);
        break;
      }
    }
  }
  mppl_free(name_syntax);
  mppl_ast__walk_call_stmt(walker, syntax, resolver);
}

int mppl_resolve(const Source *source, const MpplProgram *syntax, Res **resolution)
{
  MpplAstWalker walker;
  Resolver      resolver;
  resolver.scope  = NULL;
  resolver.res    = res_new();
  resolver.errors = array_new(sizeof(Report *));

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
    *resolution = NULL;
    res_free(resolver.res);
  } else {
    *resolution = resolver.res;
  }
  array_free(resolver.errors);
  return !!*resolution;
}
