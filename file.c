// Copyright 2019 @htz. Released under the MIT license.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hcc.h"

#define BUFF_SIZE 256

static void push_line(file_t *f);

file_t *file_new(FILE *fp) {
  string_t *src = string_new();
  char buf[BUFF_SIZE];
  while (fgets(buf, BUFF_SIZE, fp)) {
    string_append(src, buf);
  }
  return file_new_string(src);
}

file_t *file_new_filename(char *file_name) {
  FILE *fp = fopen(file_name, "r");
  file_t *f = file_new(fp);
  f->file_name = strdup(file_name);
  fclose(fp);
  return f;
}

file_t *file_new_string(string_t *str) {
  file_t *f = (file_t *)malloc(sizeof (file_t));
  f->file_name = NULL;
  f->src = str;
  f->p = f->src->buf;
  f->tbuf = vector_new();;
  f->line = 1;
  f->column = 1;
  f->lines = vector_new();
  push_line(f);
  return f;
}

static void push_line(file_t *f) {
  char *p = f->p;
  while (*p != '\0' && *p != '\r' && *p != '\n') {
    p++;
  }
  int len = p - f->p;
  char *str = (char *)malloc(p - f->p + 1);
  strncpy(str, f->p, len);
  str[len] = '\0';
  vector_push(f->lines, str);
}

void file_free(file_t *f) {
  if (f->file_name != NULL) {
    free(f->file_name);
  }
  string_free(f->src);
  vector_free(f->tbuf);
  while (f->lines->size > 0) {
    char *str = (char *)vector_pop(f->lines);
    free(str);
  }
  vector_free(f->lines);
  free(f);
}

char file_get_char(file_t *f) {
  char c = *f->p++;
  if (c == '\r') {
    c = *f->p++;
    if (c != '\n' && c != '\0') {
      f->p--;
    }
    c = '\n';
  }
  if (c == '\n') {
    if (f->lines->size < f->line) {
      push_line(f);
    }
    f->line++;
    f->column = 1;
  } else {
    f->column++;
  }
  if (c == '\0') {
    f->p--;
  }
  return c;
}

void file_unget_char(file_t *f, char c) {
  if (c == '\0') {
    return;
  }
  if (c == '\n') {
    f->line--;
    f->column = 1;
  } else {
    f->column--;
  }
  f->p--;
}
