/*
   Copyright 2022 Shota Minami

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#ifndef TERMINAL_H
#define TERMINAL_H

#include <stdio.h>

#define TERM_COLOR_NONE    0ul
#define TERM_COLOR_BLACK   1ul
#define TERM_COLOR_RED     2ul
#define TERM_COLOR_GREEN   3ul
#define TERM_COLOR_YELLOW  4ul
#define TERM_COLOR_BLUE    5ul
#define TERM_COLOR_MAGENTA 6ul
#define TERM_COLOR_CYAN    7ul
#define TERM_COLOR_WHITE   8ul
#define TERM_COLOR_BRIGHT  1ul << 24
#define TERM_COLOR_256     1ul << 25

#define TERM_INTENSITY_NORMAL 0
#define TERM_INTENSITY_STRONG 1
#define TERM_INTENSITY_FAINT  2

#define TERM_ITALIC_OFF 0
#define TERM_ITALIC_ON  1

#define TERM_UNDERLINE_OFF 0
#define TERM_UNDERLINE_ON  1

typedef unsigned long TermColorStyle;
typedef unsigned char TermIntensityStyle;
typedef unsigned char TermItalicStyle;
typedef unsigned char TermUnderlineStyle;

typedef struct TermStyle TermStyle;

struct TermStyle {
  TermColorStyle     foreground;
  TermColorStyle     background;
  TermIntensityStyle intensity;
  TermItalicStyle    italic;
  TermUnderlineStyle underline;
};

TermStyle term_default_style(void);
int       term_use_color(int flag);
void      term_style(FILE *file, const TermStyle *style);
void      term_reset(FILE *file);

#endif
