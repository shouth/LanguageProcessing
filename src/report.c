#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "array.h"
#include "report.h"
#include "utility.h"

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

void report_init(Report *report, ReportKind kind, unsigned long offset, const char *format, ...)
{
  va_list args;
  va_start(args, format);
  report_init_with_args(report, kind, offset, format, args);
  va_end(args);
}

void report_deinit(Report *report)
{
  unsigned long i;
  free(report->_message);

  for (i = 0; i < array_count(&report->_notes); ++i) {
    char **note = array_at(&report->_notes, i);
    free(*note);
  }
  array_deinit(&report->_notes);

  for (i = 0; i < array_count(&report->_annotations); ++i) {
    ReportAnnotation *label = array_at(&report->_annotations, i);
    free(label->_message);
  }
  array_deinit(&report->_annotations);
}

void report_init_with_args(Report *report, ReportKind kind, unsigned long offset, const char *format, va_list args)
{
  report->_kind    = kind;
  report->_offset  = offset;
  report->_message = vformat(format, args);
  array_init(&report->_notes, sizeof(char *));
  array_init(&report->_annotations, sizeof(ReportAnnotation));
}

void report_annotation(Report *report, unsigned long start, unsigned long end, const char *format, ...)
{
  va_list args;
  va_start(args, format);
  report_annotation_with_args(report, start, end, format, args);
  va_end(args);
}

void report_annotation_with_args(Report *report, unsigned long start, unsigned long end, const char *format, va_list args)
{
  ReportAnnotation label;
  label._start_offset = start;
  label._end_offset   = end;
  label._message      = vformat(format, args);
  array_push(&report->_annotations, &label);
}

void report_note(Report *report, const char *format, ...)
{
  va_list args;
  va_start(args, format);
  report_note_with_args(report, format, args);
  va_end(args);
}

void report_note_with_args(Report *report, const char *format, va_list args)
{
  char *note = vformat(format, args);
  array_push(&report->_notes, &note);
}
