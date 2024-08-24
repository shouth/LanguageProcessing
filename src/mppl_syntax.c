/*
   Copyright 2022 Shota Minami

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#include <string.h>

#include "mppl_syntax.h"
#include "utility.h"

MpplSyntaxKind mppl_syntax_kind_from_keyword(const char *string, unsigned long size)
{
#define KEYWORD_KIND_PAIR(keyword, name) { keyword, MPPL_SYNTAX_##name },

#define LIST_KEYWORD_KIND_PAIR(name, kind, keyword) \
  META_IF(META_DETECT(PROBE_##kind), META_DEFER(KEYWORD_KIND_PAIR)(keyword, name), META_DEFER(META_EMPTY)())

#define PROBE_KEYWORD META_DETECT_PROBE

  unsigned long i;

  static const struct {
    const char    *keyword;
    MpplSyntaxKind kind;
  } keywords[] = {
    MPPL_SYNTAX_FOR_EACH(LIST_KEYWORD_KIND_PAIR)
  };

  for (i = 0; i < sizeof(keywords) / sizeof(*keywords); ++i) {
    if (strncmp(keywords[i].keyword, string, size) == 0 && !keywords[i].keyword[size]) {
      return keywords[i].kind;
    }
  }

  return MPPL_SYNTAX_ERROR;

#undef PROBE_KEYWORD
#undef LIST_KEYWORD_KIND_PAIR
#undef KEYWORD_KIND_PAIR
}

int mppl_syntax_kind_is_token(MpplSyntaxKind kind)
{
  return kind <= MPPL_SYNTAX_EOF_TRIVIA;
}

int mppl_syntax_kind_is_trivia(MpplSyntaxKind kind)
{
  return kind >= MPPL_SYNTAX_SPACE_TRIVIA && kind <= MPPL_SYNTAX_EOF_TRIVIA;
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
