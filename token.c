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
        data->data.string.terminated = va_arg(args, int);
        break;
    case TOKEN_BRACES_COMMENT:
        data->data.braces_comment.terminated = va_arg(args, int);
        break;
    case TOKEN_CSTYLE_COMMENT:
        data->data.cstyle_comment.terminated = va_arg(args, int);
        break;
    }
    va_end(args);
}
