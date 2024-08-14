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

#include "mppl_syntax_kind.h"
#include "utility.h"

MpplSyntaxKind mppl_syntax_kind_from_keyword(const char *string, unsigned long size)
{

#define KEYWORD_SOME(str) \
  if (strncmp())

#define F(name, kind, string)

#undef F
  return MPPL_SYNTAX_ERROR;
}

int mppl_syntax_kind_is_token(MpplSyntaxKind kind)
{
  return kind <= MPPL_SYNTAX_EOF_TOKEN;
}

int mppl_syntax_kind_is_trivia(MpplSyntaxKind kind)
{
  return kind >= MPPL_SYNTAX_SPACE_TRIVIA && kind <= MPPL_SYNTAX_C_COMMENT_TRIVIA;
}

const char *mppl_syntax_kind_to_string(MpplSyntaxKind kind)
{
#define F(name, kind, string) \
  case MPPL_SYNTAX_##name:    \
    return "MPPL_SYNTAX_" #name;

  switch (kind) {
    MPPL_SYNTAX_FOR_EACH(F)

  default:
    unreachable();
  }

#undef F
}
