#include <stdlib.h>
#include <assert.h>

#include "parse_tree.h"

parse_tree_t *parse_tree_new(rule_type_t type)
{
    parse_tree_t *ret = (parse_tree_t *) malloc(sizeof(parse_tree_t));
    assert(ret != NULL);
    ret->parent = NULL;
    ret->type = type;
    ret->stream.next = NULL;
    ret->stream.child.front = NULL;
    ret->stream.child.back = &ret->stream.child.front;
    return ret;
}

parse_tree_t *parse_tree_new_terminal(const terminal_t *terminal)
{
    parse_tree_t *ret = (parse_tree_t *) malloc(sizeof(parse_tree_t));
    assert(terminal != NULL && ret != NULL);
    ret->parent = NULL;
    ret->type = RULE_TERMINAL;
    ret->terminal = *terminal;
    return ret;
}

void parse_tree_push(parse_tree_t *stream, parse_tree_t *child)
{
    assert(stream != NULL && child != NULL);
    *stream->stream.child.back = child;
    stream->stream.child.back = &child->stream.next;
    child->parent = stream;
}

void parse_tree_free(parse_tree_t *stream)
{
    if (stream->type != RULE_TERMINAL) {
        if (stream->stream.next != NULL) {
            parse_tree_free(stream->stream.next);
        }
        if (stream->stream.child.front != NULL) {
            parse_tree_free(stream->stream.child.front);
        }
    }
    free(stream);
}
