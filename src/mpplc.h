#ifndef MPPL_H
#define MPPL_H

#include "ast.h"
#include "context.h"
#include "utility.h"

/* parser.c */

int parse(context_t *);

/* pretty_print.c */

void pretty(context_t *);

/* resolve.c */

void resolve(context_t *);

/* check.c */

void check(context_t *);

#endif
