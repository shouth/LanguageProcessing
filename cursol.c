#include <assert.h>

#include "cursol.h"

void cursol_init(cursol_t *cur, const source_t *src, const char *ptr, size_t len)
{
    assert(cur && src);
    cur->init_len = len;
    cur->ptr = ptr;
    cur->len = len;
    cur->src = src;
}

int cursol_nth(const cursol_t *cur, size_t index)
{
    assert(cur);
    if (index >= cur->len) {
        return EOF;
    }
    return cur->ptr[index];
}

int cursol_first(const cursol_t *cur)
{
    assert(cur);
    return cursol_nth(cur, 0);
}

int cursol_second(const cursol_t *cur)
{
    assert(cur);
    return cursol_nth(cur, 1);
}

int cursol_eof(const cursol_t *cur)
{
    assert(cur);
    return cur->len == 0;
}

void cursol_next(cursol_t *cur)
{
    assert(cur);
    if (!cursol_eof(cur)) {
        cur->ptr++;
        cur->len--;
    }
}

size_t cursol_position(const cursol_t *cur)
{
    assert(cur);
    return cur->init_len - cur->len;
}
