/* SPDX-License-Identifier: Apache-2.0 */

#ifndef MPPL_PASSES_H
#define MPPL_PASSES_H

#include "mppl_syntax.h"
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

#endif /* MPPL_PASSES_H */
