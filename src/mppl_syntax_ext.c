#include <stdlib.h>

#include "mppl_syntax.h"
#include "mppl_syntax_ext.h"
#include "raw_syntax_tree.h"
#include "syntax_kind.h"
#include "syntax_tree.h"
#include "type.h"
#include "utility.h"

Type *mppl_std_type__to_type(const AnyMpplStdType *syntax)
{
  switch (mppl_std_type__kind(syntax)) {
  case MPPL_STD_TYPE_BOOLEAN:
    return type_new(TYPE_BOOLEAN);

  case MPPL_STD_TYPE_CHAR:
    return type_new(TYPE_CHAR);

  case MPPL_STD_TYPE_INTEGER:
    return type_new(TYPE_INTEGER);

  default:
    unreachable();
  }
}

Type *mppl_type__to_type(const AnyMpplType *syntax)
{
  switch (mppl_type__kind(syntax)) {
  case MPPL_TYPE_STD:
    return mppl_std_type__to_type((const AnyMpplStdType *) syntax);

  case MPPL_TYPE_ARRAY: {
    const MpplArrayType *array_syntax = (const MpplArrayType *) syntax;
    AnyMpplStdType      *elem_syntax  = mppl_array_type__type(array_syntax);
    MpplLitNumber       *size_syntax  = mppl_array_type__size(array_syntax);
    Type                *elem_type    = mppl_std_type__to_type(elem_syntax);
    long                 size         = mppl_lit_number__to_long(size_syntax);

    mppl_free(size_syntax);
    mppl_free(elem_syntax);
    return type_new_array(elem_type, size);
  }

  default:
    unreachable();
  }
}

long mppl_lit_number__to_long(const MpplLitNumber *syntax)
{
  const RawSyntaxToken *token = (const RawSyntaxToken *) syntax_tree_raw((const SyntaxTree *) syntax);
  return atol(token->text);
}

char *mppl_lit_string__to_string(const MpplLitString *syntax)
{
  const RawSyntaxToken *token  = (const RawSyntaxToken *) syntax_tree_raw((const SyntaxTree *) syntax);
  char                 *result = xmalloc(token->text_length + 1);
  unsigned long         i, j;
  for (i = 0, j = 0; i < token->text_length; ++i, ++j) {
    if (token->text[i] == '\'') {
      ++i;
    }
    result[j] = token->text[i];
  }
  result[j] = '\0';
  return result;
}

int mppl_lit_boolean__to_int(const MpplLitBoolean *syntax)
{
  const RawSyntaxToken *token = (const RawSyntaxToken *) syntax_tree_raw((const SyntaxTree *) syntax);
  return token->kind == SYNTAX_TRUE_KW;
}
