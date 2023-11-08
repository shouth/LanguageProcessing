#ifndef PARSER_H
#define PARSER_H

#include "token.h"

int parser_parse(const char *source, unsigned long size, TokenTree *tree);

#endif
