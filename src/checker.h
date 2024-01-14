#ifndef CHECKER_H
#define CHECKER_H

#include "inference.h"
#include "mppl_syntax.h"
#include "resolution.h"
#include "source.h"

int mppl_check(const Source *source, const MpplProgram *syntax, const Res *resolution, Infer **inference);

#endif
