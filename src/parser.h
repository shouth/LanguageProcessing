#ifndef PARSER_H
#define PARSER_H

#include "mppl_syntax.h"
#include "source.h"
#include "string.h"

int mppl_parse(const Source *source, StringContext *strings, MpplProgram **syntax);

#endif
