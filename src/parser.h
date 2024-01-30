#ifndef PARSER_H
#define PARSER_H

#include "context_fwd.h"
#include "mppl_syntax.h"
#include "source.h"

int mppl_parse(const Source *source, Ctx *ctx, MpplProgram **syntax);

#endif
