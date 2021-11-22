#ifndef PARSER_H
#define PARSER_H

#include "parse_tree.h"
#include "source.h"

parse_tree_t *parse(const source_t *src);

#endif /* PARSER_H */
