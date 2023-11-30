#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "array.h"
#include "report.h"
#include "source.h"
#include "utility.h"

typedef enum {
  DISPLAY_SEGMENT_KIND_EMPTY,
  DISPLAY_SEGMENT_KIND_POINTER,
  DISPLAY_SEGMENT_KIND_JOINT,
  DISPLAY_SEGMENT_KIND_UNDERLINE
} DisplaySegmentKind;

typedef struct DisplaySegment    DisplaySegment;
typedef struct DisplayLine       DisplayLine;
typedef struct DisplayAnnotation DisplayAnnotation;
typedef struct ReportEmitter     ReportEmitter;

struct DisplaySegment {
  DisplaySegmentKind       kind;
  unsigned long            display_column;
  unsigned long            display_length;
  const DisplayAnnotation *color;
  const DisplayAnnotation *designator;
};

struct DisplayLine {
  unsigned long   number;
  char           *display_text;
  unsigned long   display_text_length;
  DisplaySegment *segments;
  unsigned long   segment_count;
};

struct DisplayAnnotation {
  SourceLocation          start;
  SourceLocation          end;
  const ReportAnnotation *report_annotaion;
};

struct ReportEmitter {
  const Report      *report;
  const Source      *source;
  SourceLocation     location;
  unsigned long      number_margin;
  unsigned long      indent_width;
  DisplayAnnotation *annotaions;
  unsigned long      annotation_count;
  DisplayLine       *lines;
  unsigned long      line_count;
};

static unsigned long digits(unsigned long number)
{
  unsigned long result = 1;
  while (number > 9) {
    ++result;
    number /= 10;
  }
  return result;
}

static void display_line_init(DisplayLine *line, unsigned long number, const ReportEmitter *emitter)
{
  unsigned long line_length = emitter->source->line_lengths[number];
  unsigned long line_offset = emitter->source->line_offsets[number];

  const DisplayAnnotation *designator = NULL;

  Array         segments;
  unsigned long column;
  unsigned long display_column;
  unsigned long i;

  line->number = number;
  array_init(&segments, sizeof(DisplaySegment));

  line->display_text_length = 0;
  for (column = 0; column < line_length; ++column) {
    if (emitter->source->text[line_offset + column] == '\t') {
      line->display_text_length += emitter->indent_width - line->display_text_length % emitter->indent_width;
    } else {
      line->display_text_length += 1;
    }
  }

  line->display_text = xmalloc(sizeof(char) * (line->display_text_length + 1));
  for (display_column = 0, column = 0; column < line_length; ++column) {
    if (emitter->source->text[line_offset + column] == '\t') {
      unsigned long width = emitter->indent_width - display_column % emitter->indent_width;
      for (i = 0; i < width; ++i) {
        line->display_text[display_column++] = ' ';
      }
    } else {
      line->display_text[display_column++] = emitter->source->text[line_offset + column];
    }
  }
  line->display_text[line->display_text_length] = '\0';

  for (column = 0; column < line_length;) {
    const DisplayAnnotation *next_designator = NULL;

    DisplaySegment segment;
    segment.display_column = column;
    /* TODO: implement search for next color */
    segment.designator = designator;

    do {
      unsigned long region_start = designator ? column + 1 : column;
      unsigned long region_end   = designator ? designator->end.column : line_length;
      unsigned long i;
      unsigned long min_delta = region_end - region_start;

      if (designator) {
        if (designator->start.line == number && designator->end.line == number && designator->start.column == column) {
          segment.kind           = DISPLAY_SEGMENT_KIND_JOINT;
          segment.display_length = 1;
          break;
        } else if (designator->start.line != designator->end.line && (designator->start.column == column || designator->end.column == column)) {
          segment.kind           = DISPLAY_SEGMENT_KIND_POINTER;
          segment.display_length = 1;
          break;
        }
      }

      for (i = 0; i < emitter->annotation_count; ++i) {
        /* TODO: implement search for next designator */
      }

      segment.kind           = designator ? DISPLAY_SEGMENT_KIND_UNDERLINE : DISPLAY_SEGMENT_KIND_EMPTY;
      segment.display_length = min_delta;
    } while (0);

    if (segment.display_length > 0) {
      array_push(&segments, &segment);
    }

    column += segment.display_length;
    designator = next_designator;
  }

  line->segment_count = array_count(&segments);
  line->segments      = array_steal(&segments);
}

static void display_line_deinit(DisplayLine *line)
{
  free(line->display_text);
  free(line->segments);
}

static void report_emitter_init(ReportEmitter *emitter, Report *report, const Source *source)
{
  unsigned long i, j;
  Array         annotations;
  Array         lines;
  unsigned long start_line = -1ul;
  unsigned long end_line   = -1ul;

  emitter->report       = report;
  emitter->source       = source;
  emitter->indent_width = 4;
  source_location(source, report->_offset, &emitter->location);

  array_init(&annotations, sizeof(DisplayAnnotation));
  for (i = 0; i < array_count(&report->_annotations); ++i) {
    DisplayAnnotation annotation;
    annotation.report_annotaion = array_at(&report->_annotations, i);
    source_location(source, annotation.report_annotaion->_start, &annotation.start);
    source_location(source, annotation.report_annotaion->_end, &annotation.end);
    array_push(&annotations, &annotation);
  }
  emitter->annotation_count = array_count(&annotations);
  emitter->annotaions       = array_steal(&annotations);

  for (i = 0; i < emitter->annotation_count; ++i) {
    if (start_line == -1ul || start_line > emitter->annotaions[i].start.line) {
      start_line = emitter->annotaions[i].start.line;
    }
    if (end_line == -1ul || end_line < emitter->annotaions[i].end.line) {
      end_line = emitter->annotaions[i].end.line;
    }
  }

  array_init(&lines, sizeof(DisplayLine));
  for (i = start_line; i <= end_line; ++i) {
    for (j = 0; j < emitter->annotation_count; ++j) {
      DisplayLine line;
      if (i == emitter->annotaions[j].start.line) {
        display_line_init(&line, i, emitter);
        array_push(&lines, &line);
        break;
      }
      if (i == emitter->annotaions[j].end.line) {
        display_line_init(&line, i, emitter);
        array_push(&lines, &line);
        break;
      }
    }
  }

  emitter->line_count    = array_count(&lines);
  emitter->lines         = array_steal(&lines);
  emitter->number_margin = digits(emitter->lines[emitter->line_count - 1].number);
}

static void report_emitter_deinit(ReportEmitter *emitter)
{
  unsigned long i;
  for (i = 0; i < emitter->line_count; ++i) {
    display_line_deinit(emitter->lines + i);
  }
  free(emitter->lines);
  free(emitter->annotaions);
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
    emitter->location.line + 1, emitter->location.column + 1);
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

static void emit_designator_line(const ReportEmitter *emitter, const DisplayLine *line)
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
  for (i = 0; i < emitter->line_count; ++i) {
    if (i > 0 && emitter->lines[i - 1].number + 1 != emitter->lines[i].number) {
      fprintf(stderr, "%*.s ┆ \n", (int) emitter->number_margin, "");
    }

    emit_source_line(emitter, emitter->lines + i);
    emit_designator_line(emitter, emitter->lines + i);
    emit_connector_lines(emitter, emitter->lines + i);
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
