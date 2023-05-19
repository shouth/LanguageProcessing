#ifndef MPPL_H
#define MPPL_H

#include "context.h"

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

/* crossref.c */

void mpplc_crossref(context_t *);

#endif
