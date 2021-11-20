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

size_t source_line_at(source_t *src, size_t index);

#endif /* SOURCE_H */
