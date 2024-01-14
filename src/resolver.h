#ifndef RESOLVER_H
#define RESOLVER_H

#include "mppl_syntax.h"
#include "resolution.h"
#include "source.h"

int mppl_resolve(const Source *source, const MpplProgram *syntax, Res **resolution);

#endif
