#include <assert.h>
#include <stddef.h>
#include <string.h>

#include "strref.h"

static const char *empty = "";

const char *strref_data(const strref_t *strref)
{
    assert(strref != NULL && strref->data != NULL);
    return strref->data;
}

size_t strref_size(const strref_t *strref)
{
    assert(strref != NULL);
    return strref->size;
}

int strref_empty(const strref_t *strref)
{
    return strref_size(strref) == 0;
}

void strref_init(strref_t *strref, const char *data, size_t size)
{
    assert(strref != NULL);

    if (data == NULL) {
        data = empty;
        size = 0;
    }
    if (size == STRREF_NPOS) {
        size = strlen(data);
    }

    strref->data = data;
    strref->size = size;
}

int strref_at(const strref_t *strref, size_t index)
{
    assert(strref != NULL && strref->data != NULL);
    assert(index < strref->size);
    return strref->data[index];
}

void strref_slice(strref_t *ret, const strref_t *strref, size_t begin, size_t end)
{
    assert(strref != NULL);
    assert(strref->data != NULL);

    if (begin == STRREF_NPOS) {
        begin = 0;
    }
    if (end == STRREF_NPOS) {
        end = strref->size;
    }

    assert(begin <= end && end <= strref->size);
    strref_init(ret, strref->data + begin, end - begin);
}
