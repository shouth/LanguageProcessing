#include <stddef.h>
#include <string.h>

#include "checker.h"
#include "inference.h"
#include "parser.h"
#include "resolution.h"
#include "resolver.h"
#include "session.h"
#include "source.h"
#include "token_tree.h"
#include "utility.h"

struct Session {
  char      *filename;
  Source    *source;
  TokenTree *tree;
  Res       *res;
  Infer     *infer;
  int        error;
};

Session *session_new(const char *filename)
{
  Session *session  = xmalloc(sizeof(Session));
  session->filename = xmalloc(strlen(filename) + 1);
  strcpy(session->filename, filename);
  session->source = NULL;
  session->tree   = NULL;
  session->res    = NULL;
  session->infer  = NULL;
  session->error  = 0;
  return session;
}

void session_free(Session *session)
{
  if (session) {
    free(session->filename);
    source_free(session->source);
    token_tree_free(session->tree);
    res_free(session->res);
    infer_free(session->infer);
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

const TokenTree *session_parse(Session *session)
{
  if (!session->tree) {
    const Source *source = session_load(session);
    if (!session->error && mppl_parse(source, &session->tree)) {
      return session->tree;
    } else {
      session->error = 1;
    }
  }
  return session->tree;
}

const Res *session_resolve(Session *session)
{
  if (!session->res) {
    const Source    *source = session_load(session);
    const TokenTree *tree   = session_parse(session);
    if (!session->error && mppl_resolve(source, (const TokenNode *) tree, &session->res)) {
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
    const Source    *source = session_load(session);
    const TokenTree *tree   = session_parse(session);
    const Res       *res    = session_resolve(session);
    if (!session->error && mppl_check(source, (const TokenNode *) tree, res, &session->infer)) {
      return session->infer;
    } else {
      session->error = 1;
    }
  }
  return session->infer;
}
