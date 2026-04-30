/*
 * fmt.h -- format
 *
 * SPDX-FileCopyrightText: 2026 Shota Minami
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FMT_H
#define FMT_H

#include <stdio.h>

enum fmt_intensity {
  FMT_INTENSITY_NORMAL,
  FMT_INTENSITY_BOLD,
  FMT_INTENSITY_DIM
};

enum fmt_color_mode {
  FMT_COLOR_MODE_NONE,
  FMT_COLOR_MODE_4,
  FMT_COLOR_MODE_8,
  FMT_COLOR_MODE_24
};

typedef unsigned long fmt_color_t;

#define FMT_COLOR(MODE, COLOR) ((fmt_color_t) (MODE) << 24 | (COLOR))

#define FMT_BLACK   FMT_COLOR(FMT_COLOR_MODE_4, 0)
#define FMT_RED     FMT_COLOR(FMT_COLOR_MODE_4, 1)
#define FMT_GREEN   FMT_COLOR(FMT_COLOR_MODE_4, 2)
#define FMT_YELLOW  FMT_COLOR(FMT_COLOR_MODE_4, 3)
#define FMT_BLUE    FMT_COLOR(FMT_COLOR_MODE_4, 4)
#define FMT_MAGENTA FMT_COLOR(FMT_COLOR_MODE_4, 5)
#define FMT_CYAN    FMT_COLOR(FMT_COLOR_MODE_4, 6)
#define FMT_WHITE   FMT_COLOR(FMT_COLOR_MODE_4, 7)

#define FMT_BRIGHT_BLACK   FMT_COLOR(FMT_COLOR_MODE_4, 8)
#define FMT_BRIGHT_RED     FMT_COLOR(FMT_COLOR_MODE_4, 9)
#define FMT_BRIGHT_GREEN   FMT_COLOR(FMT_COLOR_MODE_4, 10)
#define FMT_BRIGHT_YELLOW  FMT_COLOR(FMT_COLOR_MODE_4, 11)
#define FMT_BRIGHT_BLUE    FMT_COLOR(FMT_COLOR_MODE_4, 12)
#define FMT_BRIGHT_MAGENTA FMT_COLOR(FMT_COLOR_MODE_4, 13)
#define FMT_BRIGHT_CYAN    FMT_COLOR(FMT_COLOR_MODE_4, 14)
#define FMT_BRIGHT_WHITE   FMT_COLOR(FMT_COLOR_MODE_4, 15)

#define FMT_RGB(COLOR) FMT_COLOR(FMT_COLOR_MODE_24, COLOR)

struct fmt_style {
  enum fmt_intensity intensity;
  fmt_color_t color;
};

void fmt_enable(int e);

void fmt_print(FILE *out, struct fmt_style *style, char const *fmt, ...);

#endif /* FMT_H */
