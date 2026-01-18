#ifndef SYNTAX_H
#define SYNTAX_H

#include <stddef.h>

typedef unsigned int SyntaxKind;
typedef struct SyntaxHeader SyntaxHeader;
typedef struct SyntaxToken SyntaxToken;
typedef struct SyntaxTrivia SyntaxTrivia;
typedef struct SyntaxTriviaPiece SyntaxTriviaPiece;

struct SyntaxHeader
{
  SyntaxHeader const *parent;
  SyntaxKind kind;
  unsigned int data;
};

struct SyntaxToken
{
  SyntaxHeader header;
  char const *text;
};

struct SyntaxTrivia
{
  unsigned long *offsets;
  unsigned long count;
  SyntaxTriviaPiece *pieces;
};

struct SyntaxTriviaPiece
{
  SyntaxKind kind;
  char const* text;
};

#endif /* SYNTAX_H */
