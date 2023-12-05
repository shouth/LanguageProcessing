#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "array.h"
#include "report.h"
#include "source.h"
#include "utility.h"

typedef struct DisplaySegment    DisplaySegment;
typedef struct DisplayLine       DisplayLine;
typedef struct DisplayAnnotation DisplayAnnotation;
typedef struct ReportEmitter     ReportEmitter;

struct DisplaySegment {
  unsigned long            start;
  unsigned long            end;
  const DisplayAnnotation *annotaion;
  DisplaySegment          *next;
};

struct DisplayLine {
  unsigned long   number;
  char           *display_text;
  unsigned long   display_text_length;
  DisplaySegment *color_segments;
  DisplaySegment *designator_segments;
};

struct DisplayAnnotation {
  SourceLocation          start;
  SourceLocation          end;
  const ReportAnnotation *annotaion;
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

static int display_segment_compare(const void *left, const void *right)
{
  const DisplaySegment *l = left;
  const DisplaySegment *r = right;

  if (l->start != r->start) {
    return l->start < r->start ? -1 : 1;
  } else if (l->end != r->end) {
    return l->end < r->end ? -1 : 1;
  } else if (l->annotaion->annotaion->_start != r->annotaion->annotaion->_start) {
    return l->annotaion->annotaion->_start < r->annotaion->annotaion->_start ? -1 : 1;
  } else if (l->annotaion->annotaion->_end != r->annotaion->annotaion->_end) {
    return l->annotaion->annotaion->_end < r->annotaion->annotaion->_end ? -1 : 1;
  } else {
    return l->annotaion->annotaion < r->annotaion->annotaion ? -1 : 1;
  }
}

static void display_segment_serialize(DisplaySegment *segments, unsigned long count)
{
  unsigned long   i;
  DisplaySegment *list = NULL;

  qsort(segments, count, sizeof(DisplaySegment), &display_segment_compare);
  for (i = 0; i < count; ++i) {
    DisplaySegment  *segment = segments + i;
    DisplaySegment **tail    = &list;

    while (*tail && segment->start < (*tail)->start) {
      tail = &(*tail)->next;
    }

    *tail = segment;
    while (*tail && segment->end < (*tail)->start) {
      tail = &(*tail)->next;
    }

    if (*tail) {
      (*tail)->start = segment->end;
    }
    segment->next = *tail;
  }
}

static void display_line_init(DisplayLine *line, unsigned long number, const ReportEmitter *emitter)
{
  unsigned long line_length = emitter->source->line_lengths[number];
  unsigned long line_offset = emitter->source->line_offsets[number];

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

  {
    Array color_segments;
    array_init(&color_segments, sizeof(DisplaySegment));
    for (i = 0; i < emitter->annotation_count; ++i) {
      int is_start = emitter->annotaions[i].start.line == number;
      int is_end   = emitter->annotaions[i].end.line == number;

      if (is_start && is_end) {
        DisplaySegment segment;
        segment.start     = emitter->annotaions[i].start.column;
        segment.end       = emitter->annotaions[i].end.column;
        segment.annotaion = emitter->annotaions + i;
        segment.next      = NULL;
        array_push(&color_segments, &segment);
      } else if (is_start) {
        DisplaySegment segment;
        segment.start     = emitter->annotaions[i].start.column;
        segment.end       = segment.start + 1;
        segment.annotaion = emitter->annotaions + i;
        segment.next      = NULL;
        array_push(&color_segments, &segment);
      } else if (is_end) {
        DisplaySegment segment;
        segment.start     = emitter->annotaions[i].end.column;
        segment.end       = segment.start + 1;
        segment.annotaion = emitter->annotaions + i;
        segment.next      = NULL;
        array_push(&color_segments, &segment);
      }
    }
    display_segment_serialize(array_data(&color_segments), array_count(&color_segments));
    line->color_segments = array_steal(&color_segments);
  }

  {
    Array designator_segments;
    array_init(&designator_segments, sizeof(DisplaySegment));
    for (i = 0; i < emitter->annotation_count; ++i) {
      int is_start = emitter->annotaions[i].start.line == number;
      int is_end   = emitter->annotaions[i].end.line == number;

      if (is_start && is_end) {
        DisplaySegment segment;
        segment.start     = emitter->annotaions[i].start.column;
        segment.end       = emitter->annotaions[i].end.column + 1;
        segment.annotaion = emitter->annotaions + i;
        segment.next      = NULL;
        array_push(&designator_segments, &segment);
      } else if (is_start) {
        DisplaySegment segment;
        segment.start     = 0;
        segment.end       = emitter->annotaions[i].end.column + 1;
        segment.annotaion = emitter->annotaions + i;
        segment.next      = NULL;
        array_push(&designator_segments, &segment);
      } else if (is_end) {
        DisplaySegment segment;
        segment.start     = emitter->annotaions[i].start.column;
        segment.end       = line_length;
        segment.annotaion = emitter->annotaions + i;
        segment.next      = NULL;
        array_push(&designator_segments, &segment);
      }
    }
    display_segment_serialize(array_data(&designator_segments), array_count(&designator_segments));
    line->designator_segments = array_steal(&designator_segments);
  }
}

static void display_line_deinit(DisplayLine *line)
{
  free(line->display_text);
  free(line->color_segments);
  free(line->designator_segments);
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
    annotation.annotaion = array_at(&report->_annotations, i);
    source_location(source, annotation.annotaion->_start, &annotation.start);
    source_location(source, annotation.annotaion->_end - 1, &annotation.end);
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
