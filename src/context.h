#ifndef CONTEXT_H
#define CONTEXT_H

#include "source.h"
#include "utility.h"

typedef struct symbol__s symbol_t;
typedef struct type__s   type_t;

/**********     symbol     **********/

struct symbol__s {
  const char *ptr;
  long        len;
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

typedef struct substs__s         substs_t;
typedef struct type__procedure_s type_procedure_t;
typedef struct type__array_s     type_array_t;

struct substs__s {
  const type_t **types;
  long           count;
};

struct type__procedure_s {
  const substs_t *params;
};

struct type__array_s {
  const substs_t *base;
  long            size;
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
  hash_t *symbol_interner;
  hash_t *substs_interner;
  hash_t *type_interner;

  const type_t *type_integer;
  const type_t *type_boolean;
  const type_t *type_char;
  const type_t *type_string;
  const type_t *type_program;
};

const symbol_t *symbol(context_t *, const char *, long);
const substs_t *substs(context_t *, const type_t **, long);
const type_t   *type_integer(context_t *);
const type_t   *type_boolean(context_t *);
const type_t   *type_char(context_t *);
const type_t   *type_string(context_t *);
const type_t   *type_array(context_t *, const substs_t *, long);
const type_t   *type_program(context_t *);
const type_t   *type_procedure(context_t *, const substs_t *);
context_t      *ctx_init(context_t *);
void            ctx_deinit(context_t *);

#endif
