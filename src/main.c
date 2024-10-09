/* SPDX-License-Identifier: Apache-2.0 */

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mppl_passes.h"
#include "mppl_semantic.h"
#include "mppl_syntax.h"
#include "report.h"
#include "source.h"
#include "syntax_tree.h"
#include "util.h"

const char *program;

Vec(const char *) filenames;

int dump_syntax   = 0;
int dump_crossref = 0;
int pretty_print  = 0;
int syntax_only   = 0;
int emit_llvm     = 0;
int emit_casl2    = 0;

void mppl_syntax_kind_print(RawSyntaxKind kind, FILE *file)
{
  fprintf(file, "%s", mppl_syntax_kind_to_string(kind));
}

static int run_compiler(void)
{
  unsigned long i, j;
  int           result = EXIT_SUCCESS;

  for (i = 0; i < filenames.count; ++i) {
    const char *filename = filenames.ptr[i];
    Source     *source   = source_new(filename, strlen(filename));

    if (!source) {
      fprintf(stderr, "Cannot open file: %s\n", filename);
      result = EXIT_FAILURE;
    } else {
      MpplParseResult parse_result = mppl_parse(source->text.ptr, source->text.count);
      if (dump_syntax) {
        syntax_tree_print(&parse_result.root->syntax, stdout, &mppl_syntax_kind_print);
      }

      if (parse_result.diags.count > 0) {
        for (j = 0; j < parse_result.diags.count; ++j) {
          report_emit(parse_result.diags.ptr[j], source);
        }
      } else {
        MpplResolveResult resolve_result = mppl_resolve(parse_result.root);
        if (dump_crossref) {
          mppl_semantics_print(&resolve_result.semantics, source);
        }

        for (j = 0; j < resolve_result.diags.count; ++j) {
          report_emit(resolve_result.diags.ptr[j], source);
        }

        mppl_semantics_free(&resolve_result.semantics);
        slice_free(&resolve_result.diags);
      }

      mppl_root_syntax_free(parse_result.root);
      slice_free(&parse_result.diags);
      source_free(source);
    }
  }
  return result;
}

static void print_help(void)
{
  printf(
    "Usage: %s [OPTIONS] INPUT\n"
    "Options:\n"
    "    --dump-syntax    Dump syntax tree\n"
    "    --dump-crossref  Dump cross reference\n"
    "    --pretty-print   Pretty print the input file\n"
    "    --syntax-only    Check syntax only\n"
    "    --emit-llvm      Emit LLVM IR\n"
    "    --emit-casl2     Emit CASL2\n"
    "    --help           Print this help message\n",
    program);
  fflush(stdout);
}

static void deinit(void)
{
  vec_free(&filenames);
}

static void init(int argc, const char **argv)
{
  int i;
  int stop   = 0;
  int status = EXIT_SUCCESS;

  program = argv[0];

  vec_alloc(&filenames, 0);

  if (argc < 2) {
    print_help();
    stop   = 1;
    status = EXIT_FAILURE;
  } else {
    for (i = 1; i < argc; ++i) {
      if (strcmp(argv[i], "--dump-syntax") == 0) {
        dump_syntax = 1;
      } else if (strcmp(argv[i], "--dump-crossref") == 0) {
        dump_crossref = 1;
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
          vec_push(&filenames, &argv[i], 1);
        }
      } else if (argv[i][0] == '-') {
        fprintf(stderr, "Unknown option: %s\n", argv[i]);
        print_help();
        stop   = 1;
        status = EXIT_FAILURE;
      } else {
        vec_push(&filenames, &argv[i], 1);
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
    /* TODO: add task */
  }

  init(argc, argv);
  status = run_compiler();
  deinit();
  return status;
}
