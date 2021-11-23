#include <stdlib.h>

#include "cursol.h"
#include "lexer.h"
#include "source.h"
#include "token.h"
#include "terminal.h"
#include "parse_tree.h"
#include "parser.h"

int main(int argc, char **argv)
{
    source_t *src;
    cursol_t cursol;
    token_t token;
    terminal_t terminal;
    parse_tree_t *tree;

    if (argc < 2) {
        return -1;
    }

    src = source_new(argv[1]);
    cursol_init(&cursol, src, src->src_ptr, src->src_size);
    tree = parse(src);

    if (tree == NULL) {
        printf("Unhandled Error!");
        exit(1);
    }

    /* pretty print will called */
    return 0;
}
