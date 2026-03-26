/* SPDX-License-Identifier: Apache-2.0 */

#include <stddef.h>

#if !defined(SYN_DEF) || !defined(SYN_PREFIX)
#error "SYN_DEF and SYN_PREFIX must be defined"
#else

#define JOIN(a, b) a##b
#define CONCAT(a, b) JOIN(a, b)
#define PREFIXED(x) CONCAT(SYN_PREFIX, x)

/* syntax tree */

#define TOK(KIND) \
  KIND,

#define SEQ(KIND, TYPE, FIELDS) \
  KIND,

#define ALT(KIND, TYPE, OPTIONS) \
  KIND,

#define REP(KIND, TYPE, ITEM_TYPE) \
  KIND,

enum PREFIXED(_syn_kind)
{
SYN_DEF(TOK, SEQ, ALT, REP)
};

#undef TOK
#undef SEQ
#undef ALT
#undef REP

struct PREFIXED(_node)
{
  enum PREFIXED(_syn_kind) kind;
  unsigned int index;
  struct PREFIXED(_node) *parent;
  struct PREFIXED(_triv) *leading;
  struct PREFIXED(_triv) *trailing;
  union
  {
    size_t *offset;
    char *content;
  }
  text;
};

struct PREFIXED(_triv)
{
  size_t count;
  enum PREFIXED(_syn_kind) *kind;
  char *content;
  size_t *offset;
};

#define TOK(KIND)

#define SEQ(KIND, TYPE, FIELDS) \
  struct TYPE \
  { \
    struct PREFIXED(_node) syntax; \
    FIELDS(FIELD) \
  };

#define FIELD(KIND, TYPE, NAME) \
  struct TYPE *NAME;

#define ALT(KIND, TYPE, OPTIONS)

#define REP(KIND, TYPE, ITEM_TYPE) \
  struct TYPE \
  { \
    struct PREFIXED(_node) syntax; \
    struct ITEM_TYPE **items; \
    size_t count; \
  };

SYN_DEF(TOK, SEQ, ALT, REP)

#undef TOK
#undef SEQ
#undef FIELD
#undef ALT
#undef REP

typedef size_t PREFIXED(_ckpt_t);

struct PREFIXED(_builder) *PREFIXED(_builder_alloc)(void);
void PREFIXED(_builder_free)(struct PREFIXED(_builder) *self);
void PREFIXED(_builder_triv)(struct PREFIXED(_builder) *self, enum PREFIXED(_syn_kind) kind, char const *content, size_t length);
void PREFIXED(_builder_token)(struct PREFIXED(_builder) *self, enum PREFIXED(_syn_kind) kind, char const *content, size_t length);
PREFIXED(_ckpt_t) PREFIXED(_builder_open)(struct PREFIXED(_builder) *self);
void PREFIXED(_builder_close)(struct PREFIXED(_builder) *self, PREFIXED(_ckpt_t) ckpt);
struct PREFIXED(_node) *PREFIXED(_builder_finish)(struct PREFIXED(_builder) *self, enum PREFIXED(_syn_kind) kind);

#undef JOIN
#undef CONCAT
#undef PREFIXED

#endif
