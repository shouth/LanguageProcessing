#include <assert.h>

#include "lexer.h"
#include "cursol.h"
#include "util.h"

token_type_t lex_space(cursol_t *cur, token_data_t *ret)
{
    assert(cur != NULL && ret != NULL);
    assert(is_space(cursol_first(cur)));

    cursol_next(cur);
    while (is_space(cursol_first(cur))) {
        cursol_next(cur);
    }
    return TOKEN_WHITESPACE;
}

token_type_t lex_braces_comment(cursol_t *cur, token_data_t *ret)
{
    assert(cur != NULL && ret != NULL);
    assert(cursol_first(cur) == '{');

    cursol_next(cur);
    while (1) {
        if (cursol_first(cur) == '}') {
            cursol_next(cur);
            ret->braces_comment.terminated = 1;
            return TOKEN_BRACES_COMMENT;
        }

        if (cursol_eof(cur) || !is_graphical(cursol_first(cur))) {
            ret->braces_comment.terminated = 0;
            return TOKEN_BRACES_COMMENT;
        }

        cursol_next(cur);
    }
}

token_type_t lex_cstyle_comment(cursol_t *cur, token_data_t *ret)
{
    assert(cur != NULL && ret != NULL);
    assert(cursol_first(cur) == '/' && cursol_second(cur) == '*');

    cursol_next(cur);
    cursol_next(cur);
    while (1) {
        if (cursol_first(cur) == '*' && cursol_second(cur) == '/') {
            cursol_next(cur);
            cursol_next(cur);
            ret->cstyle_comment.terminated = 1;
            return TOKEN_CSTYLE_COMMENT;
        }

        if (cursol_eof(cur) || !is_graphical(cursol_second(cur))) {
            ret->cstyle_comment.terminated = 0;
            return TOKEN_CSTYLE_COMMENT;
        }

        cursol_next(cur);
    }
}

token_type_t lex_string(cursol_t *cur, token_data_t *ret)
{
    assert(cur != NULL && ret != NULL);
    assert(cursol_first(cur) == '\'');

    ret->string.len = 0;
    ret->string.str_len = 0;
    cursol_next(cur);
    while (1) {
        if (cursol_first(cur) == '\'') {
            cursol_next(cur);

            if (cursol_first(cur) != '\'') {
                ret->string.terminated = 1;
                return TOKEN_STRING;
            } else {
                ret->string.len++;
            }
        }

        if (cursol_eof(cur) || !is_graphical(cursol_first(cur))
            || cursol_first(cur) == '\r' || cursol_first(cur) == '\n')
        {
            ret->string.terminated = 0;
            return TOKEN_STRING;
        }

        ret->string.len++;
        ret->string.str_len++;
        cursol_next(cur);
    }
}

token_type_t lex_name_or_keyword(cursol_t *cur, token_data_t *ret)
{
    assert(cur != NULL && ret != NULL);
    assert(is_alphabet(cursol_first(cur)));

    cursol_next(cur);
    while (is_alphabet(cursol_first(cur)) || is_number(cursol_first(cur))) {
        cursol_next(cur);
    }
    return TOKEN_NAME_OR_KEYWORD;
}

token_type_t lex_number(cursol_t *cur, token_data_t *ret)
{
    assert(cur != NULL && ret != NULL);
    assert(is_number(cursol_first(cur)));

    cursol_next(cur);
    while (is_number(cursol_first(cur))) {
        cursol_next(cur);
    }
    return TOKEN_NUMBER;
}

token_type_t lex_symbol(cursol_t *cur, token_data_t *ret)
{
    assert(cur != NULL && ret != NULL);

    switch (cursol_first(cur)) {
    case '+':
        cursol_next(cur);
        return TOKEN_PLUS;
    case '-':
        cursol_next(cur);
        return TOKEN_MINUS;
    case '*':
        cursol_next(cur);
        return TOKEN_STAR;
    case '=':
        cursol_next(cur);
        return TOKEN_EQUAL;
    case '(':
        cursol_next(cur);
        return TOKEN_LPAREN;
        break;
    case ')':
        cursol_next(cur);
        return TOKEN_RPAREN;
    case '[':
        cursol_next(cur);
        return TOKEN_LSQPAREN;
    case ']':
        cursol_next(cur);
        return TOKEN_RSQPAREN;
    case '.':
        cursol_next(cur);
        return TOKEN_DOT;
    case ',':
        cursol_next(cur);
        return TOKEN_COMMA;
    case ';':
        cursol_next(cur);
        return TOKEN_SEMI;

    case '<':
        cursol_next(cur);
        switch (cursol_first(cur)) {
        case '>':
            cursol_next(cur);
            return TOKEN_NOTEQ;
        case '=':
            cursol_next(cur);
            return TOKEN_LEEQ;
        default:
            return TOKEN_LE;
        }

    case '>':
        cursol_next(cur);
        switch (cursol_first(cur)) {
        case '=':
            cursol_next(cur);
            return TOKEN_GREQ;
        default:
            return TOKEN_GR;
        }

    case ':':
        cursol_next(cur);
        switch (cursol_first(cur)) {
        case '=':
            cursol_next(cur);
            return TOKEN_ASSIGN;
        default:
            return TOKEN_COLON;
        }

    default:
        cursol_next(cur);
        return TOKEN_UNKNOWN;
    }
}

token_type_t lex_token(cursol_t *cur, token_data_t *ret)
{
    assert(cur != NULL && ret != NULL);

    if (cursol_eof(cur)) {
        return TOKEN_EOF;
    }

    if (is_space(cursol_first(cur))) {
        return lex_space(cur, ret);
    }

    if (cursol_first(cur) == '{') {
        return lex_braces_comment(cur, ret);
    }

    if (cursol_first(cur) == '/' && cursol_second(cur) == '*') {
        return lex_cstyle_comment(cur, ret);
    }

    if (cursol_first(cur) == '\'') {
        return lex_string(cur, ret);
    }

    if (is_alphabet(cursol_first(cur))) {
        return lex_name_or_keyword(cur, ret);
    }

    if (is_number(cursol_first(cur))) {
        return lex_number(cur, ret);
    }

    return lex_symbol(cur, ret);
}

void lex(cursol_t *cursol, token_t *ret)
{
    assert(cursol != NULL && ret != NULL);
    ret->ptr = cursol->ptr;
    ret->pos = cursol_position(cursol);
    ret->src = cursol->src;
    ret->type = lex_token(cursol, &ret->data);
    ret->len = cursol_position(cursol) - ret->pos;
}
