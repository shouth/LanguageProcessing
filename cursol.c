#include <assert.h>

#include "cursol.h"

void cursol_init(cursol_t *cur, strref_t *strref)
{
    assert(cur != NULL && strref != NULL);
    cur->initial_size = strref_size(strref);
    cur->strref = *strref;
}

int cursol_nth(const cursol_t *cur, size_t index)
{
    assert(cur != NULL);
    if (index >= strref_size(&cur->strref)) {
        return EOF;
    }
    return strref_at(&cur->strref, index);
}

int cursol_first(const cursol_t *cur)
{
    return cursol_nth(cur, 0);
}

int cursol_second(const cursol_t *cur)
{
    return cursol_nth(cur, 1);
}

int cursol_eof(const cursol_t *cur)
{
    return strref_empty(&cur->strref);
}

void cursol_next(cursol_t *cur)
{
    assert(cur != NULL);
    if (cursol_eof(cur)) {
        return cur;
    }
    strref_slice(&cur->strref, &cur->strref, 1, STRREF_NPOS);
}

size_t cursol_consumed(const cursol_t *cur)
{
    assert(cur != NULL);
    return cur->initial_size - strref_size(&cur->strref);
}
