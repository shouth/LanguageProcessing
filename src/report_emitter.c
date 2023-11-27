#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "report.h"
#include "source.h"
#include "utility.h"
#include "vector.h"

typedef enum {
  DISPLAY_LINE_EVENT_KIND_END,
  DISPLAY_LINE_EVENT_KIND_START,
  DISPLAY_LINE_EVENT_KIND_INPLACE
} DisplayLineEventKind;

typedef struct DisplaySegment DisplaySegment;
typedef struct DisplayLine    DisplayLine;
typedef struct ReportEmitter  ReportEmitter;

struct DisplaySegment {
  DisplayLineEventKind    kind;
  unsigned long           display_column;
  unsigned long           display_length;
  const ReportAnnotation *text_annotation;
  const ReportAnnotation *pointer_annotation;
};

struct DisplayLine {
  unsigned long number;
  char         *display_text;
  unsigned long display_text_length;
  Vector        segments;
};

struct ReportEmitter {
  const Report *report;
  const Source *source;
  unsigned long line_number;
  unsigned long column;
  unsigned long number_margin;
  unsigned long indent_width;
  Vector        lines;
};

static int compare_display_line(const void *left, const void *right)
{
  const DisplayLine *l = left;
  const DisplayLine *r = right;
  return l->number < r->number ? -1 : 1;
}

static unsigned long digits(unsigned long number)
{
  unsigned long result = 1;
  while (number > 9) {
    ++result;
    number /= 10;
  }
  return result;
}

static void display_line_deinit(DisplayLine *line)
{
  free(line->display_text);
  vector_deinit(&line->segments);
}

static void report_emitter_create_line(ReportEmitter *emitter, unsigned long number)
{
  unsigned long line_length = emitter->source->line_lengths[number];
  unsigned long line_offset = emitter->source->line_offsets[number];

  unsigned long column;
  unsigned long display_column;
  DisplayLine   line;
  unsigned long i;

  for (i = 0; i < vector_count(&emitter->lines); ++i) {
    const DisplayLine *line = vector_at(&emitter->lines, i);
    if (line->number == number) {
      return;
    }
  }

  line.number = number;
  vector_init(&line.segments, sizeof(DisplaySegment));

  line.display_text_length = 0;
  for (column = 0; column < line_length; ++column) {
    if (emitter->source->text[line_offset + column] == '\t') {
      line.display_text_length += emitter->indent_width - line.display_text_length % emitter->indent_width;
    } else {
      line.display_text_length += 1;
    }
  }

  line.display_text = xmalloc(sizeof(char) * (line.display_text_length + 1));
  display_column    = 0;
  for (column = 0; column < line_length; ++column) {
    if (emitter->source->text[line_offset + column] == '\t') {
      unsigned long i;
      unsigned long width = emitter->indent_width - display_column % emitter->indent_width;
      for (i = 0; i < width; ++i) {
        line.display_text[display_column++] = ' ';
      }
    } else {
      line.display_text[display_column++] = emitter->source->text[line_offset + column];
    }
  }
  line.display_text[line.display_text_length] = '\0';
  vector_push(&emitter->lines, &line);
}

static void report_emitter_collect_segments(ReportEmitter *emitter, DisplayLine *line)
{
  typedef struct LineEvent LineEvent;

  struct LineEvent {
    enum {
      LINE_EVENT_KIND_START,
      LINE_EVENT_KIND_END
    } kind;
    unsigned long           column;
    const ReportAnnotation *annotation;
  };

  unsigned long i;
  Vector        events;
  vector_init(&events, sizeof(LineEvent));

  for (i = 0; i < vector_count(&emitter->report->_annotations); ++i) {
    const ReportAnnotation *annotation = vector_at(&emitter->report->_annotations, i);
    SourceLocation          start, end;

    source_location(emitter->source, annotation->_start, &start);
    source_location(emitter->source, annotation->_end, &end);

    if (start.line == end.line && start.line == line->number) {
      LineEvent event;
      event.kind       = LINE_EVENT_KIND_START;
      event.column     = start.column;
      event.annotation = annotation;
      vector_push(&events, &event);
      event.kind       = LINE_EVENT_KIND_END;
      event.column     = end.column;
      event.annotation = annotation;
      vector_push(&events, &event);
    } else if (start.line == line->number) {
      LineEvent event;
      event.kind       = LINE_EVENT_KIND_START;
      event.column     = start.column;
      event.annotation = annotation;
      vector_push(&events, &event);
      event.kind       = LINE_EVENT_KIND_END;
      event.column     = ;
      event.annotation = annotation;
      vector_push(&events, &event);
    } else if (end.line == line->number) {
    }
  }
}

static void report_emitter_init(ReportEmitter *emitter, Report *report, const Source *source)
{
  SourceLocation location;
  unsigned long  i;

  emitter->report = report;
  emitter->source = source;

  source_location(source, report->_offset, &location);
  emitter->line_number = location.line;
  emitter->column      = location.column;

  emitter->indent_width = 4;

  vector_init(&emitter->lines, sizeof(DisplayLine));
  for (i = 0; i < vector_count(&report->_annotations); ++i) {
    const ReportAnnotation *annotation = vector_at(&report->_annotations, i);
    source_location(source, annotation->_start, &location);
    report_emitter_create_line(emitter, location.line);
    source_location(source, annotation->_end, &location);
    report_emitter_create_line(emitter, location.line);
  }

  for (i = 0; i < vector_count(&emitter->lines); ++i) {
    report_emitter_collect_segments(emitter, vector_at(&emitter->lines, i));
  }
  qsort(vector_data(&emitter->lines), vector_count(&emitter->lines), sizeof(DisplayLine), &compare_display_line);

  {
    const DisplayLine *back = vector_back(&emitter->lines);
    emitter->number_margin  = digits(back->number);
  }
}

static void report_emitter_deinit(ReportEmitter *emitter)
{
  unsigned long i;
  for (i = 0; i < vector_count(&emitter->lines); ++i) {
    display_line_deinit(vector_at(&emitter->lines, i));
  }
  vector_deinit(&emitter->lines);
}

static void emit_head_line(const ReportEmitter *emitter)
{
  switch (emitter->report->_kind) {
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
  fprintf(stderr, "%s\n", emitter->report->_message);
}

static void emit_location_line(const ReportEmitter *emitter)
{
  fprintf(stderr, "%*.s ╭─[%s:%lu:%lu]\n",
    (int) emitter->number_margin, "",
    emitter->source->file_name,
    emitter->line_number + 1, emitter->column + 1);
}

static void emit_source_line(const ReportEmitter *emitter, const DisplayLine *line)
{
  unsigned long current_display_column = 0;

  fprintf(stderr, "%*.s%lu │ ",
    (int) (emitter->number_margin - digits(line->number + 1)), "",
    line->number + 1);

  fprintf(stderr, "%.*s", (int) (line->display_text_length - current_display_column), line->display_text + current_display_column);
  fprintf(stderr, "\n");
}

static void emit_pointer_line(const ReportEmitter *emitter, const DisplayLine *line)
{
  fprintf(stderr, "%*.s │ ", (int) emitter->number_margin, "");

  fprintf(stderr, "\n");
}

static void emit_connector_lines(const ReportEmitter *emitter, const DisplayLine *line)
{
  fprintf(stderr, "%*.s │ ", (int) emitter->number_margin, "");

  fprintf(stderr, "\n");
}

static void emit_interest_lines(const ReportEmitter *emitter)
{
  unsigned long i;

  fprintf(stderr, "%*.s │ \n", (int) emitter->number_margin, "");

  for (i = 0; i < vector_count(&emitter->lines); ++i) {
    DisplayLine *line = vector_at(&emitter->lines, i);
    if (i > 0) {
      DisplayLine *previous_line = vector_at(&emitter->lines, i - 1);
      if (previous_line->number + 1 != line->number) {
        fprintf(stderr, "%*.s ┆ \n", (int) emitter->number_margin, "");
      }
    }

    emit_source_line(emitter, line);
    emit_pointer_line(emitter, line);
    emit_connector_lines(emitter, line);
  }

  fprintf(stderr, "%*.s │ \n", (int) emitter->number_margin, "");
}

static void emit_tail_line(const ReportEmitter *emitter)
{
  unsigned long i;
  for (i = 0; i <= emitter->number_margin; ++i) {
    fprintf(stderr, "─");
  }
  fprintf(stderr, "╯\n");
}

void report_emit(Report *report, const Source *source)
{
  ReportEmitter emitter;
  report_emitter_init(&emitter, report, source);

  emit_head_line(&emitter);
  emit_location_line(&emitter);
  emit_interest_lines(&emitter);
  emit_tail_line(&emitter);

  report_emitter_deinit(&emitter);
  report_deinit(report);
}
