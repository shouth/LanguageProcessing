#ifndef SOURCE_H
#define SOURCE_H

typedef struct SourceLocation SourceLocation;
typedef struct Source         Source;

struct SourceLocation {
  unsigned long line;
  unsigned long column;
};

struct Source {
  char          *file_name;
  unsigned long  file_name_length;
  char          *text;
  unsigned long  text_length;
  unsigned long *line_offsets;
  unsigned long *line_lengths;
  unsigned long  line_count;
};

int  source_init(Source *source, const char *file_name, unsigned long file_name_length);
void source_deinit(Source *source);
int  source_location(const Source *source, unsigned long offset, SourceLocation *location);

#endif
