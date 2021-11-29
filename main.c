#include <stdlib.h>
#include <stdio.h>

#include "source.h"
#include "parser.h"
#include "pretty_printer.h"

int main(int argc, char **argv)
{
    source_t *src;
    ast_t *ast;

    if (argc < 2) {
        return -1;
    }

    src = source_new(argv[1]);
    ast = parse(src);
    if (ast) {
        pretty_print(ast);
    }
    delete_ast(ast);
    source_free(src);
    return 0;
}
