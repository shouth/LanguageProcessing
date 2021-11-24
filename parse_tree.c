#include <stdlib.h>
#include <assert.h>

#include <stdio.h>

#include "parse_tree.h"

parse_tree_t *parse_tree_new(rule_type_t type)
{
    parse_tree_t *ret;
    ret = (parse_tree_t *) malloc(sizeof(parse_tree_t));
    ret->parent = NULL;
    ret->type = type;
    ret->next = NULL;
    ret->data.child.front = NULL;
    ret->data.child.back = &ret->data.child.front;
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
    assert(stream->type != RULE_TERMINAL);
    *stream->data.child.back = child;
    stream->data.child.back = &child->next;
    child->parent = stream;
}

void parse_tree_free(parse_tree_t *stream)
{
    if (stream == NULL) {
        return;
    }

    if (stream->type != RULE_TERMINAL) {
        if (stream->next != NULL) {
            parse_tree_free(stream->next);
        }
        if (stream->data.child.front != NULL) {
            parse_tree_free(stream->data.child.front);
        }
    }
    free(stream);
}
