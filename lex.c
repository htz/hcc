// Copyright 2019 @htz. Released under the MIT license.

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hcc.h"

#define BUFF_SIZE 256

static void push_line(lex_t *lex);

lex_t *lex_new(FILE *fp) {
  string_t *src = string_new();
  char buf[BUFF_SIZE];
  while (fgets(buf, BUFF_SIZE, fp)) {
    string_append(src, buf);
  }
  return lex_new_string(src);
}

lex_t *lex_new_string(string_t *str) {
  lex_t *lex = (lex_t *)malloc(sizeof (lex_t));
  lex->src = str;
  lex->p = lex->src->buf;
  lex->tbuf = vector_new();;
  lex->line = 1;
  lex->column = 1;
  lex->tokens = vector_new();
  lex->lines = vector_new();
  push_line(lex);
  return lex;
}

void lex_free(lex_t *lex) {
  string_free(lex->src);
  vector_free(lex->tbuf);
  while (lex->tokens->size > 0) {
    token_t *token = (token_t *)vector_pop(lex->tokens);
    token_free(token);
  }
  vector_free(lex->tokens);
  while (lex->lines->size > 0) {
    char *str = (char *)vector_pop(lex->lines);
    free(str);
  }
  vector_free(lex->lines);
  free(lex);
}

static void mark_pos(lex_t *lex) {
  lex->mark_line = lex->line;
  lex->mark_column = lex->column;
}

static void push_line(lex_t *lex) {
  char *p = lex->p;
  while (*p != '\0' && *p != '\r' && *p != '\n') {
    p++;
  }
  int len = p - lex->p;
  char *str = (char *)malloc(p - lex->p + 1);
  strncpy(str, lex->p, len);
  str[len] = '\0';
  vector_push(lex->lines, str);
}

static char get_char(lex_t *lex) {
  char c = *lex->p++;
  if (c == '\r') {
    c = *lex->p++;
    if (c != '\n' && c != '\0') {
      lex->p--;
    }
    c = '\n';
  }
  if (c == '\n') {
    if (lex->lines->size < lex->line) {
      push_line(lex);
    }
    lex->line++;
    lex->column = 1;
  } else {
    lex->column++;
  }
  return c;
}

static void unget_char(lex_t *lex, char c) {
  if (c == '\0') {
    return;
  }
  if (c == '\n') {
    lex->line--;
    lex->column = 1;
  } else {
    lex->column--;
  }
  lex->p--;
}

static void move_to(lex_t *lex, char *to) {
  while (lex->p < to) {
    get_char(lex);
  }
}

static token_t *read_number(lex_t *lex) {
  char *end;
  int val = strtoul(lex->p, &end, 10);
  move_to(lex, end);
  token_t *token = token_new(lex, TOKEN_KIND_INT);
  token->ival = val;
  return token;
}

static char read_escaped_char(lex_t *lex) {
  char c = get_char(lex);
  if (c == '\0') {
    errorf("unterminated char or string");
  }
  switch (c) {
  case '\'': case '"': case '?': case '\\':
    return c;
  case 'a': return '\a';
  case 'b': return '\b';
  case 'f': return '\f';
  case 'n': return '\n';
  case 'r': return '\r';
  case 't': return '\t';
  case 'v': return '\v';
  case '0': return '\0';
  }
  warnf("unknown escape character: \\%c", c);
  return c;
}

static token_t *read_char(lex_t *lex) {
  char c = get_char(lex);
  if (c == '\0') {
    errorf("unterminated char");
  }
  if (c == '\'') {
    errorf("unexpected char expression");
  }
  if (c == '\\') {
    c = read_escaped_char(lex);
  }
  if (get_char(lex) != '\'') {
    errorf("unterminated char");
  }
  token_t *token = token_new(lex, TOKEN_KIND_CHAR);
  token->ival = c;
  return token;
}

static token_t *read_string(lex_t *lex) {
  string_t *val = string_new();
  for (;;) {
    char c = get_char(lex);
    if (c == '"') {
      break;
    }
    if (c == '\0') {
      errorf("unterminated string");
    }
    if (c == '\\') {
      c = read_escaped_char(lex);
    }
    string_add(val, c);
  }
  token_t *token = token_new(lex, TOKEN_KIND_STRING);
  token->sval = val;
  return token;
}

static token_t *new_keyword(lex_t *lex, int k) {
  token_t *token = token_new(lex, TOKEN_KIND_KEYWORD);
  token->keyword = k;
  return token;
}

static token_t *read_identifier(lex_t *lex) {
  string_t *ident = string_new();
  for (;;) {
    char c = get_char(lex);
    if (isalnum(c) || c == '_') {
      string_add(ident, c);
    } else {
      unget_char(lex, c);
      break;
    }
  }
  assert(ident->size > 0);
  token_t *token = token_new(lex, TOKEN_KIND_IDENTIFIER);
  token->identifier = strdup(ident->buf);
  string_free(ident);
  return token;
}

static token_t *read_unknown(lex_t *lex, char c) {
  token_t *token = token_new(lex, TOKEN_KIND_UNKNOWN);
  token->keyword = c;
  return token;
}

token_t *lex_get_token(lex_t *lex) {
  char c;
  if (lex->tbuf->size > 0) {
    return (token_t *)vector_pop(lex->tbuf);
  }
  mark_pos(lex);
  do {
    c = get_char(lex);
  } while (c != '\0' && isspace(c));
  if (isdigit(c)) {
    unget_char(lex, c);
    return read_number(lex);
  }
  if (c == '\'') {
    return read_char(lex);
  }
  if (c == '"') {
    return read_string(lex);
  }
  if (isalpha(c) || c == '_') {
    unget_char(lex, c);
    return read_identifier(lex);
  }
  switch (c) {
  case '+': case '-': case '*': case '/': case '%':
  case '(': case ')': case ',': case ';': case '=':
    return new_keyword(lex, c);
  case '\0':
    return token_new(lex, TOKEN_KIND_EOF);
  }
  return read_unknown(lex, c);
}

void lex_unget_token(lex_t *lex, token_t *token) {
  vector_push(lex->tbuf, token);
}

token_t *lex_next_token_is(lex_t *lex, int kind) {
  token_t *token = lex_get_token(lex);
  if (token->kind == kind) {
    return token;
  }
  lex_unget_token(lex, token);
  return NULL;
}

token_t *lex_expect_token_is(lex_t *lex, int k) {
  token_t *token = lex_get_token(lex);
  if (token->kind != k) {
    errorf("%s expected, but got %c", token_name(k), token_str(token));
  }
  return token;
}

token_t *lex_next_keyword_is(lex_t *lex, int k) {
  token_t *token = lex_get_token(lex);
  if (token->kind == TOKEN_KIND_KEYWORD && token->keyword == k) {
    return token;
  }
  lex_unget_token(lex, token);
  return NULL;
}

token_t *lex_expect_keyword_is(lex_t *lex, int k) {
  token_t *token = lex_get_token(lex);
  if (token->kind != TOKEN_KIND_KEYWORD) {
    errorf("%c expected, but not got keyword: %s", k, token_str(token));
  }
  if (token->keyword != k) {
    errorf("%c expected, but got %c", k, token->keyword);
  }
  return token;
}
