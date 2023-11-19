#ifndef LEXER_H
#define LEXER_H

#include "report.h"
#include "source.h"
#include "token.h"

int mppl_lex(const Source *source, unsigned long offset, TokenInfo *info, Report *report);

#endif
