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
        ir_t *ir;
        ir_item_t *item;

        /* pretty_print(ast); */
        ir = analyze_ast(ast);
        print_cross_ref(ir);
    }
    delete_ast(ast);
    delete_source(src);
    return 0;
}
