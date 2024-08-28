#include <string.h>

#include "diagnostics.h"
#include "mppl_syntax.h"
#include "report.h"
#include "stdio.h"
#include "utility.h"

typedef Report *(DiagToReport) (const Diag *diag);
typedef void(DiagFree)(Diag *diag);

struct Diag {
  DiagToReport *to_report;
  DiagFree     *free;
};

static Diag diag(DiagToReport *to_report, DiagFree *free)
{
  Diag d;
  d.to_report = to_report;
  d.free      = free;
  return d;
}

void diag_free(Diag *diag)
{
  if (diag && diag->free) {
    diag->free(diag);
  } else {
    free(diag);
  }
}

Report *diag_to_report(const Diag *diag)
{
  return diag->to_report(diag);
}

/* utility */

char *expected_set_to_string(const MpplSyntaxKindSet *expected)
{
  MpplSyntaxKind kind;
  unsigned long  count;
  unsigned long  length;
  char          *result;

  FILE *buffer = tmpfile();
  if (buffer == NULL) {
    return NULL;
  }

  count  = bitset_count(expected);
  length = 0;
  if (count > 1) {
    length += fprintf(buffer, "one of ");
  }

  for (kind = 0; kind < SENTINEL_MPPL_SYNTAX; kind++) {
    if (bitset_get(expected, kind)) {
#define PROBE_PUNCT   META_DETECT_PROBE
#define PROBE_KEYWORD META_DETECT_PROBE
#define PRINT_QUOTED(name, string)             \
  case MPPL_SYNTAX_##name:                     \
    length += fprintf(buffer, "`%s`", string); \
    break;
#define F(name, kind, string) META_DETECT(PROBE_##kind)(PRINT_QUOTED(name, string), META_EMPTY())

      switch (kind) {
      case MPPL_SYNTAX_END_OF_FILE:
        length += fprintf(buffer, "end of file");
        break;
      case MPPL_SYNTAX_NUMBER_LIT:
        length += fprintf(buffer, "number");
        break;
      case MPPL_SYNTAX_STRING_LIT:
        length += fprintf(buffer, "string");
        break;
      case MPPL_SYNTAX_IDENT_TOKEN:
        length += fprintf(buffer, "identifier");
        break;

        MPPL_SYNTAX_FOR_EACH(F)

      default:
        unreachable();
      }

#undef F
#undef PRINT_QUOTED
#undef PROBE_KEYWORD
#undef PROBE_PUNCT

      if (count > 2) {
        length += fprintf(buffer, ", ");
      } else if (count == 2) {
        length += fprintf(buffer, " and ");
      }
      --count;
    }
  }

  result = xmalloc(length + 1);
  rewind(buffer);
  fread(result, 1, length, buffer);
  result[length] = '\0';
  fclose(buffer);
  return result;
}

/* lexer */

typedef struct StrayCharError StrayCharError;

struct StrayCharError {
  Diag              interface;
  unsigned long     offset;
  int               stray;
  MpplSyntaxKindSet expected;
};

static Report *report_stray_char_error(const Diag *diag)
{
  const StrayCharError *e = (const StrayCharError *) diag;

  char   *expected = expected_set_to_string(&e->expected);
  Report *report;
  if (is_graphic(e->stray)) {
    report = report_new(REPORT_KIND_ERROR, e->offset, "stray `%c` in program", (char) e->stray);
  } else {
    report = report_new(REPORT_KIND_ERROR, e->offset, "stray `\\x%X` in program", (unsigned char) e->stray);
  }
  report_annotation(report, e->offset, e->offset + 1, "expected %s", expected);
  free(expected);
  return report;
}

Diag *diag_stray_char_error(unsigned long offset, int stray, MpplSyntaxKindSet expected)
{
  StrayCharError *e = xmalloc(sizeof(StrayCharError));
  e->interface      = diag(&report_stray_char_error, NULL);
  e->offset         = offset;
  e->stray          = stray;
  e->expected       = expected;
  return (Diag *) e;
}

typedef struct NonGraphicCharError NonGraphicCharError;

struct NonGraphicCharError {
  Diag          interface;
  unsigned long offset;
  int           nongraphic;
};

static Report *report_nongraphic_char_error(const Diag *diag)
{
  const NonGraphicCharError *e = (const NonGraphicCharError *) diag;

  Report *report = report_new(REPORT_KIND_ERROR, e->offset, "non-graphic character `\\x%X` in string", (unsigned char) e->nongraphic);
  report_annotation(report, e->offset, e->offset + 1, NULL);
  return report;
}

Diag *diag_nongraphic_char_error(unsigned long offset, int nongraphic)
{
  NonGraphicCharError *e = xmalloc(sizeof(NonGraphicCharError));
  e->interface           = diag(&report_nongraphic_char_error, NULL);
  e->nongraphic          = nongraphic;
  e->offset              = offset;
  return (Diag *) e;
}

typedef struct UnterminatedStringError UnterminatedStringError;

struct UnterminatedStringError {
  Diag          interface;
  unsigned long offset;
  unsigned long length;
};

static Report *report_unterminated_string(const Diag *diag)
{
  const UnterminatedStringError *e = (const UnterminatedStringError *) diag;

  Report *report = report_new(REPORT_KIND_ERROR, e->offset, "string is unterminated");
  report_annotation(report, e->offset, e->offset + e->length, NULL);
  return report;
}

Diag *diag_unterminated_string_error(unsigned long offset, unsigned long length)
{
  UnterminatedStringError *e = xmalloc(sizeof(UnterminatedStringError));
  e->interface               = diag(&report_unterminated_string, NULL);
  e->offset                  = offset;
  e->length                  = length;
  return (Diag *) e;
}

typedef struct UnterminatedCommentError UnterminatedCommentError;

struct UnterminatedCommentError {
  Diag          interface;
  unsigned long offset;
  unsigned long length;
};

static Report *report_unterminated_comment_error(const Diag *diagnostics)
{
  const UnterminatedCommentError *e = (const UnterminatedCommentError *) diagnostics;

  Report *report = report_new(REPORT_KIND_ERROR, e->offset, "comment is unterminated");
  report_annotation(report, e->offset, e->offset + e->length, NULL);
  return report;
}

Diag *diag_unterminated_comment_error(unsigned long offset, unsigned long length)
{
  UnterminatedCommentError *e = xmalloc(sizeof(UnterminatedCommentError));
  e->interface                = diag(&report_unterminated_comment_error, NULL);
  e->offset                   = offset;
  e->length                   = length;
  return (Diag *) e;
}

typedef struct TooBigNumberError TooBigNumberError;

struct TooBigNumberError {
  Diag          interface;
  unsigned long offset;
  unsigned long length;
};

static Report *report_too_big_number_error(const Diag *diag)
{
  const TooBigNumberError *e = (const TooBigNumberError *) diag;

  Report *report = report_new(REPORT_KIND_ERROR, e->offset, "number is too big");
  report_annotation(report, e->offset, e->offset + e->length, "numbers need to be less than 32768");
  return report;
}

Diag *diag_too_big_number_error(unsigned long offset, unsigned long length)
{
  TooBigNumberError *e = xmalloc(sizeof(TooBigNumberError));
  e->interface         = diag(&report_too_big_number_error, NULL);
  e->offset            = offset;
  e->length            = length;
  return (Diag *) e;
}

/* parser */

typedef struct UnexpectedTokenError UnexpectedTokenError;

struct UnexpectedTokenError {
  Diag              interface;
  unsigned long     offset;
  unsigned long     length;
  char             *found;
  MpplSyntaxKindSet expected;
};

static Report *report_diag_unexpected_token_error(const Diag *diag)
{
  const UnexpectedTokenError *e = (const UnexpectedTokenError *) diag;

  char   *expected = expected_set_to_string(&e->expected);
  Report *report;
  if (e->found) {
    report = report_new(REPORT_KIND_ERROR, e->offset, "expected %s, found `%s`", expected, e->found);
    report_annotation(report, e->offset, e->offset + e->length, NULL);
  } else {
    report = report_new(REPORT_KIND_ERROR, e->offset, "expected %s, found end of file", expected);
    report_annotation(report, e->offset, e->offset + 1, NULL);
  }
  free(expected);
  return report;
}

static void free_unexpected_token_error(Diag *diag)
{
  if (diag) {
    UnexpectedTokenError *e = (UnexpectedTokenError *) diag;
    free(e->found);
    free(e);
  }
}

Diag *diag_unexpected_token_error(unsigned long offset, unsigned long length, char *found, MpplSyntaxKindSet expected)
{
  UnexpectedTokenError *e = xmalloc(sizeof(UnexpectedTokenError));
  e->interface            = diag(&report_diag_unexpected_token_error, &free_unexpected_token_error);
  e->offset               = offset;
  e->length               = length;
  e->found                = found;
  e->expected             = expected;
  return (Diag *) e;
}

typedef struct ExpectedExpressionError ExpectedExpressionError;

struct ExpectedExpressionError {
  Diag          interface;
  unsigned long offset;
  unsigned long length;
};

static Report *report_expected_expression_error(const Diag *diag)
{
  const ExpectedExpressionError *e = (const ExpectedExpressionError *) diag;

  Report *report = report_new(REPORT_KIND_ERROR, e->offset, "expected expression");
  report_annotation(report, e->offset, e->offset + e->length, NULL);
  return report;
}

Diag *diag_expected_expression_error(unsigned long offset, unsigned long length)
{
  ExpectedExpressionError *e = xmalloc(sizeof(ExpectedExpressionError));
  e->interface               = diag(&report_expected_expression_error, NULL);
  e->offset                  = offset;
  e->length                  = length;
  return (Diag *) e;
}

typedef struct MissingSemicolonError MissingSemicolonError;

struct MissingSemicolonError {
  Diag          interface;
  unsigned long offset;
};

static Report *report_missing_semicolon_error(const Diag *diag)
{
  const MissingSemicolonError *e = (const MissingSemicolonError *) diag;

  Report *report = report_new(REPORT_KIND_ERROR, e->offset, "missing semicolon");
  report_annotation(report, e->offset, e->offset + 1, NULL);
  return report;
}

Diag *diag_missing_semicolon_error(unsigned long offset)
{
  MissingSemicolonError *e = xmalloc(sizeof(MissingSemicolonError));
  e->interface             = diag(&report_missing_semicolon_error, NULL);
  e->offset                = offset;
  return (Diag *) e;
}

typedef struct BreakOutsideLoopError BreakOutsideLoopError;

struct BreakOutsideLoopError {
  Diag          interface;
  unsigned long offset;
  unsigned long length;
};

static Report *report_break_outside_loop_error(const Diag *diag)
{
  const BreakOutsideLoopError *e = (const BreakOutsideLoopError *) diag;

  Report *report = report_new(REPORT_KIND_ERROR, e->offset, "`break` statement outside loop");
  report_annotation(report, e->offset, e->offset + e->length, NULL);
  return report;
}

Diag *diag_break_outside_loop_error(unsigned long offset, unsigned long length)
{
  BreakOutsideLoopError *e = xmalloc(sizeof(BreakOutsideLoopError));
  e->interface             = diag(&report_break_outside_loop_error, NULL);
  e->offset                = offset;
  e->length                = length;
  return (Diag *) e;
}
