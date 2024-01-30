#ifndef CHECKER_H
#define CHECKER_H

#include "context_fwd.h"
#include "mppl_syntax.h"
#include "source.h"

int mppl_check(const Source *source, const MpplProgram *syntax, Ctx *ctx);

#endif
