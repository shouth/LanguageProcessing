#include <stdlib.h>
#include <assert.h>

#include "rule_stream.h"

rule_stream_t *rule_stream_new(rule_stream_type_t type)
{
    rule_stream_t *ret = (rule_stream_t *) malloc(sizeof(rule_stream_t));
    assert(ret != NULL);
    ret->parent = NULL;
    ret->type = type;
    ret->stream.next = NULL;
    ret->stream.child.front = NULL;
    ret->stream.child.back = &ret->stream.child.front;
    return ret;
}

rule_stream_t *rule_stream_new_terminal(const terminal_t *terminal)
{
    rule_stream_t *ret = (rule_stream_t *) malloc(sizeof(rule_stream_t));
    assert(terminal != NULL && ret != NULL);
    ret->parent = NULL;
    ret->type = RULE_STREAM_TERMINAL;
    ret->terminal = *terminal;
    return ret;
}

void rule_stream_push(rule_stream_t *stream, rule_stream_t *child)
{
    assert(stream != NULL && child != NULL);
    *stream->stream.child.back = child;
    stream->stream.child.back = &child->stream.next;
    child->parent = stream;
}

void rule_stream_free(rule_stream_t *stream)
{
    if (stream->type != RULE_STREAM_TERMINAL) {
        if (stream->stream.next != NULL) {
            rule_stream_free(stream->stream.next);
        }
        if (stream->stream.child.front != NULL) {
            rule_stream_free(stream->stream.child.front);
        }
    }
    free(stream);
}
