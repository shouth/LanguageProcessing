#define _FILE_OFFSET_BITS 64

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

#include <sys/types.h>
#include <sys/stat.h>

#include "mppl.h"

static size_t filesize(const char *filename)
{
#if defined(__unix__) || defined(__APPLE__)
    struct stat s;

    assert(filename);
    if (stat(filename, &s) < 0) {
        return SIZE_MAX;
    } else {
        if (S_ISREG(s.st_mode)) {
            return s.st_size;
        } else {
            return SIZE_MAX;
        }
    }

#elif defined(_WIN32)
    struct __stat64 s;

    assert(filename);
    if (_stat64(filename, &s) < 0) {
        return SIZE_MAX;
    } else {
        if (s.st_mode & _S_IFREG) {
            return s.st_size;
        } else {
            return SIZE_MAX;
        }
    }

#else
    const size_t block_size = 4096;
    char buf[block_size];
    FILE *file;
    size_t size;
    size_t ret = 0;

    assert(filename);
    file = fopen(filename, "r");
    setvbuf(file, NULL, _IOFBF, block_size);
    do {
        size = fread(buf, sizeof(*buf), block_size, file);
        ret += size;
    } while (size == block_size);

    if (ferror(file) != 0) {
        ret = SIZE_MAX;
    }
    fclose(file);
    return ret;

#endif
}

static const char *nextline(const char *str)
{
    assert(str);
    str += strcspn(str, "\r\n");
    if (str[0] == '\0') {
        return str;
    }
    if (strncmp("\r\n", str, 2) == 0 || strncmp("\n\r", str, 2) == 0) {
        return str + 2;
    }
    return str + 1;
}

source_t *new_source(const char *filename)
{
    source_t *src;
    assert(filename);

    src = new(source_t);

    src->filename = new_arr(char, strlen(filename) + 1);
    {
        strcpy(src->filename, filename);
    }

    src->src_size = filesize(filename);
    {
        if (src->src_size == SIZE_MAX) {
            delete_source(src);
            return NULL;
        }
    }

    src->src_ptr = new_arr(char, src->src_size + 1);
    {
        FILE *file = fopen(filename, "r");
        if (!file) {
            delete_source(src);
            return NULL;
        }
        fread(src->src_ptr, sizeof(char), src->src_size, file);
        if (ferror(file)) {
            delete_source(src);
            return NULL;
        }
        fclose(file);
        src->src_ptr[src->src_size] = '\0';
    }

    src->lines_size = 0;
    {
        const char *cur;
        for (cur = src->src_ptr; *cur; cur = nextline(cur)) {
            src->lines_size++;
        }
    }

    src->lines_ptr = new_arr(size_t, src->lines_size + 1);
    {
        const char *cur;
        size_t linecnt = 0;
        for (cur = src->src_ptr; *cur; cur = nextline(cur)) {
            src->lines_ptr[linecnt] = cur - src->src_ptr;
            linecnt++;
        }
        src->lines_ptr[linecnt] = cur - src->src_ptr;
    }

    return src;
}

void delete_source(source_t *src)
{
    free(src->filename);
    free(src->src_ptr);
    free(src->lines_ptr);
    free(src);
}

location_t location_from(size_t line, size_t col)
{
    location_t ret;
    ret.line = line;
    ret.col = col;
    return ret;
}

location_t source_location(const source_t *src, size_t index)
{
    size_t left, right, middle;
    assert(src && src->lines_ptr);

    left = 0;
    right = src->lines_size;

    while (right - left > 1) {
        middle = (right - left) / 2 + left;

        if (src->lines_ptr[middle] <= index) {
            left = middle;
        } else {
            right = middle;
        }
    }

    return location_from(left + 1, index - src->lines_ptr[left] + 1);
}

region_t region_from(size_t pos, size_t len)
{
    region_t ret;
    ret.pos = pos;
    ret.len = len;
    return ret;
}

region_t region_unite(region_t a, region_t b)
{
    return region_from(a.pos, b.pos + b.len - a.pos);
}

int region_compare(region_t a, region_t b)
{
    if (a.pos < b.pos) {
        return -1;
    } else if (a.pos > b.pos) {
        return 1;
    } else if (a.pos + a.len < b.pos + b.len) {
        return -1;
    } else if (a.pos + a.len > b.pos + b.len) {
        return 1;
    }
    return 0;
}
