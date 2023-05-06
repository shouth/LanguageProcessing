#ifndef MESSAGE_H
#define MESSAGE_H

#include "source.h"

typedef enum {
  MSG_HELP,
  MSG_NOTE,
  MSG_WARN,
  MSG_ERROR,
  MSG_FATAL
} msg_level_t;

typedef struct msg_entry msg_entry_t;
struct msg_entry {
  char         msg[256];
  msg_level_t  level;
  msg_entry_t *next;
};

typedef struct msg_inline_entry msg_inline_entry_t;
struct msg_inline_entry {
  char                msg[256];
  region_t            region;
  msg_inline_entry_t *next;
};

typedef struct {
  const source_t     *src;
  char                msg[256];
  region_t            region;
  msg_level_t         level;
  msg_inline_entry_t *inline_entries;
  msg_entry_t        *entries;
} msg_t;

msg_t *msg_new(const source_t *src, region_t region, msg_level_t level, const char *fmt, ...);
void   msg_delete(msg_t *msg);
void   msg_add(msg_t *msg, msg_level_t level, const char *fmt, ...);
void   msg_add_inline(msg_t *msg, region_t region, const char *fmt, ...);
void   msg_emit(msg_t *msg);

#endif
