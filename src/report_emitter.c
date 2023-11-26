#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "report.h"
#include "source.h"
#include "utility.h"
#include "vector.h"

typedef enum {
  DISPLAY_LABEL_KIND_END,
  DISPLAY_LABEL_KIND_START,
  DISPLAY_LABEL_KIND_INPLACE
} DisplayLabelKind;

typedef struct SourceSegment  SourceSegment;
typedef struct PointerSegment PointerSegment;
typedef struct DisplayLabel   DisplayLabel;
typedef struct DisplayLine    DisplayLine;
typedef struct ReportEmitter  ReportEmitter;

struct SourceSegment {
  unsigned long       display_column;
  unsigned long       display_length;
  const DisplayLabel *display_label;
};

struct PointerSegment {
  unsigned long       display_column;
  unsigned long       display_length;
  const DisplayLabel *display_label;
};

struct DisplayLabel {
  DisplayLabelKind   kind;
  unsigned long      display_column;
  unsigned long      display_length;
  const ReportLabel *report_label;
};

struct DisplayLine {
  unsigned long number;
  char         *display_text;
  unsigned long display_text_length;
  unsigned long indent_width;
  Vector        source_segments;
  Vector        pointer_segments;
  Vector        display_labels;
};

struct ReportEmitter {
  const Report *report;
  const Source *source;
  unsigned long line;
  unsigned long column;
  Vector        display_lines;
  unsigned long number_margin;
  unsigned long indent_width;
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

static void display_line_init(DisplayLine *line, unsigned long number, const Source *source, unsigned long indent_width)
{
  unsigned long column;
  unsigned long display_column;
  SourceLine    line_data;
  source_line(source, number, &line_data);

  line->number       = number;
  line->indent_width = indent_width;
  vector_init(&line->source_segments, sizeof(SourceSegment));
  vector_init(&line->pointer_segments, sizeof(PointerSegment));
  vector_init(&line->display_labels, sizeof(DisplayLabel));

  line->display_text_length = 0;
  for (column = 0; column < line_data.length; ++column) {
    if (source_text(source)[line_data.offset + column] == '\t') {
      line->display_text_length += indent_width - line->display_text_length % indent_width;
    } else {
      line->display_text_length += 1;
    }
  }

  line->display_text = xmalloc(sizeof(char) * (line->display_text_length + 1));
  display_column     = 0;
  for (column = 0; column < line_data.length; ++column) {
    if (source_text(source)[line_data.offset + column] == '\t') {
      unsigned long i;
      unsigned long width = indent_width - display_column % indent_width;
      for (i = 0; i < width; ++i) {
        line->display_text[display_column++] = ' ';
      }
    } else {
      line->display_text[display_column++] = source_text(source)[line_data.offset + column];
    }
  }
  line->display_text[line->display_text_length] = '\0';
}

static void display_line_deinit(DisplayLine *line)
{
  free(line->display_text);
  vector_deinit(&line->source_segments);
  vector_deinit(&line->pointer_segments);
  vector_deinit(&line->display_labels);
}

static unsigned long report_emitter_line_index(ReportEmitter *emitter, unsigned long number)
{
  unsigned long i;

  for (i = 0; i < vector_count(&emitter->display_lines); ++i) {
    DisplayLine *line = vector_at(&emitter->display_lines, i);
    if (number == line->number) {
      return i;
    }
  }

  {
    DisplayLine new_line;
    display_line_init(&new_line, number, emitter->source, emitter->indent_width);
    vector_push(&emitter->display_lines, &new_line);
    return vector_count(&emitter->display_lines) - 1;
  }
}

static void report_emitter_init(ReportEmitter *emitter, Report *report, const Source *source)
{
  unsigned long i;

  emitter->report       = report;
  emitter->source       = source;
  emitter->indent_width = 4;
  vector_init(&emitter->display_lines, sizeof(DisplayLine));

  for (i = 0; i < vector_count(&report->_labels); ++i) {
    const ReportLabel *label = vector_at(&report->_labels, i);
    unsigned long      start_line_index;
    unsigned long      end_line_index;
    SourceLocation     start;
    SourceLocation     end;

    source_location(source, label->_start, &start);
    source_location(source, label->_end, &end);

    start_line_index = report_emitter_line_index(emitter, start.line);
    end_line_index   = report_emitter_line_index(emitter, end.line);

    if (start_line_index == end_line_index) {
      DisplayLine *line = vector_at(&emitter->display_lines, start_line_index);
      display_line_label(line, source, start.column, DISPLAY_LABEL_KIND_INPLACE, label);
    } else {
      DisplayLine *start_line = vector_at(&emitter->display_lines, start_line_index);
      DisplayLine *end_line   = vector_at(&emitter->display_lines, end_line_index);
      display_line_label(start_line, source, start.column, DISPLAY_LABEL_KIND_START, label);
      display_line_label(end_line, source, end.column, DISPLAY_LABEL_KIND_END, label);
    }
  }

  qsort(vector_data(&emitter->display_lines), vector_count(&emitter->display_lines), sizeof(DisplayLine), &compare_display_line);

  {
    DisplayLine *line      = vector_back(&emitter->display_lines);
    emitter->number_margin = digits(line->number);
  }

  {
    SourceLocation location;
    source_location(source, report->_offset, &location);
    emitter->line   = location.line;
    emitter->column = location.column;
  }
}

static void report_emitter_deinit(ReportEmitter *emitter)
{
  unsigned long i;
  for (i = 0; i < vector_count(&emitter->display_lines); ++i) {
    display_line_deinit(vector_at(&emitter->display_lines, i));
  }
  vector_deinit(&emitter->display_lines);
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
    emitter->source->_file_name,
    emitter->line + 1, emitter->column + 1);
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
  unsigned long i;
  fprintf(stderr, "%*.s │ ", (int) emitter->number_margin, "");
  for (i = 0; i < vector_count(&line->pointer_segments); ++i) {
    const PointerSegment *segment = vector_at(&line->pointer_segments, i);
    if (i > 0) {
      const PointerSegment *previous     = vector_at(&line->pointer_segments, i - 1);
      unsigned long         margin_width = segment->display_column - (previous->display_column + previous->display_length);
      fprintf(stderr, "%*.s", (int) margin_width, "");
    }
    if (segment->display_label->kind == DISPLAY_LABEL_KIND_INPLACE) {
      unsigned long j = 0;
      if (segment->display_column == segment->display_label->display_column) {
        fprintf(stderr, "┬");
        ++j;
      }
      for (; j < segment->display_length; ++j) {
        fprintf(stderr, "─");
      }
    } else {
      fprintf(stderr, "▲");
    }
  }
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

  for (i = 0; i < vector_count(&emitter->display_lines); ++i) {
    DisplayLine *line = vector_at(&emitter->display_lines, i);
    if (i > 0) {
      DisplayLine *previous_line = vector_at(&emitter->display_lines, i - 1);
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
