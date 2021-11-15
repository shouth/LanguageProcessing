#include "cursol.h"

cursol_t cursol_new(strref_t strref)
{
    return (cursol_t) { strref.size, strref };
}

int cursol_nth(cursol_t cur, size_t index)
{
    int c = strref_at(cur.strref, index);
    if (c < 0) {
        return EOF;
    }
    return c;
}

int cursol_first(cursol_t cur)
{
    return cursol_nth(cur, 0);
}

int cursol_second(cursol_t cur)
{
    return cursol_nth(cur, 1);
}

int cursol_eof(cursol_t cur)
{
    return strref_empty(cur.strref);
}

cursol_t cursol_next(cursol_t cur)
{
    if (cursol_eof(cur)) {
        return cur;
    }
    return cursol_new(strref_slice(cur.strref, 1, STRREF_NPOS));
}

size_t cursol_consumed(cursol_t cur)
{
    return cur.initial_size - strref_size(cur.strref);
}
