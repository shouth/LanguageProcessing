#ifndef SOURCE_H
#define SOURCE_H

typedef struct {
    char *filename;

    char *src_ptr;
    size_t src_size;

    size_t *lines_ptr;
    size_t lines_size;
} source_t;

typedef struct {
    size_t line;
    size_t col;

    const source_t *src;
} location_t;

source_t *source_new(const char *filename);

void source_free(source_t *src);

void source_location(const source_t *src, size_t index, location_t *loc);

#endif /* SOURCE_H */
