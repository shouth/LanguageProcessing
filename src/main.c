#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parser.h"
#include "pretty_printer.h"
#include "source.h"
#include "token_tree.h"

int main(int argc, const char **argv)
{
  Source    source;
  TokenTree tree;

  if (argc < 2) {
    fprintf(stderr, "[Usage] %s <filename>\n", argv[0]);
    return EXIT_FAILURE;
  }

  source_init(&source, argv[1], strlen(argv[1]));
  if (mppl_parse(&source, &tree)) {
    mppl_pretty_print((const TokenNode *) &tree, NULL);
  }
  token_tree_deinit(&tree);
  source_deinit(&source);
  return EXIT_SUCCESS;
}
