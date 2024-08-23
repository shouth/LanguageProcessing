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
#include "source.h"
#include "syntax_tree.h"

typedef struct LexedToken LexedToken;

typedef enum {
  LEX_OK,
  LEX_EOF,
  LEX_ERROR_STRAY_CHAR,
  LEX_ERROR_NONGRAPHIC_CHAR,
  LEX_ERROR_UNTERMINATED_STRING,
  LEX_ERROR_UNTERMINATED_COMMENT,
  LEX_ERROR_TOO_BIG_NUMBER
} LexStatus;

struct LexedToken {
  MpplSyntaxKind kind;
  unsigned long  offset;
  unsigned long  length;
};

LexStatus mpplc_lex(const Source *source, unsigned long offset, LexedToken *token);

typedef struct MpplParseResult MpplParseResult;

struct MpplParseResult {
  RawSyntaxRoot *root;
  Diag          *diags;
  unsigned long  diag_count;
};

void mpplc_parse(const Source *source, MpplParseResult *result);

#endif
