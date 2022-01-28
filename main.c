#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "mppl.h"

void print_help(const char *command)
{
    printf(
        "Usage: %s [OPTIONS] FILE\n"
        "\n"
        "Options\n"
        "    -o FILENAME   output assembly to FILENAME\n"
        "    -p            pretty print\n"
        "    -r            print cross reference\n"
        "    -c            enable color printing\n"
        , command);
    exit(1);
}

int main(int argc, char **argv)
{
    source_t *src = NULL;
    ast_t *ast = NULL;
    ir_t *ir = NULL;
    size_t i;

    const char *output = NULL;
    int flag_pretty_print = 0;
    int flag_cross_ref = 0;
    int flag_color_print = 0;

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
            flag_cross_ref = 1;
            break;
        case 'c':
            flag_color_print = 1;
            break;
        default:
            print_help(argv[0]);
        }
    }

    console_ansi(flag_color_print);

    src = new_source(argv[i], output);
    if (ast = parse_source(src)) {
        if (flag_pretty_print) {
            pretty_print(ast);
        }

        if (ir = analyze_ast(ast)) {
            if (flag_cross_ref) {
                print_cross_ref(ir);
            }
            codegen_casl2(ir);
        }
    }

    delete_ir(ir);
    delete_ast(ast);
    delete_source(src);
    return 0;
}
