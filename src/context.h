#ifndef CONTEXT_H
#define CONTEXT_H

#include "ast.h"
#include "source.h"
#include "types.h"
#include "utility.h"

typedef struct context__s context_t;

struct context__s {
  char *in_name;
  char *out_name;

  source_t *src;
  ast_t    *ast;
  def_t    *defs;
  hash_t   *resolution;
  hash_t   *types;

  hash_t  *symbol_interner;
  hash_t  *type_interner;
  subst_t *subst_loan;

  const type_t *type_integer;
  const type_t *type_boolean;
  const type_t *type_char;
  const type_t *type_string;
  const type_t *type_program;
};

const symbol_t *ctx_mk_symbol(context_t *, const char *, long);
subst_t        *ctx_loan_subst(context_t *, const type_t *);
const type_t   *ctx_mk_type_integer(context_t *);
const type_t   *ctx_mk_type_boolean(context_t *);
const type_t   *ctx_mk_type_char(context_t *);
const type_t   *ctx_mk_type_string(context_t *);
const type_t   *ctx_mk_type_array(context_t *, subst_t *, long);
const type_t   *ctx_mk_type_program(context_t *);
const type_t   *ctx_mk_type_procedure(context_t *, subst_t *);

#endif
