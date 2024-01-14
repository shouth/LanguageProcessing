#ifndef SESSION_H
#define SESSION_H

#include "inference.h"
#include "resolution.h"
#include "source.h"
#include "token_tree.h"

typedef struct Session Session;

Session         *session_new(const char *filename);
void             session_free(Session *session);
const Source    *session_load(Session *session);
const TokenTree *session_parse(Session *session);
const Res       *session_resolve(Session *session);
const Infer     *session_check(Session *session);
void             session_codegen(Session *session);

#endif
