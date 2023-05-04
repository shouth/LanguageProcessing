#ifndef MPPL_H
#define MPPL_H

#include "ast.h"
#include "context.h"
#include "utility.h"

/* parser.c */

int mpplc_parse(context_t *);

/* pretty_print.c */

void mpplc_pretty(context_t *);

/* resolve.c */

void mpplc_resolve(context_t *);

/* check.c */

void mpplc_check(context_t *);

#endif
