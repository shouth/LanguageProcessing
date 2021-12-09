#include <stdlib.h>
#include <stdio.h>

#include "mppl.h"

int main(int argc, char **argv)
{
    source_t *src;
    ast_t *ast;

    if (argc < 2) {
        return -1;
    }

    src = new_source(argv[1]);
    ast = parse_source(src);
    if (ast) {
        pretty_print(ast);
    }
    delete_ast(ast);
    delete_source(src);
    return 0;
}
