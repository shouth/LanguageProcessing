#include "parser.h"
#include "lexer.h"
#include "scanner.h"
#include "message.h"

int parser_init(parser_t *pa, const char *filename, parser_cb_t *on_success, parser_cb_t *on_failure)
{
    lexer_init(&pa->lexer, filename);
    pa->on_success = on_success;
    pa->on_failure = on_failure;
}

void parser_free(parser_t *pa)
{
    lexer_free(&pa->lexer);
}

int parser_success(parser_t *pa, ...)
{
    int ret;
    va_list args;
    va_start(args, pa);
    ret = pa->on_success(pa->lexer, args);
    va_end(args);
    return ret;
}

int parser_failure(parser_t *pa, ...)
{
    int ret;
    va_list args;
    va_start(args, pa);
    ret = pa->on_failure(pa->lexer, args);
    va_end(args);
    return ret;
}
