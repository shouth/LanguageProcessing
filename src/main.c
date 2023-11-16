#include <stddef.h>
#include <stdlib.h>

#include "module.h"
#include "pretty_printer.h"
#include "tasks.h"
#include "token.h"

int main(int argc, const char **argv)
{
  Module    module;
  TokenTree tree;
  module_init(&module, argv[1]);
  if (module_token_tree(&module, &tree)) {
    mppl_pretty_print((TokenNode *) &tree, NULL);
  }
  token_tree_deinit(&tree);
  module_deinit(&module);
  return EXIT_SUCCESS;
}
