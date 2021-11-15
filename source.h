#ifndef SOURCE_H
#define SOURCE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "str.h"

typedef struct {
    char *filename;

    char *str_ptr;
    stroff_t str_size;

    stroff_t *lines_ptr;
    size_t lines_size;
} source_t;

#if defined(_WIN32)
#   define stat_type __stat64
#   define stat_func _stat64
#   define isreg(x) ((x & _S_IFMT) == _S_IFREG)
#elif defined(__APPLE__)
#   define stat_type stat
#   define stat_func stat
#   define isreg(x) S_ISREG(x)
#elif defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__) || defined(__DragonFly__)
#   define stat_type stat
#   define stat_func stat
#   define isreg(x) S_ISREG(x)
#elif defined(__linux__)
#   define stat_type stat64
#   define stat_func stat64
#   define isreg(x) S_ISREG(x)
#endif

#if defined(stat_type) && defined(stat_func) && defined(isreg)

static stroff_t filesize(const char *filename)
{
    struct stat_type s;
    stat_func(filename, &s);

    if (isreg(s.st_mode)) {
        return STR_NPOS;
    }
    return s.st_size;
}

#undef stat_type
#undef stat_func
#undef isreg

#else

static stroff_t filesize(const char *filename)
{
    const size_t block_size = 4096;
    char buf[block_size];
    FILE *file;
    size_t size;
    stroff_t ret = 0;

    file = fopen(filename);
    do {
        size = fread(buf, sizeof(*buf), block_size, file);
        ret += size;
    } while (size != block_size);

    if (ferror(file) != 0) {
        fclose(file);
        return STR_NPOS;
    }
    return ret;
}

#endif

static size_t nextline(const char *str)
{
    size_t ret = strcspn(str, "\r\n");
    if (strncmp("\r\n", str + ret, 2) == 0 || strncmp("\n\r", str + ret, 2) == 0) {
        return ret + 2;
    }
    return ret + 1;
}

source_t *source_new(const char *filename)
{
    source_t *src;
    FILE *file;
    size_t linecnt;
    char *cur;

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

    src->str_size = filesize(filename);
    if (src->str_size == STR_NPOS) {
        source_free(src);
        return NULL;
    }

    src->str_ptr = (char *) malloc(sizeof(char) * (src->str_size + 1));
    if (src->str_ptr == NULL) {
        source_free(src);
        return NULL;
    }
    src->str_ptr[src->str_size] = '\0';
    file = fopen(filename, "r");
    if (file == NULL) {
        source_free(src);
        return NULL;
    }
    fread(src->str_ptr, sizeof(char), src->str_size, file);
    if (ferror(file) != 0) {
        source_free(src);
        return NULL;
    }
    fclose(file);

    linecnt = 0;
    for (cur = src->str_ptr; cur[0] != '\0'; cur += nextline(cur)) {
        linecnt++;
    }

    src->lines_size = linecnt;
    src->lines_ptr = (stroff_t *) malloc(sizeof(stroff_t) * src->lines_size);
    if (src->lines_ptr == NULL) {
        source_free(src);
        return NULL;
    }

    linecnt = 0;
    for (cur = src->str_ptr; cur[0] != '\0'; cur += nextline(cur)) {
        src->lines_ptr[linecnt] = cur - src->str_ptr;
        linecnt++;
    }

    return src;
}

int source_free(source_t *src)
{
    if (src != NULL) {
        if (src->filename != NULL) {
            free(src->filename);
        }
        if (src->str_ptr != NULL) {
            free(src->str_ptr);
        }
        if (src->lines_ptr != NULL) {
            free(src->lines_ptr);
        }
        free(src);
    }
}

#endif SOURCE_H