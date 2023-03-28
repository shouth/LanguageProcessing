#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mppl.h"

void print_help(const char *command)
{
  printf(
    "Usage: %s [OPTIONS] FILE\n"
    "\n"
    "Options\n"
    "    -o FILENAME   output assembly to FILENAME\n"
    "    -p            pretty print\n"
    "    -r            print cross reference\n"
    "    -c            enable color printing\n",
    command);
  exit(EXIT_FAILURE);
}

int main(int argc, char **argv)
{
  source_t *src = NULL;
  ast_t    *ast = NULL;
  ir_t     *ir  = NULL;
  long      i;

  const char *output            = NULL;
  int         flag_pretty_print = 0;
  int         flag_crossref     = 0;
  int         flag_color_print  = 0;

  if (argc < 2) {
    print_help(argv[0]);
  }

  for (i = 1; i < argc - 1; i++) {
    if (argv[i][0] != '-' || strlen(argv[i]) != 2) {
      print_help(argv[0]);
    }

    switch (argv[i][1]) {
    case 'o':
      output = argv[++i];
      break;
    case 'p':
      flag_pretty_print = 1;
      break;
    case 'r':
      flag_crossref = 1;
      break;
    case 'c':
      flag_color_print = 1;
      break;
    default:
      print_help(argv[0]);
    }
  }

  console_ansi(flag_color_print);

  src = src_new(argv[i], output);
  if (ast = parse_source(src)) {
    if (flag_pretty_print) {
      pretty_print(ast);
    }

    if (ir = analyze_ast(ast)) {
      if (flag_crossref) {
        print_crossref(ir);
      }
      codegen_casl2(ir);
    }
  }

  delete_ir(ir);
  delete_ast(ast);
  src_delete(src);
  return 0;
}
