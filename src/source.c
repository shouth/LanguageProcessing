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
    source->_file_name = xmalloc(file_name_length + 1);
    strncpy(source->_file_name, file_name, file_name_length);
    source->_file_name[file_name_length] = '\0';
  }

  {
    FILE *file = fopen(source->_file_name, "rb");
    if (file) {
      Vector text;
      char   buffer[4096];
      vector_init(&text, sizeof(char));
      while (fread(buffer, sizeof(char), sizeof(buffer), file) > 0) {
        vector_push_n(&text, buffer, 4096);
      }
      buffer[0] = '\0';
      vector_push(&text, buffer);
      vector_fit(&text);
      source->_text_length = vector_count(&text);
      source->_text        = vector_steal(&text);
    } else {
      source->_text_length = -1ul;
      source->_text        = NULL;
    }
    fclose(file);
  }

  if (source->_text) {
    Vector        line_offsets;
    Vector        line_lengths;
    unsigned long offset = 0;
    vector_init(&line_offsets, sizeof(unsigned long));
    vector_init(&line_lengths, sizeof(unsigned long));
    vector_push(&line_offsets, &offset);
    while (offset < source->_text_length) {
      unsigned long length = strcspn(source->_text + offset, "\r\n");
      vector_push(&line_lengths, &length);
      offset += length;

      if (offset < source->_text_length) {
        if (!strncmp(source->_text + offset, "\r\n", 2) || !strncmp(source->_text + offset, "\n\r", 2)) {
          offset += 2;
        } else {
          offset += 1;
        }
        vector_push(&line_offsets, &offset);
      }
    }
    vector_fit(&line_offsets);
    vector_fit(&line_lengths);
    source->_line_count   = vector_count(&line_offsets);
    source->_line_offsets = vector_steal(&line_offsets);
    source->_line_lengths = vector_steal(&line_lengths);
  } else {
    source->_line_count   = -1ul;
    source->_line_offsets = NULL;
    source->_line_lengths = NULL;
  }

  if (source->_file_name && source->_text && source->_line_offsets) {
    return 1;
  } else {
    source_deinit(source);
    return 0;
  }
}

void source_deinit(Source *source)
{
  free(source->_file_name);
  free(source->_text);
  free(source->_line_offsets);
  free(source->_line_lengths);
}

const char *source_text(const Source *source)
{
  return source->_text;
}

unsigned long source_length(const Source *source)
{
  return source->_text_length;
}
const char *source_file_name(const Source *source)
{
  return source->_file_name;
}

unsigned long source_file_name_length(const Source *source)
{
  return source->_file_name_length;
}

unsigned long source_location(const Source *source, unsigned long offset, SourceLocation *location)
{
  if (offset >= source->_text_length) {
    return 0;
  } else {
    unsigned long left  = 0;
    unsigned long right = source->_line_count;
    while (right - left > 0) {
      unsigned long middle = (right - left) / 2 + left;
      if (source->_line_offsets[middle] <= offset) {
        left = middle;
      } else {
        right = middle;
      }
    }
    if (location) {
      location->line   = left + 1;
      location->column = offset - source->_line_offsets[left];
      location->length = source->_line_lengths[left];
    }
    return left + 1;
  }
}
