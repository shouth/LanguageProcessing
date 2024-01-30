#ifndef RESOLVER_H
#define RESOLVER_H

#include "context_fwd.h"
#include "mppl_syntax.h"
#include "source.h"

int mppl_resolve(const Source *source, const MpplProgram *syntax, Ctx *ctx);

#endif
