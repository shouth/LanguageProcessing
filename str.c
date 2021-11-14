#include <assert.h>
#include <stddef.h>
#include <string.h>

#include "str.h"

static const char *empty = "";

const char *str_data(strref_t str)
{
    return str.data;
}

stroff_t str_size(strref_t str)
{
    return str.size;
}

strref_t str_new(const char *data, stroff_t size)
{
    if (data == NULL) {
        data = empty;
        size = 0;
    }
    if (size == str_npos) {
        size = strlen(data);
    }

    return (strref_t) { data, size };
}

int str_at(strref_t str, stroff_t index)
{
    assert(str.data != NULL);

    if (index >= str.size) {
        return -1;
    }

    return str.data[index];
}

strref_t str_slice(strref_t str, stroff_t begin, stroff_t end)
{
    assert(str.data != NULL);

    if (begin == str_npos) {
        begin = 0;
    }
    if (end == str_npos) {
        end = str.size;
    }

    assert(begin <= end && end <= str.size);
    return str_new(str.data + begin, end - begin);
}
