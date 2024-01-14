#ifndef PARSER_H
#define PARSER_H

#include "mppl_syntax.h"
#include "source.h"

int mppl_parse(const Source *source, MpplProgram **syntax);

#endif
