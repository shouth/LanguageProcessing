#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "checker.h"
#include "inference.h"
#include "parser.h"
#include "pretty_printer.h"
#include "resolution.h"
#include "resolver.h"
#include "source.h"
#include "token_tree.h"

int main(int argc, const char **argv)
{
  Source    *source;
  TokenTree *tree;

  if (argc < 2) {
    fprintf(stderr, "[Usage] %s <filename>\n", argv[0]);
    return EXIT_FAILURE;
  }

  source = source_new(argv[1], strlen(argv[1]));
  if (mppl_parse(source, &tree)) {
    Res *res;
    if (mppl_resolve(source, (const TokenNode *) tree, &res)) {
      Infer *infer;
      if (mppl_check(source, (const TokenNode *) tree, res, &infer)) {
        mppl_pretty_print((const TokenNode *) tree, NULL);
        infer_free(infer);
      }
      res_free(res);
    }
  }
  token_tree_free(tree);
  source_free(source);
  return EXIT_SUCCESS;
}
