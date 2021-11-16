#include <stdarg.h>
#include <assert.h>

#include "token.h"

void token_init(token_t *token, token_type_t type, ...)
{
    va_list args;
    int terminated;

    assert(token != NULL);

    token->type = type;
    va_start(args, type);
    switch (type) {
    case TOKEN_STRING:
        token->string.terminated = va_arg(args, int);
    case TOKEN_BRACES_COMMENT:
        token->braces_comment.terminated = va_arg(args, int);
    case TOKEN_CSTYLE_COMMENT:
        token->cstyle_comment.terminated = va_arg(args, int);
    }
    va_end(args);
}