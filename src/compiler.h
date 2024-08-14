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

#include "context_fwd.h"
#include "mppl_syntax.h"
#include "mppl_syntax_kind.h"
#include "source.h"

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

int mpplc_parse(const Source *source, MpplProgram **syntax);

int mpplc_resolve(const Source *source, const MpplProgram *syntax, Ctx *ctx);

int mpplc_check(const Source *source, const MpplProgram *syntax, Ctx *ctx);

int mpplc_codegen_casl2(const Source *source, const MpplProgram *syntax, Ctx *ctx);

int mpplc_codegen_llvm_ir(const Source *source, const MpplProgram *syntax, Ctx *ctx);

void mpplc_dump_syntax(const MpplProgram *syntax);

typedef struct PrinterOption PrinterOption;

struct PrinterOption {
  struct {
    int           enabled;
    unsigned long foreground;
    unsigned long program;
    unsigned long keyword;
    unsigned long operator;
    unsigned long procedure;
    unsigned long parameter;
    unsigned long string;
    unsigned long literal;
  } color;
};

void mpplc_pretty_print(const MpplProgram *syntax, const PrinterOption *option);

int mpplc_task1(int argc, const char **argv);
int mpplc_task2(int argc, const char **argv);
int mpplc_task3(int argc, const char **argv);
int mpplc_task4(int argc, const char **argv);

#endif
