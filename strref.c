#include <assert.h>
#include <stddef.h>
#include <string.h>

#include "strref.h"

static const char *empty = "";

const char *strref_data(strref_t str)
{
    return str.data;
}

size_t strref_size(strref_t str)
{
    return str.size;
}

int strref_empty(strref_t str)
{
    return strref_size(str) == 0;
}

strref_t strref_new(const char *data, size_t size)
{
    if (data == NULL) {
        data = empty;
        size = 0;
    }
    if (size == STRREF_NPOS) {
        size = strlen(data);
    }

    return (strref_t) { data, size };
}

int strref_at(strref_t str, size_t index)
{
    assert(str.data != NULL);

    if (index >= str.size) {
        return -1;
    }

    return str.data[index];
}

strref_t strref_slice(strref_t str, size_t begin, size_t end)
{
    assert(str.data != NULL);

    if (begin == STRREF_NPOS) {
        begin = 0;
    }
    if (end == STRREF_NPOS) {
        end = str.size;
    }

    assert(begin <= end && end <= str.size);
    return strref_new(str.data + begin, end - begin);
}
