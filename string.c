// Copyright 2019 @htz. Released under the MIT license.

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include "hcc.h"

#define STRING_MIN_SIZE 8

string_t *string_new_with_capacity(int capacity) {
  string_t *str = (string_t *)malloc(sizeof (string_t));
  str->size = 0;
  str->capacity = capacity;
  str->buf = (char *)malloc(sizeof (char) * str->capacity);
  return str;
}

string_t *string_new_with(char *s) {
  int len = strlen(s);
  string_t *str = string_new_with_capacity(len + 1);
  str->size = len;
  strncpy(str->buf, s, len);
  str->buf[len] = '\0';
  return str;
}

string_t *string_new() {
  return string_new_with_capacity(STRING_MIN_SIZE);
}

void string_free(string_t *str) {
  free(str->buf);
  free(str);
}

static void extend(string_t *str, int delta) {
  if (str->size + delta + 1 <= str->capacity) {
    return;
  }
  int capacity = max(str->size + delta + 1, STRING_MIN_SIZE);
  str->buf = (char *)realloc(str->buf, sizeof (char) * capacity);
  str->capacity = capacity;
}

void string_add(string_t *str, char c) {
  extend(str, 1);
  str->buf[str->size++] = c;
  str->buf[str->size] = '\0';
}

void string_append(string_t *str, char *s) {
  int len = strlen(s);
  extend(str, len);
  strncpy(str->buf + str->size, s, len);
  str->size += len;
  str->buf[str->size] = '\0';
}

void string_appendf(string_t *str, char *fmt, ...) {
  va_list args;
  for (;;) {
    int avail = str->capacity - str->size;
    va_start(args, fmt);
    int written = vsnprintf(str->buf + str->size, avail, fmt, args);
    va_end(args);
    if (avail <= written) {
      extend(str, written);
      continue;
    }
    str->size += written;
    return;
  }
}

string_t *string_dup(string_t *str0) {
  string_t *str = string_new_with_capacity(str0->capacity);
  str->size = str0->size;
  memcpy(str->buf, str0->buf, sizeof (char) * str->capacity);
  return str;
}

void string_print_quote(string_t *str, FILE *out) {
  for (char *p = str->buf; *p; p++) {
    switch (*p) {
    case '\a': fprintf(out, "\\a"); continue;
    case '\b': fprintf(out, "\\b"); continue;
    case '\f': fprintf(out, "\\f"); continue;
    case '\n': fprintf(out, "\\n"); continue;
    case '\r': fprintf(out, "\\r"); continue;
    case '\t': fprintf(out, "\\t"); continue;
    }
    if (*p == '"' || *p == '\\') {
      fprintf(out, "\\");
    }
    fprintf(out, "%c", *p);
  }
}
