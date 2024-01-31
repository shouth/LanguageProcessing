#include <limits.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "array.h"
#include "source.h"
#include "utility.h"

Source *source_new(const char *file_name, unsigned long file_name_length)
{
  Source *source = xmalloc(sizeof(Source));
  {
    source->file_name = xmalloc(file_name_length + 1);
    strncpy(source->file_name, file_name, file_name_length);
    source->file_name[file_name_length] = '\0';
    source->file_name_length            = file_name_length;
  }

  {
    FILE *file = fopen(source->file_name, "rb");
    if (file) {
      Array        *text = array_new(sizeof(char));
      char          buffer[4096];
      unsigned long length;
      while ((length = fread(buffer, sizeof(char), sizeof(buffer), file)) > 0) {
        array_push_count(text, buffer, length);
      }
      buffer[0] = '\0';
      array_push(text, buffer);
      array_fit(text);
      source->text_length = array_count(text) - 1;
      source->text        = array_steal(text);
    } else {
      source->text_length = -1ul;
      source->text        = NULL;
    }
    fclose(file);
  }

  if (source->text) {
    Array        *line_offsets = array_new(sizeof(unsigned long));
    Array        *line_lengths = array_new(sizeof(unsigned long));
    unsigned long offset       = 0;
    array_push(line_offsets, &offset);
    while (offset < source->text_length) {
      unsigned long length = strcspn(source->text + offset, "\r\n");
      array_push(line_lengths, &length);
      offset += length;

      if (offset < source->text_length) {
        if (!strncmp(source->text + offset, "\r\n", 2) || !strncmp(source->text + offset, "\n\r", 2)) {
          offset += 2;
        } else {
          offset += 1;
        }
        array_push(line_offsets, &offset);
      }
    }
    source->line_count   = array_count(line_offsets);
    source->line_offsets = array_steal(line_offsets);
    source->line_lengths = array_steal(line_lengths);
  } else {
    source->line_count   = -1ul;
    source->line_offsets = NULL;
    source->line_lengths = NULL;
  }

  if (source->file_name && source->text && source->line_offsets) {
    return source;
  } else {
    source_free(source);
    return NULL;
  }
}

void source_free(Source *source)
{
  if (source) {
    free(source->file_name);
    free(source->text);
    free(source->line_offsets);
    free(source->line_lengths);
    free(source);
  }
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
