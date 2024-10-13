/* SPDX-License-Identifier: Apache-2.0 */

#ifndef MPPL_TYPE_CONTEXT_H
#define MPPL_TYPE_CONTEXT_H

#include "syntax_tree.h"
#include "util.h"

typedef enum {
  MPPL_TY_INTEGER,
  MPPL_TY_BOOLEAN,
  MPPL_TY_CHAR,
  MPPL_TY_STRING,
  MPPL_TY_ARRAY,
  MPPL_TY_PROC
} MpplTyKind;

typedef struct MpplTy      MpplTy;
typedef struct MpplArrayTy MpplArrayTy;
typedef struct MpplProcTy  MpplProcTy;
typedef struct MpplTyCtxt  MpplTyCtxt;

struct MpplTy {
  MpplTyKind kind;
};

struct MpplArrayTy {
  MpplTy        type;
  const MpplTy *base;
  unsigned long size;
};

struct MpplProcTy {
  MpplTy type;
  Slice(const MpplTy *) params;
};

const MpplTy *mppl_ty_integer(void);
const MpplTy *mppl_ty_boolean(void);
const MpplTy *mppl_ty_char(void);
const MpplTy *mppl_ty_string(void);
const MpplTy *mppl_ty_array(MpplTyCtxt *ctxt, const MpplTy *base, unsigned long size);
const MpplTy *mppl_ty_proc(MpplTyCtxt *ctxt, const MpplTy **params, unsigned long param_count);

MpplTyCtxt   *mppl_ty_ctxt_alloc(void);
void          mppl_ty_ctxt_free(MpplTyCtxt *ctxt);
void          mppl_ty_ctxt_set(MpplTyCtxt *ctxt, const RawSyntaxNode *node, const MpplTy *ty);
const MpplTy *mppl_ty_ctxt_get(MpplTyCtxt *ctxt, const RawSyntaxNode *node);

#endif
