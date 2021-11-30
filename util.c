#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "util.h"

int is_alphabet(int c)
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

int is_number(int c)
{
    switch (c) {
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
        return 1;
    }
    return 0;
}

int is_space(int c)
{
    switch (c) {
    case ' ': case '\t': case '\n': case '\r':
        return 1;
    }
    return 0;
}

int is_graphical(int c)
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

uint64_t msb64(uint64_t n)
{
#if defined(__GNUC__) || defined(__clang__)
    return (uint64_t) 1 << (63 - __builtin_clzll(n));
#else
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    n |= n >> 32;
    return n & ~(n >> 1);
#endif
}

size_t popcount64(uint64_t n)
{
#if defined(__GNUC__) || defined(__clang__)
    return __builtin_popcountll(n);
#else
    n -= (n >> 1) & 0x5555555555555555;
    n = (n & 0x3333333333333333) + ((n >> 2) & 0x3333333333333333);
    n = (n + (n >> 4)) & 0x0f0f0f0f0f0f0f0f;
    n += n >> 8;
    n += n >> 16;
    n += n >> 32;
    return n & 0x000000000000007f;
#endif
}

void *xmalloc(size_t size)
{
    void *ret = malloc(size);
    if (ret == NULL) {
        fprintf(stderr, "memory allocation failed!");
        exit(1);
    }
    return ret;
}
