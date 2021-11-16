#include <stdarg.h>
#include <assert.h>

#include "token.h"

void token_data_init(token_data_t *data, token_type_t type, ...)
{
    va_list args;
    int terminated;

    assert(data != NULL);

    data->type = type;
    va_start(args, type);
    switch (type) {
    case TOKEN_STRING:
        data->string.terminated = va_arg(args, int);
    case TOKEN_BRACES_COMMENT:
        data->braces_comment.terminated = va_arg(args, int);
    case TOKEN_CSTYLE_COMMENT:
        data->cstyle_comment.terminated = va_arg(args, int);
    }
    va_end(args);
}