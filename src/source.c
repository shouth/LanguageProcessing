#include <limits.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "source.h"
#include "utility.h"
#include "vector.h"

int source_init(Source *source, const char *file_name, unsigned long file_name_length)
{
  {
    source->file_name = xmalloc(file_name_length + 1);
    strncpy(source->file_name, file_name, file_name_length);
    source->file_name[file_name_length] = '\0';
  }

  {
    FILE *file = fopen(source->file_name, "rb");
    if (file) {
      Vector        text;
      char          buffer[4096];
      unsigned long length;
      vector_init(&text, sizeof(char));
      while ((length = fread(buffer, sizeof(char), sizeof(buffer), file)) > 0) {
        vector_push_n(&text, buffer, length);
      }
      buffer[0] = '\0';
      vector_push(&text, buffer);
      vector_fit(&text);
      source->text_length = vector_count(&text) - 1;
      source->text        = vector_steal(&text);
    } else {
      source->text_length = -1ul;
      source->text        = NULL;
    }
    fclose(file);
  }

  if (source->text) {
    Vector        line_offsets;
    Vector        line_lengths;
    unsigned long offset = 0;
    vector_init(&line_offsets, sizeof(unsigned long));
    vector_init(&line_lengths, sizeof(unsigned long));
    vector_push(&line_offsets, &offset);
    while (offset < source->text_length) {
      unsigned long length = strcspn(source->text + offset, "\r\n");
      vector_push(&line_lengths, &length);
      offset += length;

      if (offset < source->text_length) {
        if (!strncmp(source->text + offset, "\r\n", 2) || !strncmp(source->text + offset, "\n\r", 2)) {
          offset += 2;
        } else {
          offset += 1;
        }
        vector_push(&line_offsets, &offset);
      }
    }
    vector_fit(&line_offsets);
    vector_fit(&line_lengths);
    source->line_count   = vector_count(&line_offsets);
    source->line_offsets = vector_steal(&line_offsets);
    source->line_lengths = vector_steal(&line_lengths);
  } else {
    source->line_count   = -1ul;
    source->line_offsets = NULL;
    source->line_lengths = NULL;
  }

  if (source->file_name && source->text && source->line_offsets) {
    return 1;
  } else {
    source_deinit(source);
    return 0;
  }
}

void source_deinit(Source *source)
{
  free(source->file_name);
  free(source->text);
  free(source->line_offsets);
  free(source->line_lengths);
}

const char *source_text(const Source *source)
{
  return source->text;
}

unsigned long source_length(const Source *source)
{
  return source->text_length;
}

const char *source_file_name(const Source *source)
{
  return source->file_name;
}

int source_location(const Source *source, unsigned long offset, SourceLocation *location)
{
  if (offset >= source->text_length) {
    return 0;
  } else {
    unsigned long left  = 0;
    unsigned long right = source->line_count;
    while (right - left > 1) {
      unsigned long middle = (right - left) / 2 + left;
      if (offset < source->line_offsets[middle]) {
        right = middle;
      } else {
        left = middle;
      }
    }
    if (location) {
      location->line   = left;
      location->column = offset - source->line_offsets[left];
    }
    return 1;
  }
}
