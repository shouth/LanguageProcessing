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

int pretty_print = 0;
int syntax_only  = 0;

int run_compiler(void)
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
      if (pretty_print) {
        mpplc_pretty_print(syntax, NULL);
      }

      if (mpplc_resolve(source, syntax, ctx) && mpplc_check(source, syntax, ctx)) {
        if (!syntax_only) {
          mpplc_codegen_casl2(source, syntax, ctx);
        }
      }
    }

    mppl_unref(syntax);
    source_free(source);
    ctx_free(ctx);
  }
  return result;
}

void print_help(void)
{
  printf("Usage: %s [OPTIONS] INPUT\n", program);
  printf("Options:\n");
  printf("    --pretty-print  Pretty print the input file\n");
  printf("    --syntax-only   Check syntax only\n");
  printf("    --help          Print this help message\n");
  fflush(stdout);
}

void deinit(void)
{
  array_free(filenames);
  filenames = NULL;
}

void init(int argc, const char **argv)
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
      if (strcmp(argv[i], "--pretty-print") == 0) {
        pretty_print = 1;
      } else if (strcmp(argv[i], "--syntax-only") == 0) {
        syntax_only = 1;
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
