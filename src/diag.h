/* SPDX-License-Identifier: Apache-2.0 */

#ifndef DIAG_H
#define DIAG_H

#include "mppl_syntax.h"
#include "report.h"

/* lexer */

Report *diag_stray_char_error(unsigned long offset, int stray, MpplTokenKindSet expected);
Report *diag_nongraphic_char_error(unsigned long offset, int nongraphic);
Report *diag_unterminated_string_error(unsigned long offset, unsigned long length);
Report *diag_unterminated_comment_error(unsigned long offset, unsigned long length);
Report *diag_too_big_number_error(unsigned long offset, unsigned long length);

/* parser */

Report *diag_unexpected_token_error(unsigned long offset, unsigned long length, const char *found, MpplTokenKindSet expected);
Report *diag_expected_expression_error(unsigned long offset, unsigned long length);
Report *diag_missing_semicolon_error(unsigned long offset);
Report *diag_break_outside_loop_error(unsigned long offset, unsigned long length);

#endif /* DIAGNOSTICS_H */
