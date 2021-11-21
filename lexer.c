#include "lexer.h"
#include "cursol.h"
#include "token.h"
#include "assert.h"

static int is_alphabet(int c)
{
    switch (c) {
    case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o':
    case 'p': case 'q': case 'r': case 's': case 't':
    case 'u': case 'v': case 'w': case 'x': case 'y':
    case 'z':
    case 'A': case 'B': case 'C': case 'D': case 'E':
    case 'F': case 'G': case 'H': case 'I': case 'J':
    case 'K': case 'L': case 'M': case 'N': case 'O':
    case 'P': case 'Q': case 'R': case 'S': case 'T':
    case 'U': case 'V': case 'W': case 'X': case 'Y':
    case 'Z':
        return 1;
    }
    return 0;
}

static int is_number(int c)
{
    switch (c) {
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
        return 1;
    }
    return 0;
}

static int is_space(int c)
{
    switch (c) {
    case ' ': case '\t': case '\n': case '\r':
        return 1;
    }
    return 0;
}

static int is_graphical(int c)
{
    switch (c) {
    case '!': case '"': case '#': case '$': case '%':
    case '&': case '\'': case '(': case ')': case '*':
    case '+': case ',': case '-': case '.': case '/':
    case ':': case ';': case '<': case '=': case '>':
    case '?': case '@': case '[': case '\\': case ']':
    case '^': case '_': case '`': case '{': case '|':
    case '}': case '~':
        return 1;
    }
    return is_alphabet(c) || is_number(c) || is_space(c);
}

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
    while (cursol_first(cur) != '}') {
        if (cursol_eof(cur)) {
            token_data_init(ret, TOKEN_BRACES_COMMENT, /* terminated */ 0);
            return;
        }
        cursol_next(cur);
    }
    cursol_next(cur);
    token_data_init(ret, TOKEN_BRACES_COMMENT, /* terminated */ 1);
}

void lex_cstyle_comment(cursol_t *cur, token_data_t *ret)
{
    assert(cur != NULL && ret != NULL);
    assert(cursol_first(cur) == '/' && cursol_second(cur) == '*');

    cursol_next(cur);
    cursol_next(cur);
    while (cursol_first(cur) != '*' || cursol_second(cur) != '/') {
        if (cursol_eof(cur)) {
            token_data_init(ret, TOKEN_CSTYLE_COMMENT, /* terminated */ 0);
            return;
        }
        cursol_next(cur);
    }
    cursol_next(cur);
    cursol_next(cur);
    token_data_init(ret, TOKEN_CSTYLE_COMMENT, /* terminated */ 1);
}

void lex_string(cursol_t *cur, token_data_t *ret)
{
    assert(cur != NULL && ret != NULL);
    assert(cursol_first(cur) == '\'');

    cursol_next(cur);
    while (cursol_first(cur) != '\'') {
        if (cursol_eof(cur)) {
            token_data_init(ret, TOKEN_STRING, /* terminated */ 0);
            return;
        }
        cursol_next(cur);
    }
    cursol_next(cur);
    token_data_init(ret, TOKEN_STRING, /* terminated */ 1);
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

void lexer_init(lexer_t *lexer, source_t *src)
{
    lexer->src = src;
    lexer->pos = src->src_size;
}

void lexer_next(lexer_t *lexer, token_t *ret)
{
    cursol_t cursol;
    const char *ptr = lexer->src->src_ptr + lexer->pos;
    size_t len = lexer->src->src_size - lexer->pos;
    assert(lexer != NULL && ret != NULL);

    cursol_init(&cursol, ptr, len);
    lex_token(&cursol, &ret->data);
    ret->ptr = ptr;
    ret->len = cursol_consumed(&cursol);
    ret->src = lexer->src;
    ret->pos = lexer->pos;
    lexer->pos += ret->len;
}
