#ifndef CONTEXT_H
#define CONTEXT_H

#include "ast.h"
#include "ir.h"
#include "source.h"
#include "utility.h"

typedef struct ref__s    ref_t;
typedef struct symbol__s symbol_t;
typedef struct def__s    def_t;
typedef struct type__s   type_t;

/**********     ref     **********/

struct ref_s {
  const void *ref;
  ref_t      *next;
};

ref_t *make_ref(const void *);

/**********     symbol     **********/

struct symbol__s {
  const char *ptr;
  long        len;
};

/**********     definition     **********/

typedef enum {
  DEF_PROGRAM,
  DEF_PROCEDURE,
  DEF_VAR,
  DEF_ARG_VAR,
  DEF_LOCAL_VAR
} def_kind_t;

struct def__s {
  union {
    const ast_program_t             *program;
    const ast_decl_part_procedure_t *procedure;
    const ast_decl_part_variable_t  *var;
    const ast_decl_param_t          *arg;
    const ast_decl_part_variable_t  *local_var;
  } def;

  def_kind_t      kind;
  const type_t   *type;
  const symbol_t *name;
  region_t        pos;
  def_t          *next;
};

/**********     type     **********/

typedef enum {
  TYPE_INTEGER,
  TYPE_BOOLEAN,
  TYPE_CHAR,
  TYPE_STRING,
  TYPE_ARRAY,
  TYPE_PROGRAM,
  TYPE_PROCEDURE
} type_kind_t;

typedef struct type__procedure_s type_procedure_t;
typedef struct type__array_s     type_array_t;

struct type__procedure_s {
  ref_t *params;
};

struct type__array_s {
  ref_t        *base;
  unsigned long size;
};

struct type__s {
  union {
    type_procedure_t procedure;
    type_array_t     array;
  } type;

  type_kind_t kind;
};

/**********     context     **********/

typedef struct context__s context_t;

struct context__s {
  def_t *defs;

  hash_t *symbol_interner;
  hash_t *type_interner;

  const type_t *type_integer;
  const type_t *type_boolean;
  const type_t *type_char;
  const type_t *type_string;
  const type_t *type_program;
};

const symbol_t *make_symbol(context_t *, const char *, long);

const type_t *make_type_integer(context_t *);
const type_t *make_type_boolean(context_t *);
const type_t *make_type_char(context_t *);
const type_t *make_type_string(context_t *);
const type_t *make_type_array(context_t *, ref_t *, unsigned long);
const type_t *make_type_program(context_t *);
const type_t *make_type_procedure(context_t *, ref_t *);

#endif
