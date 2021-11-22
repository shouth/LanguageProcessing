#include <stdlib.h>
#include <assert.h>

#include "parse_tree.h"

parse_tree_t *parse_tree_new(rule_type_t type)
{
    parse_tree_t *ret;
    ret = (parse_tree_t *) malloc(sizeof(parse_tree_t));
    ret->parent = NULL;
    ret->type = type;
    ret->data.stream.next = NULL;
    ret->data.stream.child.front = NULL;
    ret->data.stream.child.back = &ret->data.stream.child.front;
    return ret;
}

parse_tree_t *parse_tree_new_terminal(const terminal_t *terminal)
{
    parse_tree_t *ret;
    assert(terminal != NULL);
    ret = (parse_tree_t *) malloc(sizeof(parse_tree_t));
    ret->parent = NULL;
    ret->type = RULE_TERMINAL;
    ret->data.terminal = *terminal;
    return ret;
}

void parse_tree_push(parse_tree_t *stream, parse_tree_t *child)
{
    assert(stream != NULL && child != NULL);
    *stream->data.stream.child.back = child;
    stream->data.stream.child.back = &child->data.stream.next;
    child->parent = stream;
}

void parse_tree_free(parse_tree_t *stream)
{
    if (stream == NULL) {
        return;
    }

    if (stream->type != RULE_TERMINAL) {
        if (stream->data.stream.next != NULL) {
            parse_tree_free(stream->data.stream.next);
        }
        if (stream->data.stream.child.front != NULL) {
            parse_tree_free(stream->data.stream.child.front);
        }
    }
    free(stream);
}
