#ifndef PARSER_H
#define PARSER_H

#include "token.h"

int mppl_parse(const char *source, unsigned long size, TokenTree *tree);

#endif
