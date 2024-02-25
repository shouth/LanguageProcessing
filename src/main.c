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

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "array.h"
#include "compiler.h"
#include "context.h"
#include "mppl_syntax.h"
#include "source.h"

const char *program;
Array      *filenames = NULL;

int dump_syntax  = 0;
int pretty_print = 0;
int syntax_only  = 0;
int emit_llvm    = 0;
int emit_casl2   = 0;

static int run_compiler(void)
{
  unsigned long i;
  int           result = EXIT_SUCCESS;

  for (i = 0; i < array_count(filenames); ++i) {
    const char  *filename = *(const char **) array_at(filenames, i);
    Ctx         *ctx      = ctx_new();
    Source      *source   = source_new(filename, strlen(filename));
    MpplProgram *syntax   = NULL;

    if (!source) {
      fprintf(stderr, "Cannot open file: %s\n", filename);
      result = EXIT_FAILURE;
      continue;
    }

    if (mpplc_parse(source, ctx, &syntax)) {
      if (dump_syntax) {
        mpplc_dump_syntax(syntax);
      }

      if (pretty_print) {
        mpplc_pretty_print(syntax, NULL);
      }

      if (mpplc_resolve(source, syntax, ctx) && mpplc_check(source, syntax, ctx)) {
        if (!syntax_only) {
          if (emit_casl2) {
            mpplc_codegen_casl2(source, syntax, ctx);
          }

          if (emit_llvm) {
            mpplc_codegen_llvm_ir(source, syntax, ctx);
          }
        }
      }
    }

    mppl_unref(syntax);
    source_free(source);
    ctx_free(ctx);
  }
  return result;
}

static void print_help(void)
{
  printf(
    "Usage: %s [OPTIONS] INPUT\n"
    "Options:\n"
    "    --dump-syntax   Dump syntax tree\n"
    "    --pretty-print  Pretty print the input file\n"
    "    --syntax-only   Check syntax only\n"
    "    --emit-llvm     Emit LLVM IR\n"
    "    --emit-casl2    Emit CASL2\n"
    "    --help          Print this help message\n",
    program);
  fflush(stdout);
}

static void deinit(void)
{
  array_free(filenames);
  filenames = NULL;
}

static void init(int argc, const char **argv)
{
  int i;
  int stop   = 0;
  int status = EXIT_SUCCESS;

  program   = argv[0];
  filenames = array_new(sizeof(const char *));

  if (argc < 2) {
    print_help();
    stop   = 1;
    status = EXIT_FAILURE;
  } else {
    for (i = 1; i < argc; ++i) {
      if (strcmp(argv[i], "--dump-syntax") == 0) {
        dump_syntax = 1;
      } else if (strcmp(argv[i], "--pretty-print") == 0) {
        pretty_print = 1;
      } else if (strcmp(argv[i], "--syntax-only") == 0) {
        syntax_only = 1;
      } else if (strcmp(argv[i], "--emit-llvm") == 0) {
        emit_llvm = 1;
      } else if (strcmp(argv[i], "--emit-casl2") == 0) {
        emit_casl2 = 1;
      } else if (strcmp(argv[i], "--help") == 0) {
        print_help();
        stop   = 1;
        status = EXIT_SUCCESS;
      } else if (strcmp(argv[i], "--") == 0) {
        for (++i; i < argc; ++i) {
          array_push(filenames, &argv[i]);
        }
      } else if (argv[i][0] == '-') {
        fprintf(stderr, "Unknown option: %s\n", argv[i]);
        print_help();
        stop   = 1;
        status = EXIT_FAILURE;
      } else {
        array_push(filenames, &argv[i]);
      }
    }
  }

  if (!emit_llvm && !emit_casl2) {
    emit_casl2 = 1;
  }

  if (stop) {
    deinit();
    exit(status);
  }
}

int main(int argc, const char **argv)
{
  int status;

  char *mode = getenv("MPPLC_MODE");
  if (mode) {
    if (strcmp(mode, "TASK1") == 0) {
      return mpplc_task1(argc, argv);
    } else if (strcmp(mode, "TASK2") == 0) {
      return mpplc_task2(argc, argv);
    } else if (strcmp(mode, "TASK3") == 0) {
      return mpplc_task3(argc, argv);
    } else if (strcmp(mode, "TASK4") == 0) {
      return mpplc_task4(argc, argv);
    }
  }

  init(argc, argv);
  status = run_compiler();
  deinit();
  return status;
}
