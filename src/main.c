#include <stdlib.h>

#include "module.h"
#include "tasks.h"
#include "token.h"

int main(int argc, const char **argv)
{
  Module    module;
  TokenTree tree;
  module_init(&module, argv[1]);
  module_token_tree(&module, &tree);
  token_node_print((TokenNode *) &tree);
  token_tree_deinit(&tree);
  module_deinit(&module);
  return EXIT_SUCCESS;
}
