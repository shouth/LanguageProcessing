#ifndef MPPL_SYNTAX_EXT_H
#define MPPL_SYNTAX_EXT_H

#include "mppl_syntax.h"
#include "type.h"

Type *mppl_std_type__to_type(const AnyMpplStdType *syntax);

Type *mppl_type__to_type(const AnyMpplType *syntax);

long mppl_lit_number__to_long(const MpplNumberLit *syntax);

char *mppl_lit_string__to_string(const MpplStringLit *syntax);

int mppl_lit_boolean__to_int(const MpplBooleanLit *syntax);

#endif
