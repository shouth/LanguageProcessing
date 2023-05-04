#ifndef MPPL_H
#define MPPL_H

#include "ast.h"
#include "context.h"
#include "utility.h"

/* context.c */

void mpplc_init(context_t *, const char *, const char *);
void mpplc_deinit(context_t *);

/* parser.c */

int mpplc_parse(context_t *);

/* pretty.c */

void mpplc_pretty(context_t *);

/* resolver.c */

void mpplc_resolve(context_t *);

/* checker.c */

void mpplc_check(context_t *);

#endif
