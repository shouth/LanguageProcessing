#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "report.h"
#include "source.h"
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

void report_deinit(Report *report)
{
  free(report->_message);
  vector_deinit(&report->_notes);
  vector_deinit(&report->_labels);
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

static unsigned long get_number_of_digits(unsigned long number)
{
  unsigned long result = 1;
  while (number > 9) {
    number /= 10;
    ++result;
  }
  return result;
}

static unsigned long get_line_number_margin(const Report *report, const Source *source)
{
  unsigned long  i;
  unsigned long  line = 0;
  SourceLocation location;

  source_location(source, report->_start, &location);
  if (line < location.line) {
    line = location.line;
  }
  source_location(source, report->_end, &location);
  if (line < location.line) {
    line = location.line;
  }

  for (i = 0; i < vector_count(&report->_labels); ++i) {
    Label *label = vector_at(&report->_labels, i);

    source_location(source, label->start, &location);
    if (line < location.line) {
      line = location.line;
    }
    source_location(source, label->end, &location);
    if (line < location.line) {
      line = location.line;
    }
  }

  return get_number_of_digits(line);
}

static unsigned long get_start_line_number(const Report *report, const Source *source)
{
  unsigned long  result = ULONG_MAX;
  SourceLocation location;
  unsigned long  i;

  source_location(source, report->_start, &location);
  if (result > location.line) {
    result = location.line;
  }

  for (i = 0; i < vector_count(&report->_labels); ++i) {
    Label *label = vector_at(&report->_labels, i);
    source_location(source, label->start, &location);
    if (result > location.line) {
      result = location.line;
    }
  }

  return result;
}

static unsigned long get_end_line_number(const Report *report, const Source *source)
{
  unsigned long  result = 0;
  SourceLocation location;
  unsigned long  i;

  source_location(source, report->_end, &location);
  if (result < location.line) {
    result = location.line;
  }

  for (i = 0; i < vector_count(&report->_labels); ++i) {
    Label *label = vector_at(&report->_labels, i);
    source_location(source, label->end, &location);
    if (result < location.line) {
      result = location.line;
    }
  }

  return result;
}

static int is_line_skippable(const Report *report, const Source *source, unsigned long line)
{
  SourceLocation location;
  unsigned long  i;

  source_location(source, report->_start, &location);
  if (location.line == line) {
    return 0;
  }
  source_location(source, report->_end, &location);
  if (location.line == line) {
    return 0;
  }

  for (i = 0; i < vector_count(&report->_labels); ++i) {
    Label *label = vector_at(&report->_labels, i);
    source_location(source, label->start, &location);
    if (location.line == line) {
      return 0;
    }
    source_location(source, label->end, &location);
    if (location.line == line) {
      return 0;
    }
  }

  return 1;
}

static void print_header_line(const Report *report)
{
  switch (report->_kind) {
  case REPORT_KIND_ERROR:
    fprintf(stderr, "[ERROR] ");
    break;
  case REPORT_KIND_WARN:
    fprintf(stderr, "[WARN] ");
    break;
  case REPORT_KIND_NOTE:
    fprintf(stderr, "[NOTE] ");
    break;
  }
  fprintf(stderr, "%s\n", report->_message);
}

static void print_location_line(const Report *report, const Source *source, unsigned long margin)
{
  SourceLocation location;
  source_location(source, report->_start, &location);
  printf("%*.s ╭─[%s:%lu:%lu]\n", (int) margin, "", source->_file_name, location.line, location.column);
}

static void print_empty_body_line(unsigned long margin)
{
  printf("%*.s │\n", (int) margin, "");
}

static void print_skipped_body_line(const Report *report, const Source *source, unsigned long margin, unsigned long line_number)
{
  printf("%*.s ┆\n", (int) margin, "");
}

static void print_body_line(const Report *report, const Source *source, unsigned long margin, unsigned long line_number)
{
  printf("%*.s%lu │ ", (int) (margin - get_number_of_digits(line_number)), "", line_number);
  printf("\n");
}

static void print_tail_line(unsigned long margin)
{
  unsigned long i;
  for (i = 0; i <= margin; ++i) {
    printf("─");
  }
  printf("╯\n");
}

void report_emit(Report *report, const Source *source)
{
  unsigned long line_number_margin     = get_line_number_margin(report, source);
  unsigned long start_line_number      = get_start_line_number(report, source);
  unsigned long end_line_number        = get_end_line_number(report, source);
  unsigned long last_print_line_number = start_line_number;
  unsigned long i;

  print_header_line(report);
  print_location_line(report, source, line_number_margin);
  print_empty_body_line(line_number_margin);

  for (i = start_line_number; i <= end_line_number; ++i) {
    if (is_line_skippable(report, source, i)) {
      if (last_print_line_number + 1 == i) {
        print_skipped_body_line(report, source, line_number_margin, i);
      }
    } else {
      print_body_line(report, source, line_number_margin, i);
      last_print_line_number = i;
    }
  }

  print_empty_body_line(line_number_margin);
  print_tail_line(line_number_margin);
  report_deinit(report);
}
