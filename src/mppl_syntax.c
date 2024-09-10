/* SPDX-License-Identifier: Apache-2.0 */

#include <string.h>

#include "mppl_syntax.h"
#include "utility.h"

MpplSyntaxKind mppl_syntax_kind_from_keyword(const char *string, unsigned long size)
{
#define PROBE_KEYWORD META_DETECT_PROBE
#define KEYWORD_KIND_PAIR(keyword, name) { keyword, MPPL_SYNTAX_##name },
#define F(name, kind, keyword) \
  META_DETECT(PROBE_##kind)(KEYWORD_KIND_PAIR(keyword, name), META_EMPTY())

  unsigned long i;

  static const struct {
    const char    *keyword;
    MpplSyntaxKind kind;
  } keyword_to_kind[] = {
    MPPL_SYNTAX_FOR_EACH(F)
  };

  for (i = 0; i < count_of(keyword_to_kind); ++i) {
    if (strncmp(keyword_to_kind[i].keyword, string, size) == 0 && !keyword_to_kind[i].keyword[size]) {
      return keyword_to_kind[i].kind;
    }
  }

  return MPPL_SYNTAX_ERROR;

#undef F
#undef KEYWORD_KIND_PAIR
#undef PROBE_KEYWORD
}

int mppl_syntax_kind_is_token(MpplSyntaxKind kind)
{
  return kind <= MPPL_SYNTAX_C_COMMENT_TRIVIA;
}

int mppl_syntax_kind_is_trivia(MpplSyntaxKind kind)
{
  return kind >= MPPL_SYNTAX_SPACE_TRIVIA && kind <= MPPL_SYNTAX_C_COMMENT_TRIVIA;
}

const char *mppl_syntax_kind_to_string(MpplSyntaxKind kind)
{
#define KIND_TO_STRING(name, kind, string) \
  case MPPL_SYNTAX_##name:                 \
    return #name;

  switch (kind) {
    MPPL_SYNTAX_FOR_EACH(KIND_TO_STRING)

  default:
    unreachable();
  }

#undef KIND_TO_STRING
}
