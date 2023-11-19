#ifndef MESSAGE_H
#define MESSAGE_H

#include <stdarg.h>

#include "source.h"
#include "vector.h"

typedef enum {
  REPORT_KIND_NOTE,
  REPORT_KIND_WARN,
  REPORT_KIND_ERROR
} ReportKind;

typedef struct Report Report;

struct Report {
  ReportKind    _kind;
  unsigned long _start;
  unsigned long _end;
  char         *_message;
  Vector        _labels;
  Vector        _notes;
};

void report_init(Report *report, ReportKind kind, unsigned long start, unsigned long end, const char *format, ...);
void report_init_with_args(Report *report, ReportKind kind, unsigned long start, unsigned long end, const char *format, va_list args);
void report_deinit(Report *report);
void report_label(Report *report, unsigned long start, unsigned long end, const char *format, ...);
void report_label_with_args(Report *report, unsigned long start, unsigned long end, const char *format, va_list args);
void report_note(Report *report, unsigned long start, unsigned long end, const char *format, ...);
void report_note_with_args(Report *report, unsigned long start, unsigned long end, const char *format, va_list args);
void report_emit(Report *report, const Source *source);

#endif
