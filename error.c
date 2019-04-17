// Copyright 2019 @htz. Released under the MIT license.

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "hcc.h"

enum {
  COLOR_RED = 31,
  COLOR_YELLOW = 33,
};

static void print_message(char *label, int color, char *fmt, va_list args) {
  if (isatty(fileno(stderr))) {
    fprintf(stderr, "\e[1;%dm[%s]\e[0m ", color, label);
  } else {
    fprintf(stderr, "[%s] ", label);
  }
  vfprintf(stderr, fmt, args);
  fprintf(stderr, "\n");
}

void errorf(char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  print_message("ERROR", COLOR_RED, fmt, args);
  va_end(args);
  exit(1);
}

void warnf(char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  print_message("WARN", COLOR_YELLOW, fmt, args);
  va_end(args);
}
