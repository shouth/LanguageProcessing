#include <stdlib.h>
#include <stdio.h>

#include "source.h"
#include "parse_tree.h"
#include "parser.h"
#include "pretty_printer.h"

int main(int argc, char **argv)
{
    source_t *src;
    parse_tree_t *tree;

    if (argc < 2) {
        return -1;
    }

    src = source_new(argv[1]);
    tree = parse(src);

    if (tree == NULL) {
        printf("Unhandled Error!");
        exit(1);
    }

    pretty_print(tree);
    return 0;
}
