#ifndef SOURCE_H
#define SOURCE_H

typedef struct {
    char *filename;

    char *src_ptr;
    size_t src_size;

    size_t *lines_ptr;
    size_t lines_size;
} source_t;

source_t *source_new(const char *filename);

void source_free(source_t *src);

void source_location(source_t *src, size_t index, size_t *line, size_t *col);

#endif /* SOURCE_H */
