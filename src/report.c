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

typedef enum {
  EVENT_KIND_START,
  EVENT_KIND_END,
  EVENT_KIND_INLINE
} EventKind;

typedef struct Event Event;

struct Event {
  EventKind      kind;
  SourceLocation location[2];
  const Label   *label;
};

static int compare_events(const void *left, const void *right)
{
  const Event *l = left;
  const Event *r = right;

  if (l->location->line != r->location->line) {
    return l->location->line < r->location->line ? -1 : 1;
  } else if (l->location->column != r->location->column) {
    return l->location->column < r->location->column ? -1 : 1;
  } else {
    return 0;
  }
}

static void label_events(const Report *report, const Source *source, Vector *events)
{
  unsigned long i;
  vector_init(events, sizeof(Event));
  for (i = 0; i < vector_count(&report->_labels); ++i) {
    Event          event;
    SourceLocation start, end;
    event.label = vector_at(&report->_labels, i);
    source_location(source, event.label->start, &start);
    source_location(source, event.label->end, &end);

    if (start.line != end.line) {
      event.kind        = EVENT_KIND_START;
      event.location[0] = start;
      vector_push(events, &event);

      event.kind        = EVENT_KIND_END;
      event.location[0] = end;
      vector_push(events, &event);
    } else {
      event.kind        = EVENT_KIND_INLINE;
      event.location[0] = start;
      event.location[1] = end;
      vector_push(events, &event);
    }
  }
  qsort(vector_data(events), vector_count(events), sizeof(Event), &compare_events);
}

static void print_header(const Report *report)
{
  switch (report->_kind) {
  case REPORT_KIND_NOTE:
    fprintf(stderr, "[NOTE] ");
    break;
  case REPORT_KIND_WARN:
    fprintf(stderr, "[WARN] ");
    break;
  case REPORT_KIND_ERROR:
    fprintf(stderr, "[ERROR] ");
    break;
  }
  fprintf(stderr, "%s\n", report->_message);
}

static unsigned long line_number_margin(Vector *events)
{
  Event        *back   = vector_back(events);
  unsigned long line   = back->location->line;
  unsigned long result = 1;
  while (line > 9) {
    line /= 10;
    ++result;
  }
  return result;
}

static unsigned long arrow_margin(Vector *events)
{
  unsigned long i;
  unsigned long result;
  for (i = 0; i < vector_count(events); ++i) {
    Event        *event = vector_at(events, i);
    unsigned long line  = event->location->line;
    while (1) {
      
    }
  }
  return result * 2 + 1;
}

void report_emit(Report *report, const Source *source)
{
  Vector events;
  label_events(report, source, &events);

  vector_deinit(&events);
  report_deinit(report);
}
