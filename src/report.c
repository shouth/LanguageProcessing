/* SPDX-License-Identifier: Apache-2.0 */

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "report.h"
#include "term.h"
#include "util.h"

/* Report */

typedef struct ReportAnnotation ReportAnnotation;
typedef struct ReportNote       ReportNote;

struct ReportAnnotation {
  ListNode node;
  size_t   start_offset;
  size_t   end_offset;
  size_t   start_line;
  size_t   start_column;
  size_t   end_line;
  size_t   end_column;
  char    *message;
};

struct ReportNote {
  ListNode node;
  char    *message;
};

struct Report {
  ListNode      node;
  ReportKind    kind;
  unsigned long offset;
  char         *message;
  ListNode      annotations;
  ListNode      notes;
};

static char *vformat(char const *format, va_list args)
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

Report *report_new(ReportKind kind, unsigned long offset, char const *format, ...)
{
  va_list args;
  Report *report;
  va_start(args, format);
  report = report_new_with_args(kind, offset, format, args);
  va_end(args);
  return report;
}

void report_free(Report *report)
{
  if (report) {
    ListNode *n, *m;
    free(report->message);
    for (n = report->notes.next, m = n->next; n != &report->notes; n = m, m = m->next) {
      ReportNote *note = container_of(n, ReportNote, node);
      list_erase(n);
      free(note->message);
      free(note);
    }
    for (n = report->annotations.next, m = n->next; n != &report->annotations; n = m, m = m->next) {
      ReportAnnotation *annotation = container_of(n, ReportAnnotation, node);
      list_erase(n);
      free(annotation->message);
      free(annotation);
    }
    free(report);
  }
}

Report *report_new_with_args(ReportKind kind, unsigned long offset, char const *format, va_list args)
{
  Report *report      = xmalloc(sizeof(Report));
  report->kind        = kind;
  report->offset      = offset;
  report->message     = vformat(format, args);
  list_init(&report->annotations);
  list_init(&report->notes);
  return report;
}

void report_annotation(Report *report, unsigned long start, unsigned long end, char const *format, ...)
{
  va_list args;
  va_start(args, format);
  report_annotation_with_args(report, start, end, format, args);
  va_end(args);
}

void report_annotation_with_args(Report *report, unsigned long start, unsigned long end, char const *format, va_list args)
{
  ReportAnnotation *label = xmalloc(sizeof(ReportAnnotation));
  label->start_offset     = start;
  label->end_offset       = end;
  label->message          = format ? vformat(format, args) : NULL;
  list_push_back(&report->annotations, &label->node);
}

void report_note(Report *report, char const *format, ...)
{
  va_list args;
  va_start(args, format);
  report_note_with_args(report, format, args);
  va_end(args);
}

void report_note_with_args(Report *report, char const *format, va_list args)
{
  ReportNote *note = xmalloc(sizeof(ReportNote));
  note->message    = format ? vformat(format, args) : NULL;
  list_push_back(&report->notes, &note->node);
}

/* Report emitter */

typedef struct LineSegment LineSegment;
typedef struct Indicator   Indicator;
typedef struct Connector   Connector;
typedef struct Writer      Writer;

typedef enum {
  INDICATOR_INLINE,
  INDICATOR_END,
  INDICATOR_BEGIN
} IndicatorKind;

typedef enum {
  CONNECTOR_END,
  CONNECTOR_BEGIN
} ConnectorKind;

struct LineSegment {
  ListNode                node;
  ReportAnnotation const *annotation;
  unsigned long           start;
  unsigned long           end;
};

struct Indicator {
  ListNode                node;
  ReportAnnotation const *annotation;
  IndicatorKind           kind;
  unsigned long           column;
  unsigned long           length;
};

struct Connector {
  ListNode                node;
  ReportAnnotation const *annotation;
  ConnectorKind           kind;
  int                     multiline;
  unsigned long           column;
  unsigned long           depth;
};

struct Writer {
  Report const *report;
  char const   *filename;
  char const   *source;
  size_t        source_length;
  size_t       *offsets;
  size_t        line_count;
  int           number_margin;
  int           tab_width;
};

static int compare_line_segments(ListNode const *left, ListNode const *right)
{
  LineSegment const *l = container_of(left, LineSegment, node);
  LineSegment const *r = container_of(right, LineSegment, node);

  if (l->start != r->start) {
    return l->start < r->start ? -1 : 1;
  } else if (l->end != r->end) {
    return l->end > r->end ? -1 : 1;
  } else {
    return 0;
  }
}

static int compare_indicators(ListNode const *left, ListNode const *right)
{
  Indicator const *l = container_of(left, Indicator, node);
  Indicator const *r = container_of(right, Indicator, node);

  if (l->kind != r->kind) {
    return l->kind < r->kind ? -1 : 1;
  } else if (l->column != r->column) {
    return l->column < r->column ? -1 : 1;
  } else if (l->length != r->length) {
    return l->length > r->length ? -1 : 1;
  } else {
    return 0;
  }
}

static int compare_connectors(ListNode const *left, ListNode const *right)
{
  Connector const *l = container_of(left, Connector, node);
  Connector const *r = container_of(right, Connector, node);

  if (l->column != r->column) {
    return l->column < r->column ? -1 : 1;
  } else {
    return 0;
  }
}

static int compare_annotations(ListNode const *left, ListNode const *right)
{
  ReportAnnotation const *l = container_of(left, ReportAnnotation, node);
  ReportAnnotation const *r = container_of(right, ReportAnnotation, node);

  if (l->start_offset != r->start_offset) {
    return l->start_offset < r->start_offset ? -1 : 1;
  } else if (l->end_offset != r->end_offset) {
    return l->end_offset < r->end_offset ? -1 : 1;
  } else {
    return 0;
  }
}

static void write_head_line(Writer *writer, TermBuf *canvas)
{
  TermStyle style;

  switch (writer->report->kind) {
  case REPORT_KIND_ERROR:
    style            = term_default_style();
    style.foreground = TERM_COLOR_BRIGHT | TERM_COLOR_RED;
    style.intensity  = TERM_INTENSITY_STRONG;
    term_buf_write(canvas, &style, "[ERROR] ");
    break;

  case REPORT_KIND_WARN:
    style            = term_default_style();
    style.foreground = TERM_COLOR_BRIGHT | TERM_COLOR_YELLOW;
    style.intensity  = TERM_INTENSITY_STRONG;
    term_buf_write(canvas, &style, "[WARN] ");
    break;

  case REPORT_KIND_NOTE:
    style            = term_default_style();
    style.foreground = TERM_COLOR_BRIGHT | TERM_COLOR_CYAN;
    style.intensity  = TERM_INTENSITY_STRONG;
    term_buf_write(canvas, &style, "[NOTE] ");
    break;
  }

  style            = term_default_style();
  style.foreground = TERM_COLOR_BRIGHT | TERM_COLOR_WHITE;
  term_buf_write(canvas, &style, "%s", writer->report->message);

  term_buf_next_line(canvas);
}

static void write_location_line(Writer *writer, TermBuf *canvas)
{
  TermStyle style;

  size_t line   = fenwick_upper_bound(writer->offsets, writer->line_count, writer->report->offset);
  size_t column = writer->report->offset - fenwick_query(writer->offsets, writer->line_count, line);

  style           = term_default_style();
  style.intensity = TERM_INTENSITY_FAINT;
  term_buf_write(canvas, &style, " %*.s ╭─[", writer->number_margin, "");

  style            = term_default_style();
  style.foreground = TERM_COLOR_WHITE | TERM_COLOR_BRIGHT;
  term_buf_write(canvas, &style, "%s:%lu:%lu", writer->filename, line + 1, column + 1);

  style           = term_default_style();
  style.intensity = TERM_INTENSITY_FAINT;
  term_buf_write(canvas, &style, "]");

  term_buf_next_line(canvas);
}

static void write_annotation_left(
  Writer                 *writer,
  TermBuf                *canvas,
  unsigned long           line_number,
  unsigned long           line_column,
  ReportAnnotation const *connect,
  int                     dotted)
{
  ListNode *n;
  TermStyle style = term_default_style();

  ReportAnnotation const *strike = NULL;
  style.foreground = TERM_COLOR_BRIGHT | TERM_COLOR_RED;
  for (n = writer->report->annotations.next; n != &writer->report->annotations; n = n->next) {
    ReportAnnotation const *annotation = container_of(n, ReportAnnotation, node);
    if (annotation->start_line != annotation->end_line) {
      if (strike) {
        term_buf_write(canvas, &style, "──");
      } else if (line_number < annotation->start_line || line_number > annotation->end_line) {
        term_buf_write(canvas, &style, "  ");
      } else if (line_number == annotation->start_line) {
        if (line_column == -1ul) {
          term_buf_write(canvas, &style, "  ");
        } else if (line_column > annotation->start_column) {
          term_buf_write(canvas, &style, dotted ? "╎ " : "│ ");
        } else if (line_column < annotation->start_column) {
          term_buf_write(canvas, &style, "  ");
        } else if (annotation == connect) {
          term_buf_write(canvas, &style, "╭─");
          strike = annotation;
        } else {
          term_buf_write(canvas, &style, dotted ? "╎ " : "│ ");
        }
      } else if (line_number == annotation->end_line) {
        if (line_column == -1ul) {
          term_buf_write(canvas, &style, dotted ? "╎ " : "│ ");
        } else if (line_column < annotation->end_column) {
          term_buf_write(canvas, &style, dotted ? "╎ " : "│ ");
        } else if (line_column > annotation->end_column) {
          term_buf_write(canvas, &style, "  ");
        } else if (annotation == connect) {
          term_buf_write(canvas, &style, "╰─");
          strike = annotation;
        } else {
          term_buf_write(canvas, &style, "  ");
        }
      } else {
        term_buf_write(canvas, &style, dotted ? "╎ " : "│ ");
      }
    }
  }
}

static void write_source_line(Writer *writer, TermBuf *canvas, unsigned long line_number)
{
  size_t    i;
  ListNode *n, *m;
  TermStyle style;

  unsigned long line_width;
  char         *line;

  unsigned long line_offset;
  unsigned long column_offset;

  ListNode segments;
  ListNode nongraphics;

  size_t offset = fenwick_query(writer->offsets, writer->line_count, line_number);
  size_t length = fenwick_query(writer->offsets, writer->line_count, line_number + 1) - offset;

  list_init(&segments);
  list_init(&nongraphics);

  line_width = 0;
  for (i = 0; i < length; ++i) {
    char c = writer->source[offset + i];
    if (c == '\t') {
      line_width += writer->tab_width - (line_width % writer->tab_width);
    } else if (!is_graphic(c)) {
      line_width += strlen("\\xXX");
    } else {
      ++line_width;
    }
  }

  line        = xmalloc(line_width + 1);
  line_offset = 0;
  for (i = 0; i < length; ++i) {
    char c = writer->source[offset + i];
    if (c == '\t') {
      unsigned long adjusted_width = writer->tab_width - (line_offset % writer->tab_width);
      line_offset += sprintf(line + line_offset, "%*.s", (int) adjusted_width, "");
    } else if (!is_graphic(c)) {
      LineSegment *segment = xmalloc(sizeof(LineSegment));
      segment->annotation = NULL;

      segment->start = line_offset;
      line_offset += sprintf(line + line_offset, "\\x%X", (unsigned char) (c & 0xFF));
      segment->end = line_offset - 1;
      list_push_back(&nongraphics, &segment->node);
    } else {
      line[line_offset++] = c ? c : ' ';
    }
  }
  line[line_width] = '\0';

  for (n = writer->report->annotations.next; n != &writer->report->annotations; n = n->next) {
    ReportAnnotation const *annotation = container_of(n, ReportAnnotation, node);
    LineSegment            *segment    = xmalloc(sizeof(LineSegment));
    segment->annotation = annotation;
    if (annotation->start_line == line_number && annotation->end_line == line_number) {
      segment->start = annotation->start_column;
      segment->end   = annotation->end_column;
    } else if (annotation->start_line == line_number) {
      segment->start = annotation->start_column;
      segment->end   = line_width;
    } else if (annotation->end_line == line_number) {
      segment->start = 0;
      segment->end   = annotation->end_column;
    } else {
      continue;
    }
    list_push_back(&segments, &segment->node);
  }
  list_sort(&segments, &compare_line_segments);

  style           = term_default_style();
  style.intensity = TERM_INTENSITY_FAINT;
  term_buf_write(canvas, &style, " %*.lu │ ", writer->number_margin, line_number + 1);

  write_annotation_left(writer, canvas, line_number, -1ul, NULL, 0);
  line_offset   = term_buf_line(canvas);
  column_offset = term_buf_column(canvas);

  style            = term_default_style();
  style.foreground = TERM_COLOR_BRIGHT | TERM_COLOR_WHITE;
  term_buf_write(canvas, &style, "%s", line);

  style           = term_default_style();
  style.intensity = TERM_INTENSITY_FAINT;
  for (n = nongraphics.next; n != &nongraphics; n = n->next) {
    LineSegment const *nongraphic = container_of(n, LineSegment, node);
    term_buf_seek(canvas, line_offset, column_offset + nongraphic->start);
    term_buf_write(canvas, &style, "%.*s", (int) (nongraphic->end - nongraphic->start + 1), line + nongraphic->start);
  }

  for (n = segments.next; n != &segments; n = n->next) {
    LineSegment const *segment = container_of(n, LineSegment, node);
    term_buf_seek(canvas, line_offset, column_offset + segment->start);

    style            = term_default_style();
    style.foreground = TERM_COLOR_BRIGHT | TERM_COLOR_RED;
    term_buf_write(canvas, &style, "%.*s", (int) (segment->end - segment->start + 1), line + segment->start);

    style            = term_default_style();
    style.foreground = TERM_COLOR_BRIGHT | TERM_COLOR_RED;
    style.intensity  = TERM_INTENSITY_FAINT;
    for (m = nongraphics.next; m != &nongraphics; m = m->next) {
      LineSegment const *nongraphic = container_of(m, LineSegment, node);
      if (nongraphic->start >= segment->start && nongraphic->end <= segment->end) {
        term_buf_seek(canvas, line_offset, column_offset + nongraphic->start);
        term_buf_write(canvas, &style, "%.*s", (int) (nongraphic->end - nongraphic->start + 1), line + nongraphic->start);
      }
    }
  }
  term_buf_next_line(canvas);

  for (n = segments.next, m = n->next; n != &segments; n = m, m = m->next) {
    LineSegment *segment = container_of(n, LineSegment, node);
    list_erase(n);
    free(segment);
  }

  for (n = nongraphics.next, m = n->next; n != &nongraphics; n = m, m = m->next) {
    LineSegment *nongraphic = container_of(n, LineSegment, node);
    list_erase(n);
    free(nongraphic);
  }

  free(line);
}

static void write_indicator_line(Writer *writer, TermBuf *canvas, unsigned long line_number)
{
  size_t    i;
  ListNode *n, *m;
  TermStyle style;

  unsigned long line_offset;
  unsigned long column_offset;

  ListNode indicators;
  list_init(&indicators);

  for (n = writer->report->annotations.next; n != &writer->report->annotations; n = n->next) {
    ReportAnnotation const *annotation = container_of(n, ReportAnnotation, node);
    if (annotation->start_line == line_number && annotation->end_line == line_number) {
      Indicator *indicator  = xmalloc(sizeof(Indicator));
      indicator->annotation = annotation;
      indicator->kind       = INDICATOR_INLINE;
      indicator->column     = annotation->start_column;
      indicator->length     = annotation->end_column - annotation->start_column + 1;
      list_push_back(&indicators, &indicator->node);
    } else if (annotation->start_line == line_number) {
      Indicator *indicator  = xmalloc(sizeof(Indicator));
      indicator->annotation = annotation;
      indicator->kind       = INDICATOR_BEGIN;
      indicator->column     = annotation->start_column;
      indicator->length     = 1;
      list_push_back(&indicators, &indicator->node);
    } else if (annotation->end_line == line_number) {
      Indicator *indicator  = xmalloc(sizeof(Indicator));
      indicator->annotation = annotation;
      indicator->kind       = INDICATOR_END;
      indicator->column     = annotation->end_column;
      indicator->length     = 1;
      list_push_back(&indicators, &indicator->node);
    } else {
      continue;
    }
  }
  list_sort(&indicators, &compare_indicators);

  style           = term_default_style();
  style.intensity = TERM_INTENSITY_FAINT;
  term_buf_write(canvas, &style, " %*.s │ ", writer->number_margin, "");

  write_annotation_left(writer, canvas, line_number, -1ul, NULL, 0);
  line_offset   = term_buf_line(canvas);
  column_offset = term_buf_column(canvas);

  style            = term_default_style();
  style.foreground = TERM_COLOR_BRIGHT | TERM_COLOR_RED;
  for (n = indicators.next; n != &indicators; n = n->next) {
    Indicator const *indicator = container_of(n, Indicator, node);
    term_buf_seek(canvas, line_offset, column_offset + indicator->column);
    switch (indicator->kind) {
    case INDICATOR_INLINE:
      term_buf_write(canvas, &style, indicator->annotation->message ? "┬" : "─");
      for (i = 1; i < indicator->length; ++i) {
        term_buf_write(canvas, &style, "─");
      }
      break;

    case INDICATOR_END:
    case INDICATOR_BEGIN:
      term_buf_write(canvas, &style, "▲");
      break;
    }
  }
  term_buf_next_line(canvas);

  for (n = indicators.next, m = n->next; n != &indicators; n = m, m = m->next) {
    Indicator *indicator = container_of(n, Indicator, node);
    list_erase(n);
    free(indicator);
  }
}

static void write_annotation_lines(Writer *writer, TermBuf *canvas, unsigned long line_number)
{
  size_t    i;
  ListNode *n, *m;
  TermStyle style;

  unsigned long label_offset = 0;
  unsigned long line_offset;
  unsigned long column_offset;
  unsigned long end_line_offset;
  unsigned long depth;

  ListNode connectors;
  list_init(&connectors);

  for (n = writer->report->annotations.next; n != &writer->report->annotations; n = n->next) {
    ReportAnnotation const *annotation = container_of(n, ReportAnnotation, node);

    if (annotation->start_line == line_number && label_offset < annotation->start_column) {
      label_offset = annotation->start_column;
    }
    if (annotation->end_line == line_number && label_offset < annotation->end_column) {
      label_offset = annotation->end_column;
    }

    if (annotation->start_line == line_number && annotation->end_line == line_number) {
      Connector *connector  = xmalloc(sizeof(Connector));
      connector->annotation = annotation;
      connector->kind       = CONNECTOR_END;
      connector->multiline  = 0;
      connector->column     = annotation->start_column;
      connector->depth      = -1ul;
      list_push_back(&connectors, &connector->node);
    } else if (annotation->start_line == line_number) {
      Connector *connector  = xmalloc(sizeof(Connector));
      connector->annotation = annotation;
      connector->kind       = CONNECTOR_BEGIN;
      connector->multiline  = 1;
      connector->column     = annotation->start_column;
      connector->depth      = -1ul;
      list_push_back(&connectors, &connector->node);
    } else if (annotation->end_line == line_number) {
      Connector *connector  = xmalloc(sizeof(Connector));
      connector->annotation = annotation;
      connector->kind       = CONNECTOR_END;
      connector->multiline  = 1;
      connector->column     = annotation->end_column;
      connector->depth      = -1ul;
      list_push_back(&connectors, &connector->node);
    } else {
      continue;
    }
  }
  list_sort(&connectors, &compare_connectors);

  depth           = 0;
  style           = term_default_style();
  style.intensity = TERM_INTENSITY_FAINT;
  for (n = connectors.next; n != &connectors; n = n->next) {
    Connector *connector = container_of(n, Connector, node);
    if (connector->multiline || connector->annotation->message) {
      connector->depth = depth;
      if (depth > 0) {
        term_buf_next_line(canvas);
      }

      term_buf_write(canvas, &style, " %*.s │ ", writer->number_margin, "");
      write_annotation_left(writer, canvas, line_number, connector->column, connector->annotation, 0);
      if (depth == 0) {
        line_offset   = term_buf_line(canvas);
        column_offset = term_buf_column(canvas);
      }
      term_buf_next_line(canvas);
      term_buf_write(canvas, &style, " %*.s │ ", writer->number_margin, "");
      write_annotation_left(writer, canvas, line_number, connector->column, NULL, 0);
      depth += 2;
    }
  }
  end_line_offset = term_buf_line(canvas);

  for (n = connectors.prev; n != &connectors; n = n->prev) {
    Connector const *connector = container_of(n, Connector, node);

    style            = term_default_style();
    style.foreground = TERM_COLOR_RED | TERM_COLOR_BRIGHT;
    if (connector->depth != -1ul) {
      for (i = 0; i < connector->depth; ++i) {
        term_buf_seek(canvas, line_offset + i, column_offset + connector->column);
        term_buf_write(canvas, &style, "│");
      }
    }

    switch (connector->kind) {
    case CONNECTOR_END:
      style            = term_default_style();
      style.foreground = TERM_COLOR_RED | TERM_COLOR_BRIGHT;
      if (connector->multiline) {
        term_buf_seek(canvas, line_offset + i, column_offset);
        for (i = 0; i < connector->column; ++i) {
          term_buf_write(canvas, &style, "─");
        }
        term_buf_write(canvas, &style, connector->annotation->message ? "┴" : "╯");
      } else if (connector->annotation->message) {
        term_buf_seek(canvas, line_offset + i, column_offset + connector->column);
        term_buf_write(canvas, &style, "╰");
      }

      if (connector->annotation->message) {
        for (i = connector->column + 1; i < label_offset + 3; ++i) {
          term_buf_write(canvas, &style, "─");
        }

        style            = term_default_style();
        style.foreground = TERM_COLOR_WHITE | TERM_COLOR_BRIGHT;
        term_buf_write(canvas, &style, " %s", connector->annotation->message);
      }
      break;

    case CONNECTOR_BEGIN:
      style            = term_default_style();
      style.foreground = TERM_COLOR_RED | TERM_COLOR_BRIGHT;
      term_buf_seek(canvas, line_offset + i, column_offset);
      for (i = 0; i < connector->column; ++i) {
        term_buf_write(canvas, &style, "─");
      }
      term_buf_write(canvas, &style, "╯");
      break;
    }
  }
  term_buf_seek(canvas, end_line_offset, 0);

  for (n = connectors.next, m = n->next; n != &connectors; n = m, m = m->next) {
    Connector *connector = container_of(n, Connector, node);
    list_erase(n);
    free(connector);
  }
}

static void write_interest_lines(Writer *writer, TermBuf *canvas)
{
  size_t    i;
  ListNode *n;
  TermStyle style;

  unsigned long start_line    = -1ul;
  unsigned long end_line      = 0;
  unsigned long previous_line = -1ul;

  for (n = writer->report->annotations.next; n != &writer->report->annotations; n = n->next) {
    ReportAnnotation const *annotation = container_of(n, ReportAnnotation, node);
    if (start_line > annotation->start_line) {
      start_line = annotation->start_line;
    }
    if (end_line < annotation->end_line) {
      end_line = annotation->end_line;
    }
  }

  for (i = start_line; i <= end_line; ++i) {
    for (n = writer->report->annotations.next; n != &writer->report->annotations; n = n->next) {
      ReportAnnotation const *annotation = container_of(n, ReportAnnotation, node);
      if (i == annotation->start_line || i == annotation->end_line) {
        int dotted = previous_line != -1ul && previous_line + 1 != i;

        style           = term_default_style();
        style.intensity = TERM_INTENSITY_FAINT;
        term_buf_write(canvas, &style, " %*.s %s", writer->number_margin, "", dotted ? "╎ " : "│ ");
        write_annotation_left(writer, canvas, i, -1ul, NULL, dotted);
        term_buf_next_line(canvas);

        write_source_line(writer, canvas, i);
        write_indicator_line(writer, canvas, i);
        write_annotation_lines(writer, canvas, i);
        previous_line = i;
        break;
      }
    }
  }
}

static void write_tail_line(Writer *writer, TermBuf *canvas)
{
  int       i;
  TermStyle style;

  style           = term_default_style();
  style.intensity = TERM_INTENSITY_FAINT;
  term_buf_write(canvas, &style, "─");
  for (i = 0; i <= writer->number_margin; ++i) {
    term_buf_write(canvas, &style, "─");
  }
  term_buf_write(canvas, &style, "╯");
  term_buf_next_line(canvas);
}

static int digits(unsigned long number)
{
  int result = 1;
  while (number > 9) {
    ++result;
    number /= 10;
  }
  return result;
}

static size_t display_locate(Writer *writer, unsigned long offset, int start, size_t *out_column)
{
  size_t i;
  size_t column;
  size_t line;
  size_t line_column;
  size_t line_offset;

  if (!start) {
    --offset;
  }
  line = fenwick_upper_bound(writer->offsets, writer->line_count, offset);
  line_offset = fenwick_query(writer->offsets, writer->line_count, line);
  line_column = offset - line_offset;
  if (!start) {
    ++line_column;
  }

  column = line_column;
  for (i = 0; i < line_column; ++i) {
    char c = writer->source[line_offset + i];
    if (c == '\t') {
      column += writer->tab_width - (column % writer->tab_width);
    } else if (!is_graphic(c)) {
      column += strlen("\\xXX");
    } else {
      ++column;
    }
  }

  if (!start) {
    --column;
  }

  if (out_column) {
    *out_column = column;
  }
  return line;
}

static size_t *build_fenwick(char const *text, size_t *line_count)
{
  size_t  line = 0;
  size_t  capacity = 16;
  size_t *offsets = xmalloc(sizeof(size_t) * capacity);

  do {
    size_t line_length = strcspn(text, "\r\n");
    if (!strncmp(text, "\r\n", 2) || !strncmp(text, "\n\r", 2)) {
      ++line_length;
    }
    ++line_length;

    if (line + 1 >= capacity) {
      size_t *old_offsets = offsets;
      capacity *= 2;
      offsets = xmalloc(sizeof(size_t) * capacity);
      memcpy(offsets, old_offsets, sizeof(size_t) * line);
      free(old_offsets);
    }
    offsets[line++] = line_length;

    text += line_length;
  } while (*text);

  fenwick_construct(offsets, line + 1);
  if (line_count) {
    *line_count = line;
  }
  return offsets;
}

void report_emit(Report *report, char const *filename, char const *source)
{
  Writer    writer;
  TermBuf  *canvas = term_buf_new();
  ListNode *n;

  list_sort(&report->annotations, &compare_annotations);

  writer.report        = report;
  writer.source        = source;
  writer.filename      = filename;
  writer.offsets       = build_fenwick(source, &writer.line_count);
  writer.tab_width     = 4;
  writer.number_margin = 0;
  for (n = report->annotations.next; n != &report->annotations; n = n->next) {
    int               margin;
    ReportAnnotation *annotation = container_of(n, ReportAnnotation, node);
    annotation->start_line = display_locate(&writer, annotation->start_offset, 1, &annotation->start_column);
    annotation->end_line   = display_locate(&writer, annotation->end_offset, 0, &annotation->end_column);

    margin = digits(annotation->start_line + 1);
    if (writer.number_margin < margin) {
      writer.number_margin = margin;
    }
    margin = digits(annotation->end_line + 1);
    if (writer.number_margin < margin) {
      writer.number_margin = margin;
    }
  }

  write_head_line(&writer, canvas);
  write_location_line(&writer, canvas);
  write_interest_lines(&writer, canvas);
  write_tail_line(&writer, canvas);

  free(writer.offsets);
  term_buf_print(canvas, stderr);
  term_buf_free(canvas);
  report_free(report);
}
