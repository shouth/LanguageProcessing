#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "mppl.h"

static int symbol_comparator(const void *lhs, const void *rhs)
{
    const symbol_instance_t *l = lhs, *r = rhs;
    size_t len = l->len < r->len ? l->len : r->len;
    int ret = strncmp(l->ptr, r->ptr, len);
    return ret ? 0 : (l->len == r->len);
}

static uint64_t symbol_hasher(const void *ptr)
{
    const symbol_instance_t *s = ptr;
    return fnv1((const uint8_t *) s->ptr, s->len);
}

symbol_storage_t *new_symbol_storage()
{
    symbol_storage_t *ret = new(symbol_storage_t);
    ret->table = new_hash_table(symbol_comparator, symbol_hasher);
    return ret;
}

symbol_storage_t *delete_symbol_storage(symbol_storage_t *storage)
{
    if (!storage) {
        return NULL;
    }
    delete_hash_table(storage->table, free, NULL);
    free(storage);
}

symbol_t symbol_intern(symbol_storage_t *storage, const char *ptr, size_t len)
{
    const hash_table_entry_t *entry;
    symbol_instance_t *ret;
    assert(storage && ptr);

    ret = new(symbol_instance_t);
    ret->ptr = ptr;
    ret->len = len;
    if (entry = hash_table_find(storage->table, ret)) {
        free(ret);
        return (symbol_t) entry->value;
    }
    hash_table_insert_unchecked(storage->table, ret, ret);
    return (symbol_t) ret;
}

const symbol_instance_t *symbol_get_instance(symbol_t symbol)
{
    return (symbol_instance_t *) symbol;
}
