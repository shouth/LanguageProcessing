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
#if defined(_WIN32)
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

#elif defined(__unix__) || defined(__APPLE__)
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

source_t *source_new(const char *filename)
{
    source_t *src;
    FILE *file;
    size_t linecnt;
    const char *cur;

    assert(filename);

    src = (source_t *) xmalloc(sizeof(source_t));
    src->lines_ptr = NULL;
    src->src_ptr = NULL;

    src->filename = (char *) xmalloc(sizeof(char) * (strlen(filename) + 1));
    strcpy(src->filename, filename);

    src->src_size = filesize(filename);
    if (src->src_size == SIZE_MAX) {
        source_free(src);
        return NULL;
    }

    src->src_ptr = (char *) xmalloc(sizeof(char) * (src->src_size + 1));
    file = fopen(filename, "r");
    if (!file) {
        source_free(src);
        return NULL;
    }
    fread(src->src_ptr, sizeof(char), src->src_size, file);
    if (ferror(file)) {
        source_free(src);
        return NULL;
    }
    fclose(file);
    src->src_ptr[src->src_size] = '\0';

    linecnt = 0;
    for (cur = src->src_ptr; cur[0] != '\0'; cur = nextline(cur)) {
        linecnt++;
    }

    src->lines_size = linecnt;
    src->lines_ptr = (size_t *) xmalloc(sizeof(size_t) * (src->lines_size + 1));

    linecnt = 0;
    for (cur = src->src_ptr; cur[0] != '\0'; cur = nextline(cur)) {
        src->lines_ptr[linecnt] = cur - src->src_ptr;
        linecnt++;
    }
    src->lines_ptr[linecnt] = cur - src->src_ptr;

    return src;
}

void source_free(source_t *src)
{
    free(src->filename);
    free(src->src_ptr);
    free(src->lines_ptr);
    free(src);
}

void source_location(const source_t *src, size_t index, location_t *loc)
{
    size_t left, right, middle;

    assert(src && src->lines_ptr && loc);

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

    loc->line = left + 1;
    loc->col = index - src->lines_ptr[left] + 1;
    loc->src = src;
}
