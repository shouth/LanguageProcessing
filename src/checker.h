#ifndef CHECKER_H
#define CHECKER_H

#include "inference.h"
#include "resolution.h"
#include "source.h"
#include "token_tree.h"

int mppl_check(const Source *source, const TokenNode *tree, const Res *resolution, Infer **inference);

#endif
