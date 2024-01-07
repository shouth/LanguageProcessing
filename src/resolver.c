#include <string.h>

#include "array.h"
#include "map.h"
#include "mppl_syntax.h"
#include "report.h"
#include "resolution.h"
#include "resolver.h"
#include "source.h"
#include "syntax_kind.h"
#include "syntax_tree.h"
#include "token_tree.h"
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
  report_annotation(report, previous->offset, previous->offset + previous->length,
    "previous definition of `%s`", previous->name);
  report_annotation(report, conflict->offset, conflict->offset + conflict->length, "redefinition of `%s`", previous->name);
  array_push(resolver->errors, &report);
}

static void error_var_res_failure(Resolver *resolver, const Binding *missing)
{
  Report *report = report_new(REPORT_KIND_ERROR, missing->offset, "failed to resolve `%s`", missing->name);
  report_annotation(report, missing->offset, missing->offset + missing->length,
    "use of undeclared variable or parameter `%s`", missing->name);
  array_push(resolver->errors, &report);
}

static void error_proc_res_failure(Resolver *resolver, const Binding *missing)
{
  Report *report = report_new(REPORT_KIND_ERROR, missing->offset, "failed to resolve `%s`", missing->name);
  report_annotation(report, missing->offset, missing->offset + missing->length,
    "use of undeclared procedure `%s`", missing->name);
  array_push(resolver->errors, &report);
}

static unsigned long def_key_hasher(const void *key)
{
  return fnv1a(FNV1A_INIT, key, strlen(key));
}

static int def_key_compare(const void *left, const void *right)
{
  return !strcmp(left, right);
}

static void push_scope(Resolver *resolver)
{
  Scope *scope    = xmalloc(sizeof(Scope));
  scope->defs     = map_new(&def_key_hasher, &def_key_compare);
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

static void try_create_def(Resolver *resolver, DefKind kind, Binding *binding, const TokenNode *node)
{
  MapIndex index;
  if (resolver->scope && map_find(resolver->scope->defs, (void *) binding->name, &index)) {
    const Def *previous = map_value(resolver->scope->defs, &index);
    error_def_conflict(resolver, &previous->binding, binding);
  } else {
    const Def *def = res_create_def(resolver->res, kind, binding, node);
    if (resolver->scope) {
      map_update(resolver->scope->defs, &index, (void *) binding->name, (void *) def);
    }
  }
}

static const Def *get_def(Resolver *resolver, const char *name)
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

static void enter_program(const MpplProgram *program, Resolver *resolver)
{
  MpplToken   *name_syntax = mppl_program__name(program);
  const Token *name_token  = (const Token *) syntax_tree_raw((SyntaxTree *) name_syntax);
  Binding      binding;
  binding.name   = name_token->text;
  binding.offset = syntax_tree_offset((const SyntaxTree *) name_syntax);
  binding.length = name_token->text_length;
  try_create_def(resolver, DEF_PROGRAM, &binding, syntax_tree_raw((const SyntaxTree *) program));
  mppl_free(name_syntax);
}

static void enter_proc_decl(const MpplProcDecl *decl, Resolver *resolver)
{
  MpplToken   *name_syntax = mppl_proc_decl__name(decl);
  const Token *name_token  = (const Token *) syntax_tree_raw((SyntaxTree *) name_syntax);
  Binding      binding;
  binding.name   = name_token->text;
  binding.offset = syntax_tree_offset((const SyntaxTree *) name_syntax);
  binding.length = name_token->text_length;
  try_create_def(resolver, DEF_PROC, &binding, syntax_tree_raw((const SyntaxTree *) decl));
  mppl_free(name_syntax);
}

static void enter_var_decl(const MpplVarDecl *var_decl, Resolver *resolver)
{
  unsigned long i;
  for (i = 0; i < mppl_var_decl__name_count(var_decl); ++i) {
    MpplToken   *name_syntax = mppl_var_decl__name(var_decl, i);
    const Token *name_token  = (const Token *) syntax_tree_raw((SyntaxTree *) name_syntax);
    Binding      binding;
    binding.name   = name_token->text;
    binding.offset = syntax_tree_offset((const SyntaxTree *) name_syntax);
    binding.length = name_token->text_length;
    try_create_def(resolver, DEF_VAR, &binding, syntax_tree_raw((SyntaxTree *) var_decl));
    mppl_free(name_syntax);
  }
}

static void enter_fml_param_sec(const MpplFmlParamSec *sec, Resolver *resolver)
{
  unsigned long i;
  for (i = 0; i < mppl_fml_param_sec__name_count(sec); ++i) {
    SyntaxTree  *name_syntax = (SyntaxTree *) mppl_fml_param_sec__name(sec, i);
    const Token *name_token  = (const Token *) syntax_tree_raw(name_syntax);
    Binding      binding;
    binding.name   = name_token->text;
    binding.offset = syntax_tree_offset(name_syntax);
    binding.length = name_token->text_length;
    try_create_def(resolver, DEF_PARAM, &binding, syntax_tree_raw((SyntaxTree *) sec));
    mppl_free(name_syntax);
  }
}

static void enter_var_entire(const MpplEntireVar *var, Resolver *resolver)
{
  MpplToken   *name_syntax = mppl_entire_var__name(var);
  const Token *name_token  = (const Token *) syntax_tree_raw((SyntaxTree *) name_syntax);
  const Def   *def         = get_def(resolver, name_token->text);
  if (def) {
    res_record_ref(resolver->res, (const TokenNode *) name_token, def);
  } else {
    Binding binding;
    binding.name   = name_token->text;
    binding.offset = syntax_tree_offset((const SyntaxTree *) name_syntax);
    binding.length = name_token->text_length;
    error_var_res_failure(resolver, &binding);
  }
  mppl_free(name_syntax);
}

static void enter_var_indexed(const MpplIndexedVar *var, Resolver *resolver)
{
  MpplToken   *name_syntax = mppl_indexed_var__name(var);
  const Token *name_token  = (const Token *) syntax_tree_raw((SyntaxTree *) name_syntax);
  const Def   *def         = get_def(resolver, name_token->text);
  if (def) {
    res_record_ref(resolver->res, (const TokenNode *) name_token, def);
  } else {
    Binding binding;
    binding.name   = name_token->text;
    binding.offset = syntax_tree_offset((const SyntaxTree *) name_syntax);
    binding.length = name_token->text_length;
    error_var_res_failure(resolver, &binding);
  }
  mppl_free(name_syntax);
}

static void enter_call_stmt(const MpplCallStmt *stmt, Resolver *resolver)
{
  MpplToken   *name_syntax = mppl_call_stmt__name(stmt);
  const Token *name_token  = (const Token *) syntax_tree_raw((SyntaxTree *) name_syntax);
  const Def   *def         = get_def(resolver, name_token->text);
  if (def) {
    res_record_ref(resolver->res, (const TokenNode *) name_token, def);
  } else {
    Binding binding;
    binding.name   = name_token->text;
    binding.offset = syntax_tree_offset((const SyntaxTree *) name_syntax);
    binding.length = name_token->text_length;
    error_proc_res_failure(resolver, &binding);
  }
  mppl_free(name_syntax);
}

static int visit_syntax_tree(const SyntaxTree *tree, void *resolver, int enter)
{
  if (enter) {
    switch (syntax_tree_kind(tree)) {
    case SYNTAX_PROGRAM:
      enter_program((const MpplProgram *) tree, resolver);
      push_scope(resolver);
      return 1;

    case SYNTAX_PROC_DECL:
      enter_proc_decl((const MpplProcDecl *) tree, resolver);
      push_scope(resolver);
      return 1;

    case SYNTAX_VAR_DECL:
      enter_var_decl((const MpplVarDecl *) tree, resolver);
      return 0;

    case SYNTAX_FML_PARAM_SECTION:
      enter_fml_param_sec((const MpplFmlParamSec *) tree, resolver);
      return 0;

    case SYNTAX_ENTIRE_VAR:
      enter_var_entire((const MpplEntireVar *) tree, resolver);
      return 0;

    case SYNTAX_INDEXED_VAR:
      enter_var_indexed((const MpplIndexedVar *) tree, resolver);
      return 0;

    case SYNTAX_CALL_STMT:
      enter_call_stmt((const MpplCallStmt *) tree, resolver);
      return 0;

    default:
      return 1;
    }
  } else {
    switch (syntax_tree_kind(tree)) {
    case SYNTAX_PROGRAM:
      pop_scope(resolver);
      return 0;

    case SYNTAX_PROC_DECL:
      pop_scope(resolver);
      return 0;

    default:
      return 1;
    }
  }
}

int mppl_resolve(const Source *source, const TokenNode *tree, Res **resolution)
{
  SyntaxTree *syntax;

  Resolver resolver;
  resolver.scope  = NULL;
  resolver.res    = res_new();
  resolver.errors = array_new(sizeof(Report *));

  syntax = syntax_tree_root((const TokenTree *) tree);
  syntax_tree_visit(syntax, &visit_syntax_tree, &resolver);
  syntax_tree_free(syntax);

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
