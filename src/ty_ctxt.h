/* SPDX-License-Identifier: Apache-2.0 */

#ifndef TY_CTXT_H
#define TY_CTXT_H

#include "syntax_tree.h"
#include "util.h"

typedef enum {
  TY_ERROR,
  TY_INTEGER,
  TY_BOOLEAN,
  TY_CHAR,
  TY_STRING,
  TY_ARRAY,
  TY_PROC
} TyKind;

typedef struct Ty      Ty;
typedef struct ArrayTy ArrayTy;
typedef struct ProcTy  ProcTy;
typedef struct TyCtxt  TyCtxt;

struct Ty {
  TyKind kind;
};

struct ArrayTy {
  Ty            type;
  const Ty     *base;
  unsigned long size;
};

struct ProcTy {
  Ty type;
  Slice(const Ty *) params;
};

const Ty *ty_error(void);
const Ty *ty_integer(void);
const Ty *ty_boolean(void);
const Ty *ty_char(void);
const Ty *ty_string(void);
const Ty *ty_array(TyCtxt *ctxt, const Ty *base, unsigned long size);
const Ty *ty_proc(TyCtxt *ctxt, const Ty **params, unsigned long param_count);

TyCtxt   *ty_ctxt_alloc(void);
void      ty_ctxt_free(TyCtxt *ctxt);
void      ty_ctxt_set(TyCtxt *ctxt, const RawSyntaxNode *node, const Ty *ty);
const Ty *ty_ctxt_get(TyCtxt *ctxt, const RawSyntaxNode *node);

#endif
