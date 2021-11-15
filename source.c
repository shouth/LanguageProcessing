#define _FILE_OFFSET_BITS 64

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <sys/types.h>
#include <sys/stat.h>

#include "source.h"
#include "strref.h"

#if defined(_WIN32)

static size_t filesize(const char *filename)
{
    struct __stat64 s;

    assert(filename != NULL);
    if (_stat64(filename, &s) < 0) {
        return STRREF_NPOS;
    } else {
        if (s.st_mode & _S_IFREG) {
            return s.st_size;
        } else {
            return STRREF_NPOS;
        }
    }
}

#elif defined(__unix__) || defined(__APPLE__)

static size_t filesize(const char *filename)
{
    struct stat s;

    assert(filename != NULL);
    if (stat(filename, &s) < 0) {
        return STRREF_NPOS;
    } else {
        if (S_ISREG(s.st_mode)) {
            return s.st_size;
        } else {
            return STRREF_NPOS;
        }
    }
}

#else

static size_t filesize(const char *filename)
{
    const size_t block_size = 4096;
    char buf[block_size];
    FILE *file;
    size_t size;
    size_t ret = 0;

    assert(filename != NULL);
    file = fopen(filename, "r");
    setvbuf(file, NULL, _IOFBF, block_size);
    do {
        size = fread(buf, sizeof(*buf), block_size, file);
        ret += size;
    } while (size == block_size);

    if (ferror(file) != 0) {
        ret = STRREF_NPOS;
    }
    fclose(file);
    return ret;
}

#endif

static const char *nextline(const char *str)
{
    assert(str != NULL);
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

    assert(filename != NULL);

    src = (source_t *) malloc(sizeof(source_t));
    if (src == NULL) {
        return NULL;
    }
    src->filename = (char *) malloc(sizeof(char) * (strlen(filename) + 1));
    if (src->filename == NULL) {
        source_free(src);
        return NULL;
    }
    strcpy(src->filename, filename);

    src->strref_size = filesize(filename);
    if (src->strref_size == STRREF_NPOS) {
        source_free(src);
        return NULL;
    }

    src->strref_ptr = (char *) malloc(sizeof(char) * (src->strref_size + 1));
    if (src->strref_ptr == NULL) {
        source_free(src);
        return NULL;
    }
    file = fopen(filename, "r");
    if (file == NULL) {
        source_free(src);
        return NULL;
    }
    fread(src->strref_ptr, sizeof(char), src->strref_size, file);
    if (ferror(file) != 0) {
        source_free(src);
        return NULL;
    }
    fclose(file);
    src->strref_ptr[src->strref_size] = '\0';

    linecnt = 0;
    for (cur = src->strref_ptr; cur[0] != '\0'; cur = nextline(cur)) {
        linecnt++;
    }

    src->lines_size = linecnt;
    src->lines_ptr = (size_t *) malloc(sizeof(size_t) * src->lines_size);
    if (src->lines_ptr == NULL) {
        source_free(src);
        return NULL;
    }

    linecnt = 0;
    for (cur = src->strref_ptr; cur[0] != '\0'; cur = nextline(cur)) {
        src->lines_ptr[linecnt] = cur - src->strref_ptr;
        linecnt++;
    }

    return src;
}

void source_free(source_t *src)
{
    if (src != NULL) {
        if (src->filename != NULL) {
            free(src->filename);
        }
        if (src->strref_ptr != NULL) {
            free(src->strref_ptr);
        }
        if (src->lines_ptr != NULL) {
            free(src->lines_ptr);
        }
        free(src);
    }
}

void source_str(source_t *src, strref_t *strref)
{
    assert(src != NULL && strref != NULL);
    strref_init(strref, src->strref_ptr, src->strref_size);
}

size_t source_line_at(source_t *src, size_t index)
{
    size_t left = 0, right = src->lines_size, middle;

    assert(src != NULL && src->lines_ptr != NULL);
    assert(index < src->strref_size);

    while (right - left > 1) {
        middle = (right - left) / 2 + left;

        if (src->lines_ptr[middle] <= index) {
            left = middle;
        } else {
            right = middle;
        }
    }

    return left;
}