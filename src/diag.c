/* SPDX-License-Identifier: Apache-2.0 */

#include <string.h>

#include "diag.h"
#include "mppl_syntax.h"
#include "report.h"
#include "stdio.h"
#include "util.h"

/* utility */

char *expected_set_to_string(const MpplTokenKindSet *expected)
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
  if (count > 2) {
    length += fprintf(buffer, "one of ");
  }

  for (kind = MPPL_BEGIN_TOKEN; kind < MPPL_END_TOKEN; kind++) {
    if (bitset_get(expected, kind)) {
      const char *lexeme = mppl_syntax_kind_static_lexeme(kind - MPPL_BEGIN_TOKEN);

      if (lexeme) {
        length += fprintf(buffer, "`%s`", lexeme);
      } else {
        switch (kind) {
        case MPPL_SYNTAX_EOF:
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

        default:
          unreachable();
        }
      }

      if (count > 2) {
        length += fprintf(buffer, ", ");
      } else if (count == 2) {
        length += fprintf(buffer, " or ");
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

Report *diag_stray_char_error(unsigned long offset, int stray, MpplTokenKindSet expected)
{
  char   *str = expected_set_to_string(&expected);
  Report *report;
  if (is_graphic(stray)) {
    report = report_new(REPORT_KIND_ERROR, offset, "stray `%c` in program", (char) stray);
  } else {
    report = report_new(REPORT_KIND_ERROR, offset, "stray `\\x%X` in program", (unsigned char) stray);
  }
  report_annotation(report, offset, offset + 1, "expected %s", str);
  free(str);
  return report;
}

Report *diag_nongraphic_char_error(unsigned long offset, int nongraphic)
{
  Report *report = report_new(REPORT_KIND_ERROR, offset, "non-graphic character `\\x%X` in string", (unsigned char) nongraphic);
  report_annotation(report, offset, offset + 1, NULL);
  return report;
}

Report *diag_unterminated_string_error(unsigned long offset, unsigned long length)
{
  Report *report = report_new(REPORT_KIND_ERROR, offset, "string is unterminated");
  report_annotation(report, offset, offset + length, NULL);
  return report;
}

Report *diag_unterminated_comment_error(unsigned long offset, unsigned long length)
{
  Report *report = report_new(REPORT_KIND_ERROR, offset, "comment is unterminated");
  report_annotation(report, offset, offset + length, NULL);
  return report;
}

Report *diag_too_big_number_error(unsigned long offset, unsigned long length)
{
  Report *report = report_new(REPORT_KIND_ERROR, offset, "number is too big");
  report_annotation(report, offset, offset + length, "numbers need to be less than 32768");
  return report;
}

/* parser */

Report *diag_unexpected_token_error(unsigned long offset, unsigned long length, const char *found, MpplTokenKindSet expected)
{
  char   *str = expected_set_to_string(&expected);
  Report *report;
  if (found) {
    report = report_new(REPORT_KIND_ERROR, offset, "expected %s, found `%.*s`", str, (int) length, found);
    report_annotation(report, offset, offset + length, NULL);
  } else {
    report = report_new(REPORT_KIND_ERROR, offset, "expected %s, found end of file", str);
    report_annotation(report, offset, offset + 1, NULL);
  }
  free(str);
  return report;
}

Report *diag_expected_expression_error(unsigned long offset, unsigned long length)
{
  Report *report = report_new(REPORT_KIND_ERROR, offset, "expected expression");
  report_annotation(report, offset, offset + length, NULL);
  return report;
}

Report *diag_missing_semicolon_error(unsigned long offset)
{
  Report *report = report_new(REPORT_KIND_ERROR, offset, "missing semicolon");
  report_annotation(report, offset, offset + 1, NULL);
  return report;
}

Report *diag_break_outside_loop_error(unsigned long offset, unsigned long length)
{
  Report *report = report_new(REPORT_KIND_ERROR, offset, "`break` statement outside loop");
  report_annotation(report, offset, offset + length, NULL);
  return report;
}

/* resolver */

Report *diag_multiple_definition_error(unsigned long offset, unsigned long length, const char *name, unsigned long previous_offset)
{
  Report *report = report_new(REPORT_KIND_ERROR, offset, "`%.*s` has multiple definitions", (int) length, name);
  report_annotation(report, offset, offset + length, "current definition");
  report_annotation(report, previous_offset, previous_offset + length, "previous definition");
  return report;
}

Report *diag_not_found_error(unsigned long offset, unsigned long length, const char *name)
{
  Report *report = report_new(REPORT_KIND_ERROR, offset, "`%.*s` is not defined", (int) length, name);
  report_annotation(report, offset, offset + length, NULL);
  return report;
}
