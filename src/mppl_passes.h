/* SPDX-License-Identifier: Apache-2.0 */

#ifndef MPPL_PASSES_H
#define MPPL_PASSES_H

#include "mppl_semantic.h"
#include "mppl_syntax.h"
#include "mppl_ty_ctxt.h"
#include "report.h"
#include "util.h"

typedef struct MpplLexResult MpplLexResult;

struct MpplLexResult {
  MpplSyntaxKind kind;
  unsigned long  span;
  int            is_unterminated; /* For STRING_LIT, BRACES_COMMENT and C_COMMENT */
  int            has_nongraphic; /* For STRING_LIT */
};

MpplLexResult mppl_lex(const char *text, unsigned long length);

typedef struct MpplParseResult MpplParseResult;

struct MpplParseResult {
  MpplRootSyntax *root;
  Slice(Report *) diags;
};

MpplParseResult mppl_parse(const char *text, unsigned long length);

typedef struct MpplResolveResult MpplResolveResult;

struct MpplResolveResult {
  MpplSemantics semantics;
  Slice(Report *) diags;
};

MpplResolveResult mppl_resolve(const MpplRootSyntax *syntax);

typedef struct MpplCheckResult MpplCheckResult;

struct MpplCheckResult {
  MpplTyCtxt *ctxt;
  Slice(Report *) diags;
};

MpplCheckResult mppl_check(const MpplRootSyntax *syntax, const MpplSemantics *semantics);

#endif /* MPPL_PASSES_H */
