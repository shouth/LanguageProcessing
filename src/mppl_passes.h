/*
   Copyright 2022 Shota Minami

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

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
  RawSyntaxRoot *root;
  Diag         **diags;
  unsigned long  diag_count;
};

MpplParseResult mppl_parse(const char *text, unsigned long length);

#endif
