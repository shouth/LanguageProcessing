#ifndef MPPL_H
#define MPPL_H

#include "ast.h"
#include "ir.h"

/* parser.c */

ast_t *parse_source(const source_t *src);

/* pretty_print.c */

void pretty_print(const ast_t *ast);

/* analyzer.c */

ir_t *lower_ast(ast_t *ast);

/* crossref.c */

void print_crossref(const ir_t *ir);

/* codegen.c */

void codegen_casl2(const ir_t *ir);

#endif
