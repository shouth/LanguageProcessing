#ifndef RESOLVER_H
#define RESOLVER_H

#include "resolution.h"
#include "source.h"
#include "token_tree.h"

int mppl_resolve(const Source *source, const TokenNode *tree, Res **resolution);

#endif
