#ifndef PARSER_H
#define PARSER_H

#include "token.h"
#include "vector.h"

const TokenTree *parser_parse(TokenContext *context, const char *source, unsigned long size, Vector *errors);

#endif
