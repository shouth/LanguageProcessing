#ifndef SOURCE_H
#define SOURCE_H

typedef struct SourceLocation SourceLocation;
typedef struct Source         Source;

struct SourceLocation {
  unsigned long line;
  unsigned long column;
  unsigned long length;
};

struct Source {
  char          *_file_name;
  unsigned long  _file_name_length;
  char          *_text;
  unsigned long  _text_length;
  unsigned long *_line_offsets;
  unsigned long *_line_lengths;
  unsigned long  _line_count;
};

int           source_init(Source *source, const char *file_name, unsigned long file_name_length);
void          source_deinit(Source *source);
const char   *source_text(const Source *source);
unsigned long source_length(const Source *source);
const char   *source_file_name(const Source *source);
unsigned long source_file_name_length(const Source *source);
unsigned long source_location(const Source *source, unsigned long offset, SourceLocation *location);

#endif