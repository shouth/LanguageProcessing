/*
 * fmt.c -- format
 *
 * SPDX-FileCopyrightText: 2026 Shota Minami
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdarg.h>
#include <stdio.h>

#include "fmt.h"

#if defined(_POSIX_C_SOURCE)

#include <unistd.h>

static int is_tty(FILE *out)
{
  return isatty(fileno(out));
}

#else

static int is_tty(FILE *out)
{
  (void) out;
  return 0;
}

#endif

static int enable = 1;

void fmt_enable(int e)
{
  enable = e;
}

void fmt_print(FILE *out, struct fmt_style *style, char const *fmt, ...)
{
  va_list args;
  va_start(args, fmt);

  if (!enable || !is_tty(out)) {
    style = NULL;
  }

  if (style) {
    printf("\x1b[");

    switch (style->intensity) {
    case FMT_INTENSITY_BOLD: printf("1"); break;
    case FMT_INTENSITY_DIM:  printf("2"); break;

    default: break;
    }

    printf(";");

    switch (style->color >> 24) {
    case FMT_COLOR_MODE_4:
      switch (style->color) {
      case FMT_BLACK:   printf("30"); break;
      case FMT_RED:     printf("31"); break;
      case FMT_GREEN:   printf("32"); break;
      case FMT_YELLOW:  printf("33"); break;
      case FMT_BLUE:    printf("34"); break;
      case FMT_MAGENTA: printf("35"); break;
      case FMT_CYAN:    printf("36"); break;
      case FMT_WHITE:   printf("37"); break;

      case FMT_BRIGHT_BLACK:   printf("90"); break;
      case FMT_BRIGHT_RED:     printf("91"); break;
      case FMT_BRIGHT_GREEN:   printf("92"); break;
      case FMT_BRIGHT_YELLOW:  printf("93"); break;
      case FMT_BRIGHT_BLUE:    printf("94"); break;
      case FMT_BRIGHT_MAGENTA: printf("95"); break;
      case FMT_BRIGHT_CYAN:    printf("96"); break;
      case FMT_BRIGHT_WHITE:   printf("97"); break;

      default: break;
      }
      break;

    case FMT_COLOR_MODE_8:
      printf("38;5;%lu", style->color & 0xFF);
      break;

    case FMT_COLOR_MODE_24:
      printf("38;2;%lu;%lu;%lu", (style->color & 0xFF0000) >> 16, (style->color & 0x00FF00) >> 8, style->color & 0x0000FF);
      break;

    default:
      break;
    }

    printf("m");
  }

  vfprintf(out, fmt, args);

  if (style) {
    printf("\x1b[0m");
  }

  va_end(args);
}
