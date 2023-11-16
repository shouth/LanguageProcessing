#ifndef LEXER_H
#define LEXER_H

#include "report.h"
#include "token.h"

int mppl_lex(const char *source, unsigned long offset, unsigned long length, TokenInfo *info, Report *report);

#endif
