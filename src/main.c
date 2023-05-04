#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ast.h"
#include "context.h"
#include "mppl.h"
#include "source.h"
#include "utility.h"

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
  long        i;
  const char *output            = NULL;
  int         flag_pretty_print = 0;
  int         flag_crossref     = 0;
  int         flag_color_print  = -1;

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

  term_ansi_stdout(flag_color_print);
  term_ansi_stderr(flag_color_print);

  {
    context_t *ctx = ctx_new(argv[i], output);
    if (parse(ctx)) {
      if (flag_pretty_print) {
        ast_pretty(ctx);
      }

      resolve(ctx);
    }
    ctx_delete(ctx);
  }
  return EXIT_SUCCESS;
}
