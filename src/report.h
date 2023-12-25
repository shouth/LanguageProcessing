#ifndef MESSAGE_H
#define MESSAGE_H

#include <stdarg.h>

#include "array.h"
#include "source.h"

typedef enum {
  REPORT_KIND_NOTE,
  REPORT_KIND_WARN,
  REPORT_KIND_ERROR
} ReportKind;

typedef struct ReportAnnotation ReportAnnotation;
typedef struct Report           Report;

struct ReportAnnotation {
  unsigned long  _start_offset;
  unsigned long  _end_offset;
  SourceLocation _start;
  SourceLocation _end;
  char          *_message;
};

struct Report {
  ReportKind    _kind;
  unsigned long _offset;
  char         *_message;
  Array         _annotations;
  Array         _notes;
};

void report_init(Report *report, ReportKind kind, unsigned long offset, const char *format, ...);
void report_init_with_args(Report *report, ReportKind kind, unsigned long offset, const char *format, va_list args);
void report_deinit(Report *report);
void report_annotation(Report *report, unsigned long start, unsigned long end, const char *format, ...);
void report_annotation_with_args(Report *report, unsigned long start, unsigned long end, const char *format, va_list args);
void report_note(Report *report, const char *format, ...);
void report_note_with_args(Report *report, const char *format, va_list args);
void report_emit(Report *report, const Source *source);

#endif
