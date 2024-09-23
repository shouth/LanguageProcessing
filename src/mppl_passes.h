/* SPDX-License-Identifier: Apache-2.0 */

#ifndef COMPILER_H
#define COMPILER_H

#include "diagnostics.h"
#include "mppl_syntax.h"
#include "syntax_tree.h"

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
  SyntaxTree   *root;
  Diag        **diags;
  unsigned long diag_count;
};

MpplParseResult mppl_parse(const char *text, unsigned long length);

#endif
