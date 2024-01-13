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

static void try_create_def(Resolver *resolver, DefKind kind, const SyntaxTree *item_syntax, const SyntaxTree *name_syntax)
{
  MapIndex     index;
  const Token *name_token = (const Token *) syntax_tree_raw(name_syntax);
  Binding      binding;
  binding.name   = name_token->text;
  binding.offset = syntax_tree_offset(name_syntax);
  binding.length = name_token->text_length;
  if (resolver->scope && map_find(resolver->scope->defs, (void *) binding.name, &index)) {
    const Def *previous = map_value(resolver->scope->defs, &index);
    error_def_conflict(resolver, &previous->binding, &binding);
  } else {
    const TokenNode *raw_syntax = (const TokenNode *) syntax_tree_raw(item_syntax);
    unsigned long    offset     = syntax_tree_offset(item_syntax);
    const Def       *def        = res_create_def(resolver->res, kind, &binding, (const TokenNode *) name_token, raw_syntax, offset);
    if (resolver->scope) {
      map_update(resolver->scope->defs, &index, (void *) binding.name, (void *) def);
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

static const Def *try_record_ref(Resolver *resolver, const SyntaxTree *syntax, int is_proc)
{
  const Token *node = (const Token *) syntax_tree_raw(syntax);
  const Def   *def  = get_def(resolver, node->text);
  if (resolver->scope && def) {
    res_record_ref(resolver->res, (const TokenNode *) node, def);
    return def;
  } else {
    Binding binding;
    binding.name   = node->text;
    binding.offset = syntax_tree_offset(syntax);
    binding.length = node->text_length;
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

static int visit_syntax_tree(const SyntaxTree *tree, void *resolver, int enter)
{
  if (enter) {
    unsigned long i;
    switch (syntax_tree_kind(tree)) {
    case SYNTAX_PROGRAM: {
      const MpplProgram *program     = (const MpplProgram *) tree;
      MpplToken         *name_syntax = mppl_program__name(program);
      try_create_def(resolver, DEF_PROGRAM, (const SyntaxTree *) program, (SyntaxTree *) name_syntax);
      mppl_free(name_syntax);
      push_scope(resolver);
      return 1;
    }

    case SYNTAX_PROC_DECL: {
      const MpplProcDecl *decl        = (const MpplProcDecl *) tree;
      MpplToken          *name_syntax = mppl_proc_decl__name(decl);
      try_create_def(resolver, DEF_PROC, (const SyntaxTree *) decl, (SyntaxTree *) name_syntax);
      mppl_free(name_syntax);
      push_scope(resolver);
      return 1;
    }

    case SYNTAX_VAR_DECL: {
      const MpplVarDecl *var_decl = (const MpplVarDecl *) tree;
      for (i = 0; i < mppl_var_decl__name_count(var_decl); ++i) {
        MpplToken *name_syntax = mppl_var_decl__name(var_decl, i);
        try_create_def(resolver, DEF_VAR, (const SyntaxTree *) var_decl, (SyntaxTree *) name_syntax);
        mppl_free(name_syntax);
      }
      return 0;
    }

    case SYNTAX_FML_PARAM_SECTION: {
      const MpplFmlParamSec *sec = (const MpplFmlParamSec *) tree;
      for (i = 0; i < mppl_fml_param_sec__name_count(sec); ++i) {
        SyntaxTree *name_syntax = (SyntaxTree *) mppl_fml_param_sec__name(sec, i);
        try_create_def(resolver, DEF_PARAM, (const SyntaxTree *) sec, name_syntax);
        mppl_free(name_syntax);
      }
      return 0;
    }

    case SYNTAX_ENTIRE_VAR: {
      const MpplEntireVar *var         = (const MpplEntireVar *) tree;
      MpplToken           *name_syntax = mppl_entire_var__name(var);
      try_record_ref(resolver, (const SyntaxTree *) name_syntax, 0);
      mppl_free(name_syntax);
      return 0;
    }

    case SYNTAX_INDEXED_VAR: {
      const MpplIndexedVar *var         = (const MpplIndexedVar *) tree;
      MpplToken            *name_syntax = mppl_indexed_var__name(var);
      try_record_ref(resolver, (const SyntaxTree *) name_syntax, 0);
      mppl_free(name_syntax);
      return 0;
    }

    case SYNTAX_CALL_STMT: {
      const MpplCallStmt *stmt        = (const MpplCallStmt *) tree;
      MpplToken          *name_syntax = mppl_call_stmt__name(stmt);
      const Def          *proc        = try_record_ref(resolver, (const SyntaxTree *) name_syntax, 1);
      if (proc) {
        const SyntaxTree *node;
        for (node = tree; node; node = syntax_tree_parent(node)) {
          if (syntax_tree_kind(node) == SYNTAX_PROC_DECL && syntax_tree_raw(node) == proc->body) {
            error_call_stmt_recursion(resolver, proc, (const SyntaxTree *) name_syntax);
            break;
          }
        }
      }
      mppl_free(name_syntax);
      return 0;
    }

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

  syntax = syntax_tree_root(tree, token_node_trivia_length(tree));
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
