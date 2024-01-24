#include <stddef.h>
#include <string.h>

#include "checker.h"
#include "inference.h"
#include "mppl_syntax.h"
#include "parser.h"
#include "resolution.h"
#include "resolver.h"
#include "session.h"
#include "source.h"
#include "string.h"
#include "utility.h"

struct Session {
  char          *filename;
  Source        *source;
  MpplProgram   *syntax;
  Res           *res;
  Infer         *infer;
  int            error;
  StringContext *strings;
};

Session *session_new(const char *filename)
{
  Session *session  = xmalloc(sizeof(Session));
  session->filename = xmalloc(strlen(filename) + 1);
  strcpy(session->filename, filename);
  session->source  = NULL;
  session->syntax  = NULL;
  session->res     = NULL;
  session->infer   = NULL;
  session->error   = 0;
  session->strings = string_context_new();
  return session;
}

void session_free(Session *session)
{
  if (session) {
    free(session->filename);
    source_free(session->source);
    mppl_unref(session->syntax);
    res_free(session->res);
    infer_free(session->infer);
    string_context_free(session->strings);
    free(session);
  }
}

const Source *session_load(Session *session)
{
  if (!session->source) {
    if (!session->error) {
      session->source = source_new(session->filename, strlen(session->filename));
      if (!session->source) {
        session->error = 1;
      }
    }
  }
  return session->source;
}

const MpplProgram *session_parse(Session *session)
{
  if (!session->syntax) {
    const Source *source = session_load(session);
    if (!session->error && mppl_parse(source, session->strings, &session->syntax)) {
      return session->syntax;
    } else {
      session->error = 1;
    }
  }
  return session->syntax;
}

const Res *session_resolve(Session *session)
{
  if (!session->res) {
    const Source      *source = session_load(session);
    const MpplProgram *syntax = session_parse(session);
    if (!session->error && mppl_resolve(source, syntax, &session->res)) {
      return session->res;
    } else {
      session->error = 1;
    }
  }
  return session->res;
}

const Infer *session_check(Session *session)
{
  if (!session->infer) {
    const Source      *source = session_load(session);
    const MpplProgram *syntax = session_parse(session);
    const Res         *res    = session_resolve(session);
    if (!session->error && mppl_check(source, syntax, res, &session->infer)) {
      return session->infer;
    } else {
      session->error = 1;
    }
  }
  return session->infer;
}
