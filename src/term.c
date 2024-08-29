#include "term.h"
#include "utility.h"

TermStyle term_default_style(void)
{
  TermStyle style;
  style.foreground = TERM_COLOR_NONE;
  style.background = TERM_COLOR_NONE;
  style.intensity  = TERM_INTENSITY_NORMAL;
  style.italic     = TERM_ITALIC_OFF;
  style.underline  = TERM_UNDERLINE_OFF;
  return style;
}

#if defined(__linux__) || defined(__unix__) || defined(__APPLE__)

#if defined(_POSIX_C_SOURCE) && _POSIX_C_SOURCE >= 200112L

#include <stdio.h>
#include <unistd.h>

static int term_color_supported(FILE *file)
{
  return isatty(fileno(file));
}

#else

int term_color_supported(FILE *file)
{
  return 0;
}

#endif

#elif defined(_WIN32) || defined(_WIN64)

#include <io.h>
#include <stdio.h>

int term_color_supported(FILE *file)
{
  return _isatty(_fileno(file));
}

#else

int term_color_supported(FILE *file)
{
  return 0;
}

#endif

int term_use_color(int flag)
{
  static int use_color = -1;
  if (flag < 0) {
    if (use_color < 0) {
      use_color = term_color_supported(stdout);
    }
  } else {
    if (flag && term_color_supported(stdout)) {
      use_color = 1;
    } else {
      use_color = 0;
    }
  }
  return use_color;
}

void term_style(FILE *file, const TermStyle *style)
{
  if (!term_use_color(-1)) {
    return;
  }

  switch (style->intensity) {
  case TERM_INTENSITY_NORMAL:
    /* Do nothing */
    break;
  case TERM_INTENSITY_STRONG:
    fprintf(file, "\033[1m");
    break;
  case TERM_INTENSITY_FAINT:
    fprintf(file, "\033[2m");
    break;
  default:
    unreachable();
  }

  switch (style->italic) {
  case TERM_ITALIC_OFF:
    /* Do nothing */
    break;
  case TERM_ITALIC_ON:
    fprintf(file, "\033[3m");
    break;
  default:
    unreachable();
  }

  switch (style->underline) {
  case TERM_UNDERLINE_OFF:
    /* Do nothing */
    break;
  case TERM_UNDERLINE_ON:
    fprintf(file, "\033[4m");
    break;
  default:
    unreachable();
  }

  switch (style->foreground) {
  case TERM_COLOR_NONE:
    /* Do nothing */
    break;
  case TERM_COLOR_BLACK:
    fprintf(file, "\033[30m");
    break;
  case TERM_COLOR_RED:
    fprintf(file, "\033[31m");
    break;
  case TERM_COLOR_GREEN:
    fprintf(file, "\033[32m");
    break;
  case TERM_COLOR_YELLOW:
    fprintf(file, "\033[33m");
    break;
  case TERM_COLOR_BLUE:
    fprintf(file, "\033[34m");
    break;
  case TERM_COLOR_MAGENTA:
    fprintf(file, "\033[35m");
    break;
  case TERM_COLOR_CYAN:
    fprintf(file, "\033[36m");
    break;
  case TERM_COLOR_WHITE:
    fprintf(file, "\033[37m");
    break;
  case TERM_COLOR_BLACK | TERM_COLOR_BRIGHT:
    fprintf(file, "\033[90m");
    break;
  case TERM_COLOR_RED | TERM_COLOR_BRIGHT:
    fprintf(file, "\033[91m");
    break;
  case TERM_COLOR_GREEN | TERM_COLOR_BRIGHT:
    fprintf(file, "\033[92m");
    break;
  case TERM_COLOR_YELLOW | TERM_COLOR_BRIGHT:
    fprintf(file, "\033[93m");
    break;
  case TERM_COLOR_BLUE | TERM_COLOR_BRIGHT:
    fprintf(file, "\033[94m");
    break;
  case TERM_COLOR_MAGENTA | TERM_COLOR_BRIGHT:
    fprintf(file, "\033[95m");
    break;
  case TERM_COLOR_CYAN | TERM_COLOR_BRIGHT:
    fprintf(file, "\033[96m");
    break;
  case TERM_COLOR_WHITE | TERM_COLOR_BRIGHT:
    fprintf(file, "\033[97m");
    break;
  default: {
    if (style->foreground & TERM_COLOR_256) {
      unsigned long fg = style->foreground & ((1ul << 24) - 1);
      fprintf(file, "\033[38;2;%lu;%lu;%lum", fg >> 16, (fg >> 8) & 0xff, fg & 0xff);
    } else {
      unreachable();
    }
  }
  }

  switch (style->background) {
  case TERM_COLOR_NONE:
    /* Do nothing */
    break;
  case TERM_COLOR_BLACK:
    fprintf(file, "\033[40m");
    break;
  case TERM_COLOR_RED:
    fprintf(file, "\033[41m");
    break;
  case TERM_COLOR_GREEN:
    fprintf(file, "\033[42m");
    break;
  case TERM_COLOR_YELLOW:
    fprintf(file, "\033[43m");
    break;
  case TERM_COLOR_BLUE:
    fprintf(file, "\033[44m");
    break;
  case TERM_COLOR_MAGENTA:
    fprintf(file, "\033[45m");
    break;
  case TERM_COLOR_CYAN:
    fprintf(file, "\033[46m");
    break;
  case TERM_COLOR_WHITE:
    fprintf(file, "\033[47m");
    break;
  case TERM_COLOR_BLACK | TERM_COLOR_BRIGHT:
    fprintf(file, "\033[100m");
    break;
  case TERM_COLOR_RED | TERM_COLOR_BRIGHT:
    fprintf(file, "\033[101m");
    break;
  case TERM_COLOR_GREEN | TERM_COLOR_BRIGHT:
    fprintf(file, "\033[102m");
    break;
  case TERM_COLOR_YELLOW | TERM_COLOR_BRIGHT:
    fprintf(file, "\033[103m");
    break;
  case TERM_COLOR_BLUE | TERM_COLOR_BRIGHT:
    fprintf(file, "\033[104m");
    break;
  case TERM_COLOR_MAGENTA | TERM_COLOR_BRIGHT:
    fprintf(file, "\033[105m");
    break;
  case TERM_COLOR_CYAN | TERM_COLOR_BRIGHT:
    fprintf(file, "\033[106m");
    break;
  case TERM_COLOR_WHITE | TERM_COLOR_BRIGHT:
    fprintf(file, "\033[107m");
    break;
  default: {
    if (style->background & TERM_COLOR_256) {
      unsigned long bg = style->background & ((1ul << 24) - 1);
      fprintf(file, "\033[48;2;%lu;%lu;%lum", bg >> 16, (bg >> 8) & 0xff, bg & 0xff);
    } else {
      unreachable();
    }
  }
  }
}

void term_reset(FILE *file)
{
  if (!term_use_color(-1)) {
    return;
  }

  fprintf(file, "\033[0m");
}
