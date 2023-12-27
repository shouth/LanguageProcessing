#ifndef PARSER_H
#define PARSER_H

#include "source.h"
#include "token_tree.h"

int mppl_parse(const Source *source, TokenTree *tree);

#endif
