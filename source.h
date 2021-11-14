#ifndef SOURCE_H
#define SOURCE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/stat.h>

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

static stroff_t filesize(const char *filename)
{

}

source_t *source_new(const char *filename)
{
    source_t *src;
    FILE *file;

    src = (source_t *) malloc(sizeof(source_t));
    if (src == NULL) {
        return NULL;
    }
    src->filename = (char *) malloc(sizeof(char) * strlen(filename));
    strcpy(src->filename, filename);

    src->str_size = filesize(filename);
    if (src->str_size < 0) {
        return NULL;
    }

    file = fopen(filename, "r");
    fread(src->str_ptr, src->str_size, src->str_size, file);
    fclose(file);
}

int source_free(source_t *src)
{
    free(src->filename);
    free(src->str_ptr);
    free(src->lines_ptr);
    free(src);
}

#endif SOURCE_H