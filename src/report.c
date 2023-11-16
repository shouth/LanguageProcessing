#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "report.h"
#include "utility.h"
#include "vector.h"

typedef struct Label Label;
typedef struct Note  Note;

struct Label {
  unsigned long start;
  unsigned long end;
  char         *message;
};

struct Note {
  unsigned long start;
  unsigned long end;
  char         *message;
};

char *vformat(const char *format, va_list args)
{
  FILE         *file   = tmpfile();
  unsigned long length = vfprintf(file, format, args);
  char         *text   = xmalloc(sizeof(char) * (length + 1));
  rewind(file);
  fread(text, sizeof(char), length, file);
  text[length] = '\0';
  fclose(file);
  return text;
}

void report_init(Report *report, ReportKind kind, unsigned long start, unsigned long end, const char *format, ...)
{
  va_list args;
  va_start(args, format);
  report_init_with_args(report, kind, start, end, format, args);
  va_end(args);
}

void report_init_with_args(Report *report, ReportKind kind, unsigned long start, unsigned long end, const char *format, va_list args)
{
  report->_kind    = kind;
  report->_start   = start;
  report->_end     = end;
  report->_message = vformat(format, args);
  vector_init(&report->_notes, sizeof(Note));
  vector_init(&report->_labels, sizeof(Label));
}

void report_label(Report *report, unsigned long start, unsigned long end, const char *format, ...)
{
  va_list args;
  va_start(args, format);
  report_label_with_args(report, start, end, format, args);
  va_end(args);
}

void report_label_with_args(Report *report, unsigned long start, unsigned long end, const char *format, va_list args)
{
  Label label;
  label.start   = start;
  label.end     = end;
  label.message = vformat(format, args);
  vector_push(&report->_labels, &label);
}

void report_note(Report *report, unsigned long start, unsigned long end, const char *format, ...)
{
  va_list args;
  va_start(args, format);
  report_note_with_args(report, start, end, format, args);
  va_end(args);
}

void report_note_with_args(Report *report, unsigned long start, unsigned long end, const char *format, va_list args)
{
  Note note;
  note.start   = start;
  note.end     = end;
  note.message = vformat(format, args);
  vector_push(&report->_notes, &note);
}

void report_deinit(Report *report)
{
  free(report->_message);
  vector_deinit(&report->_notes);
  vector_deinit(&report->_labels);
}
