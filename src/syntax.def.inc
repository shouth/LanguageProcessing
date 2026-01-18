#if !defined SYNTAX_DEF
#error "SYNTAX_DEF must be defined before including this file"
#else

#ifndef TOKEN
#define TOKEN(KIND)
#endif

#ifndef SEQ_BEGIN
#define SEQ_BEGIN(TYPE, KIND)
#endif

#ifndef SEQ_FIELD
#define SEQ_FIELD(KIND, FIELD_TYPE, FIELD_NAME)
#endif

#ifndef SEQ_END
#define SEQ_END(KIND)
#endif

#ifndef ALT_BEGIN
#define ALT_BEGIN(TYPE)
#endif

#ifndef ALT_FIELD
#define ALT_FIELD(FIELD_TYPE, FIELD_NAME)
#endif

#ifndef ALT_END
#define ALT_END()
#endif

#ifndef REP
#define REP(TYPE, KIND, ITEM_TYPE)
#endif

#include SYNTAX_DEF

#undef TOKEN
#undef SEQ_BEGIN
#undef SEQ_FIELD
#undef SEQ_END
#undef ALT_BEGIN
#undef ALT_FIELD
#undef ALT_END
#undef REP

#endif
