/* SPDX-License-Identifier: Apache-2.0 */

#ifndef REPORT_H
#define REPORT_H

#include <stdarg.h>

typedef enum {
  REPORT_KIND_NOTE,
  REPORT_KIND_WARN,
  REPORT_KIND_ERROR
} ReportKind;

typedef struct Report Report;

Report *report_new(ReportKind kind, unsigned long offset, const char *format, ...);
Report *report_new_with_args(ReportKind kind, unsigned long offset, char const *format, va_list args);
void    report_free(Report *report);
void    report_annotation(Report *report, unsigned long start, unsigned long end, char const *format, ...);
void    report_annotation_with_args(Report *report, unsigned long start, unsigned long end, char const *format, va_list args);
void    report_note(Report *report, char const *format, ...);
void    report_note_with_args(Report *report, char const *format, va_list args);
void    report_emit(Report *report, char const *filename, char const *source);

#endif /* REPORT_H */
