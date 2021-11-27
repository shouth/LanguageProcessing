#ifndef PARSER_H
#define PARSER_H

#include "source.h"
#include "ast.h"

ast_t *parse(const source_t *src);

#endif /* PARSER_H */
