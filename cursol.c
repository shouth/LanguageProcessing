#include <assert.h>

#include "cursol.h"

void cursol_init(cursol_t *cur, const char *src, size_t len)
{
    assert(cur != NULL && src != NULL);
    cur->init_len = len;
    cur->ptr = src;
    cur->len = len;
}

int cursol_nth(const cursol_t *cur, size_t index)
{
    assert(cur != NULL);
    if (index >= cur->len) {
        return EOF;
    }
    return cur->ptr[index];
}

int cursol_first(const cursol_t *cur)
{
    assert(cur != NULL);
    return cursol_nth(cur, 0);
}

int cursol_second(const cursol_t *cur)
{
    assert(cur != NULL);
    return cursol_nth(cur, 1);
}

int cursol_eof(const cursol_t *cur)
{
    assert(cur != NULL);
    return cur->len == 0;
}

void cursol_next(cursol_t *cur)
{
    assert(cur != NULL);
    if (!cursol_eof(cur)) {
        cur->ptr++;
        cur->len--;
    }
}

size_t cursol_consumed(const cursol_t *cur)
{
    assert(cur != NULL);
    return cur->init_len - cur->len;
}
