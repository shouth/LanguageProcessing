#include <assert.h>

#include "lexer.h"
#include "cursol.h"
#include "token.h"
#include "util.h"

void lex_space(cursol_t *cur, token_data_t *ret)
{
    assert(cur != NULL && ret != NULL);
    assert(is_space(cursol_first(cur)));

    cursol_next(cur);
    while (is_space(cursol_first(cur))) {
        cursol_next(cur);
    }
    token_data_init(ret, TOKEN_WHITESPACE);
}

void lex_braces_comment(cursol_t *cur, token_data_t *ret)
{
    assert(cur != NULL && ret != NULL);
    assert(cursol_first(cur) == '{');

    cursol_next(cur);
    while (1) {
        if (cursol_first(cur) == '}') {
            cursol_next(cur);
            token_data_init(ret, TOKEN_BRACES_COMMENT, /* terminated */ 1);
            return;
        }

        if (cursol_eof(cur) || !is_graphical(cursol_first(cur))) {
            token_data_init(ret, TOKEN_BRACES_COMMENT, /* terminated */ 0);
            return;
        }

        cursol_next(cur);
    }
}

void lex_cstyle_comment(cursol_t *cur, token_data_t *ret)
{
    assert(cur != NULL && ret != NULL);
    assert(cursol_first(cur) == '/' && cursol_second(cur) == '*');

    cursol_next(cur);
    cursol_next(cur);
    while (1) {
        if (cursol_first(cur) == '*' && cursol_second(cur) == '/') {
            cursol_next(cur);
            cursol_next(cur);
            token_data_init(ret, TOKEN_CSTYLE_COMMENT, /* terminated */ 1);
            return;
        }

        if (cursol_eof(cur) || !is_graphical(cursol_second(cur))) {
            token_data_init(ret, TOKEN_CSTYLE_COMMENT, /* terminated */ 0);
            return;
        }

        cursol_next(cur);
    }
}

void lex_string(cursol_t *cur, token_data_t *ret)
{
    assert(cur != NULL && ret != NULL);
    assert(cursol_first(cur) == '\'');

    cursol_next(cur);
    while (1) {
        if (cursol_first(cur) == '\'') {
            cursol_next(cur);

            if (cursol_first(cur) == '\'') {
                cursol_next(cur);
            } else {
                token_data_init(ret, TOKEN_STRING, /* terminated */ 1);
                return;
            }
        }

        if (cursol_eof(cur) || !is_graphical(cursol_first(cur))
            || cursol_first(cur) == '\r' || cursol_first(cur) == '\n')
        {
            token_data_init(ret, TOKEN_STRING, /* terminated */ 0);
            return;
        }

        cursol_next(cur);
    }
}

void lex_name_or_keyword(cursol_t *cur, token_data_t *ret)
{
    assert(cur != NULL && ret != NULL);
    assert(is_alphabet(cursol_first(cur)));

    cursol_next(cur);
    while (is_alphabet(cursol_first(cur)) || is_number(cursol_first(cur))) {
        cursol_next(cur);
    }
    token_data_init(ret, TOKEN_NAME_OR_KEYWORD);
}

void lex_number(cursol_t *cur, token_data_t *ret)
{
    assert(cur != NULL && ret != NULL);
    assert(is_number(cursol_first(cur)));

    cursol_next(cur);
    while (is_number(cursol_first(cur))) {
        cursol_next(cur);
    }
    token_data_init(ret, TOKEN_NUMBER);
}

void lex_symbol(cursol_t *cur, token_data_t *ret)
{
    int c;
    assert(cur != NULL && ret != NULL);

    switch (cursol_first(cur)) {
    case '+':
        cursol_next(cur);
        token_data_init(ret, TOKEN_PLUS);
        break;
    case '-':
        cursol_next(cur);
        token_data_init(ret, TOKEN_MINUS);
        break;
    case '*':
        cursol_next(cur);
        token_data_init(ret, TOKEN_STAR);
        break;
    case '=':
        cursol_next(cur);
        token_data_init(ret, TOKEN_EQUAL);
        break;
    case '(':
        cursol_next(cur);
        token_data_init(ret, TOKEN_LPAREN);
        break;
    case ')':
        cursol_next(cur);
        token_data_init(ret, TOKEN_RPAREN);
        break;
    case '[':
        cursol_next(cur);
        token_data_init(ret, TOKEN_LSQPAREN);
        break;
    case ']':
        cursol_next(cur);
        token_data_init(ret, TOKEN_RSQPAREN);
        break;
    case '.':
        cursol_next(cur);
        token_data_init(ret, TOKEN_DOT);
        break;
    case ',':
        cursol_next(cur);
        token_data_init(ret, TOKEN_COMMA);
        break;
    case ';':
        cursol_next(cur);
        token_data_init(ret, TOKEN_SEMI);
        break;

    case '<':
        cursol_next(cur);
        switch (cursol_first(cur)) {
        case '>':
            cursol_next(cur);
            token_data_init(ret, TOKEN_NOTEQ);
            break;
        case '=':
            cursol_next(cur);
            token_data_init(ret, TOKEN_LEEQ);
            break;
        default:
            token_data_init(ret, TOKEN_LE);
            break;
        }
        break;

    case '>':
        cursol_next(cur);
        switch (cursol_first(cur)) {
        case '=':
            cursol_next(cur);
            token_data_init(ret, TOKEN_GREQ);
            break;
        default:
            token_data_init(ret, TOKEN_GR);
            break;
        }
        break;

    case ':':
        cursol_next(cur);
        switch (cursol_first(cur)) {
        case '=':
            cursol_next(cur);
            token_data_init(ret, TOKEN_ASSIGN);
            break;
        default:
            token_data_init(ret, TOKEN_COLON);
            break;
        }
        break;

    default:
        cursol_next(cur);
        token_data_init(ret, TOKEN_UNKNOWN);
        break;
    }
}

void lex_token(cursol_t *cur, token_data_t *ret)
{
    assert(cur != NULL && ret != NULL);

    if (cursol_eof(cur)) {
        token_data_init(ret, TOKEN_EOF);
        return;
    }

    if (is_space(cursol_first(cur))) {
        lex_space(cur, ret);
        return;
    }

    if (cursol_first(cur) == '{') {
        lex_braces_comment(cur, ret);
        return;
    }

    if (cursol_first(cur) == '/' && cursol_second(cur) == '*') {
        lex_cstyle_comment(cur, ret);
        return;
    }

    if (cursol_first(cur) == '\'') {
        lex_string(cur, ret);
        return;
    }

    if (is_alphabet(cursol_first(cur))) {
        lex_name_or_keyword(cur, ret);
        return;
    }

    if (is_number(cursol_first(cur))) {
        lex_number(cur, ret);
        return;
    }

    lex_symbol(cur, ret);
}

void lex(cursol_t *cursol, token_t *ret)
{
    assert(cursol != NULL && ret != NULL);
    ret->ptr = cursol->ptr;
    ret->pos = cursol_position(cursol);
    ret->src = cursol->src;

    lex_token(cursol, &ret->data);
    ret->len = cursol_position(cursol) - ret->pos;
}
