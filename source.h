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

    struct {
        char *str_ptr;
        stroff_t str_size;
    };

    struct {
        stroff_t *lines_ptr;
        size_t lines_size;
    };
} source_t;

#if defined(_WIN32)
#   define stat_type __stat64
#   define stat_func _stat64
#elif defined(__APPLE__)
#   define stat_type stat
#   define stat_func stat
#elif defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__) || defined(__DragonFly__)
#   define stat_type stat
#   define stat_func stat
#elif defined(__linux__)
#   define stat_type stat64
#   define stat_func stat64
#endif

#if defined(stat_type) && defined(stat_func)

static stroff_t filesize(const char *filename)
{
    struct stat_type s;
    stat_func(filename, &s);

    if (s.st_mode & S_IFREG) {
        return str_npos;
    }
    return s.st_size;
}

#undef stat_type
#undef stat_func

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
    fclose(file);

    return ret;
}

#endif

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
        return NULL;
    }
    strcpy(src->filename, filename);

    src->str_size = filesize(filename);
    if (src->str_size < 0) {
        return NULL;
    }

    src->str_ptr = (char *) malloc(sizeof(char) * (src->str_size + 1));
    if (src->str_ptr == NULL) {
        return NULL;
    }
    src->str_ptr[src->str_size] = '\0';
    file = fopen(filename, "r");
    if (file == NULL) {
        return NULL;
    }
    fread(src->str_ptr, sizeof(char), src->str_size, file);
    if (ferror(file) != 0) {
        return NULL;
    }
    fclose(file);

    linecnt = 0;
    for (cur = src->str_ptr; cur[0] != '\0'; cur += strcspn(cur, "\r\n")) {
        linecnt++;
        if (strncmp("\r\n", cur, 2) == 0 || strncmp("\n\r", cur, 2) == 0) {
            cur += 2;
        } else {
            cur += 1;
        }
    }

    src->lines_size = linecnt;
    src->lines_ptr = (stroff_t *) malloc(sizeof(stroff_t) * src->lines_size);
    if (src->lines_ptr == NULL) {
        return NULL;
    }

    linecnt = 0;
    for (cur = src->str_ptr; cur[0] != '\0'; cur += strcspn(cur, "\r\n")) {
        if (strncmp("\r\n", cur, 2) == 0 || strncmp("\n\r", cur, 2) == 0) {
            cur += 2;
        } else {
            cur += 1;
        }
        src->lines_ptr[linecnt] = cur - src->str_ptr;
        linecnt++;
    }

    return src;
}

int source_free(source_t *src)
{
    free(src->filename);
    free(src->str_ptr);
    free(src->lines_ptr);
    free(src);
}

#endif SOURCE_H