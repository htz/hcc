// Copyright 2019 @htz. Released under the MIT license.

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hcc.h"

static lex_t *lex_new_file(file_t *f) {
  lex_t *lex = (lex_t *)malloc(sizeof (lex_t));
  lex->files = vector_new();
  vector_push(lex->files, f);
  lex->tokens = vector_new();
  return lex;
}

lex_t *lex_new(FILE *fp) {
  return lex_new_file(file_new(fp));
}

lex_t *lex_new_string(string_t *str) {
  return lex_new_file(file_new_string(str));
}

void lex_free(lex_t *lex) {
  for (int i = 0; i < lex->files->size; i++) {
    file_free((file_t *)lex->files->data[i]);
  }
  vector_free(lex->files);
  while (lex->tokens->size > 0) {
    token_t *token = (token_t *)vector_pop(lex->tokens);
    token_free(token);
  }
  vector_free(lex->tokens);
  free(lex);
}

file_t *lex_current_file(lex_t *lex) {
  if (lex->files->size == 0) {
    errorf("no input files");
  }
  return (file_t *)lex->files->data[lex->files->size - 1];
}

void lex_include(lex_t *lex, char *file_name) {
  vector_push(lex->files, file_new_filename(file_name));
}

static void mark_pos(lex_t *lex) {
  file_t *f = lex_current_file(lex);
  lex->mark_line = f->line;
  lex->mark_column = f->column;
  lex->mark_p = f->p;
}

char lex_get_char(lex_t *lex) {
  char c;
  for (;;) {
    file_t *f = lex_current_file(lex);
    c = file_get_char(f);
    if (c != '\0') {
      break;
    }
    if (lex->files->size == 1) {
      break;
    }
    file_free(f);
    vector_pop(lex->files);
    mark_pos(lex);
    return '\n';
  }
  return c;
}

void lex_unget_char(lex_t *lex, char c) {
  file_unget_char(lex_current_file(lex), c);
}

static bool next_char(lex_t *lex, char expact) {
  char c = lex_get_char(lex);
  if (c == expact) {
    return true;
  }
  lex_unget_char(lex, c);
  return false;
}

static void move_to(lex_t *lex, char *to) {
  file_t *f = lex_current_file(lex);
  while (f->p < to) {
    lex_get_char(lex);
  }
}

static token_t *read_integer(lex_t *lex, long val, char *end) {
  int kind = TOKEN_KIND_INT;
  if (end[0] == 'u' || end[0] == 'U') {
    if (end[1] == 'l' || end[1] == 'L') {
      if (end[2] == 'l' || end[2] == 'L') {
        end += 3;
        kind = TOKEN_KIND_ULLONG;
      } else {
        end += 2;
        kind = TOKEN_KIND_ULONG;
      }
    } else {
      end++;
      kind = TOKEN_KIND_UINT;
    }
  } else if (end[0] == 'l' || end[0] == 'L') {
    if (end[1] == 'l' || end[1] == 'L') {
      end += 2;
      kind = TOKEN_KIND_LLONG;
    } else {
      end++;
      kind = TOKEN_KIND_LONG;
    }
  }
  move_to(lex, end);
  token_t *token = token_new(lex, kind);
  token->ival = val;
  return token;
}

static token_t *read_float(lex_t *lex, double val, char *end) {
  int kind = TOKEN_KIND_DOUBLE;
  if (end[0] == 'f' || end[0] == 'F') {
    end++;
    kind = TOKEN_KIND_FLOAT;
  }
  move_to(lex, end);
  token_t *token = token_new(lex, kind);
  token->fval = val;
  return token;
}

static token_t *read_number(lex_t *lex, int base) {
  file_t *f = lex_current_file(lex);
  char *iend;
  long ival = strtoul(f->p, &iend, base);
  if (base == 10) {
    char *fend;
    double fval = strtod(f->p, &fend);
    if (iend < fend) {
      return read_float(lex, fval, fend);
    }
  }
  if (base == 16 && f->p == iend) {
    errorf("invalid hexadecimal digit");
  }
  return read_integer(lex, ival, iend);
}

static char read_escaped_char(lex_t *lex) {
  char c = lex_get_char(lex);
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
  char c = lex_get_char(lex);
  if (c == '\0') {
    errorf("unterminated char");
  }
  if (c == '\'') {
    errorf("unexpected char expression");
  }
  if (c == '\\') {
    c = read_escaped_char(lex);
  }
  if (lex_get_char(lex) != '\'') {
    errorf("unterminated char");
  }
  token_t *token = token_new(lex, TOKEN_KIND_CHAR);
  token->ival = c;
  return token;
}

static token_t *read_string(lex_t *lex) {
  string_t *val = string_new();
  for (;;) {
    char c = lex_get_char(lex);
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
    char c = lex_get_char(lex);
    if (isalnum(c) || c == '_') {
      string_add(ident, c);
    } else {
      lex_unget_char(lex, c);
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
  file_t *f = lex_current_file(lex);
  char c;
  if (f->tbuf->size > 0) {
    return (token_t *)vector_pop(f->tbuf);
  }
  lex->is_space = false;
  mark_pos(lex);
  char *start_p = lex->mark_p;
retry:
  for (;;) {
    c = lex_get_char(lex);
    if (c == '\0') {
      return token_new(lex, TOKEN_KIND_EOF);
    }
    if (c == ' ' || c == '\t' || c == '\v') {
      lex->is_space = true;
      continue;
    }
    if (c == '\n') {
      lex->is_space = true;
      return token_new(lex, TOKEN_KIND_NEWLINE);
    }
    if (c == '\r') {
      next_char(lex, '\n');
      lex->is_space = true;
      return token_new(lex, TOKEN_KIND_NEWLINE);
    }
    if (c == '\\') {
      c = lex_get_char(lex);
      if (c == '\n') {
        lex->is_space = true;
        continue;
      }
      if (c == '\r') {
        next_char(lex, '\n');
        lex->is_space = true;
        continue;
      }
      lex_unget_char(lex, c);
    }
    break;
  }
  f = lex_current_file(lex);
  if (start_p != f->p - 1 || (f->line > 1 && f->column - 1 == 1)) {
    lex->is_space = true;
  }
  lex->mark_p = f->p - 1;
  if (isdigit(c)) {
    if (c == '0') {
      c = lex_get_char(lex);
      if (c == 'x' || c == 'X') {
        return read_number(lex, 16);
      }
      lex_unget_char(lex, c);
      if (isdigit(c)) {
        return read_number(lex, 8);
      }
      return read_number(lex, 10);
    }
    lex_unget_char(lex, c);
    return read_number(lex, 10);
  }
  if (c == '\'') {
    return read_char(lex);
  }
  if (c == '"') {
    return read_string(lex);
  }
  if (isalpha(c) || c == '_') {
    lex_unget_char(lex, c);
    token_t *token = read_identifier(lex);
    assert(token->kind == TOKEN_KIND_IDENTIFIER);
    if (strcmp("if", token->identifier) == 0) {
      return new_keyword(lex, TOKEN_KEYWORD_IF);
    }
    if (strcmp("else", token->identifier) == 0) {
      return new_keyword(lex, TOKEN_KEYWORD_ELSE);
    }
    if (strcmp("return", token->identifier) == 0) {
      return new_keyword(lex, TOKEN_KEYWORD_RETURN);
    }
    if (strcmp("while", token->identifier) == 0) {
      return new_keyword(lex, TOKEN_KEYWORD_WHILE);
    }
    if (strcmp("do", token->identifier) == 0) {
      return new_keyword(lex, TOKEN_KEYWORD_DO);
    }
    if (strcmp("for", token->identifier) == 0) {
      return new_keyword(lex, TOKEN_KEYWORD_FOR);
    }
    if (strcmp("break", token->identifier) == 0) {
      return new_keyword(lex, TOKEN_KEYWORD_BREAK);
    }
    if (strcmp("continue", token->identifier) == 0) {
      return new_keyword(lex, TOKEN_KEYWORD_CONTINUE);
    }
    if (strcmp("signed", token->identifier) == 0) {
      return new_keyword(lex, TOKEN_KEYWORD_SIGNED);
    }
    if (strcmp("unsigned", token->identifier) == 0) {
      return new_keyword(lex, TOKEN_KEYWORD_UNSIGNED);
    }
    if (strcmp("struct", token->identifier) == 0) {
      return new_keyword(lex, TOKEN_KEYWORD_STRUCT);
    }
    if (strcmp("union", token->identifier) == 0) {
      return new_keyword(lex, TOKEN_KEYWORD_UNION);
    }
    if (strcmp("enum", token->identifier) == 0) {
      return new_keyword(lex, TOKEN_KEYWORD_ENUM);
    }
    if (strcmp("sizeof", token->identifier) == 0) {
      return new_keyword(lex, OP_SIZEOF);
    }
    if (strcmp("switch", token->identifier) == 0) {
      return new_keyword(lex, TOKEN_KEYWORD_SWITCH);
    }
    if (strcmp("case", token->identifier) == 0) {
      return new_keyword(lex, TOKEN_KEYWORD_CASE);
    }
    if (strcmp("default", token->identifier) == 0) {
      return new_keyword(lex, TOKEN_KEYWORD_DEFAULT);
    }
    if (strcmp("goto", token->identifier) == 0) {
      return new_keyword(lex, TOKEN_KEYWORD_GOTO);
    }
    if (strcmp("typedef", token->identifier) == 0) {
      return new_keyword(lex, TOKEN_KEYWORD_TYPEDEF);
    }
    if (strcmp("static", token->identifier) == 0) {
      return new_keyword(lex, TOKEN_KEYWORD_STATIC);
    }
    if (strcmp("extern", token->identifier) == 0) {
      return new_keyword(lex, TOKEN_KEYWORD_EXTERN);
    }
    if (strcmp("const", token->identifier) == 0) {
      return new_keyword(lex, TOKEN_KEYWORD_CONST);
    }
    if (strcmp("__typecode", token->identifier) == 0) {
      return new_keyword(lex, TOKEN_KEYWORD_TYPECODE);
    }
    return token;
  }
  switch (c) {
  case '+':
    if (next_char(lex, '+')) {
      return new_keyword(lex, OP_INC);
    }
    if (next_char(lex, '=')) {
      return new_keyword(lex, '+' | OP_ASSIGN_MASK);
    }
    return new_keyword(lex, '+');
  case '-':
    if (next_char(lex, '-')) {
      return new_keyword(lex, OP_DEC);
    }
    if (next_char(lex, '=')) {
      return new_keyword(lex, '-' | OP_ASSIGN_MASK);
    }
    if (next_char(lex, '>')) {
      return new_keyword(lex, OP_ARROW);
    }
    return new_keyword(lex, '-');
  case '*':
    if (next_char(lex, '=')) {
      return new_keyword(lex, '*' | OP_ASSIGN_MASK);
    }
    return new_keyword(lex, '*');
  case '/':
    if (next_char(lex, '=')) {
      return new_keyword(lex, '/' | OP_ASSIGN_MASK);
    }
    if (next_char(lex, '/')) {
      for (;;) {
        c = lex_get_char(lex);
        if (c == '\0') {
          return token_new(lex, TOKEN_KIND_EOF);
        }
        if (c == '\n') {
          lex->is_space = true;
          goto retry;
        }
      }
    }
    if (next_char(lex, '*')) {
      for (;;) {
        c = lex_get_char(lex);
        if (c == '\0') {
          return token_new(lex, TOKEN_KIND_EOF);
        }
        if (c == '*') {
          c = lex_get_char(lex);
          if (c == '\0') {
            return token_new(lex, TOKEN_KIND_EOF);
          }
          if (c == '/') {
            lex->is_space = true;
            goto retry;
          }
        }
      }
    }
    return new_keyword(lex, '/');
  case '%':
    if (next_char(lex, '=')) {
      return new_keyword(lex, '%' | OP_ASSIGN_MASK);
    }
    return new_keyword(lex, '%');
  case '&':
    if (next_char(lex, '&')) {
      return new_keyword(lex, OP_ANDAND);
    }
    if (next_char(lex, '=')) {
      return new_keyword(lex, '&' | OP_ASSIGN_MASK);
    }
    return new_keyword(lex, '&');
  case '|':
    if (next_char(lex, '|')) {
      return new_keyword(lex, OP_OROR);
    }
    if (next_char(lex, '=')) {
      return new_keyword(lex, '|' | OP_ASSIGN_MASK);
    }
    return new_keyword(lex, '|');
  case '^':
    if (next_char(lex, '=')) {
      return new_keyword(lex, '^' | OP_ASSIGN_MASK);
    }
    return new_keyword(lex, '^');
  case '<':
    if (next_char(lex, '<')) {
      if (next_char(lex, '=')) {
        return new_keyword(lex, OP_SAL | OP_ASSIGN_MASK);
      }
      return new_keyword(lex, OP_SAL);
    }
    if (next_char(lex, '=')) {
      return new_keyword(lex, OP_LE);
    }
    return new_keyword(lex, '<');
  case '>':
    if (next_char(lex, '>')) {
      if (next_char(lex, '=')) {
        return new_keyword(lex, OP_SAR | OP_ASSIGN_MASK);
      }
      return new_keyword(lex, OP_SAR);
    }
    if (next_char(lex, '=')) {
      return new_keyword(lex, OP_GE);
    }
    return new_keyword(lex, '>');
  case '=':
    if (next_char(lex, '=')) {
      return new_keyword(lex, OP_EQ);
    }
    return new_keyword(lex, '=');
  case '!':
    if (next_char(lex, '=')) {
      return new_keyword(lex, OP_NE);
    }
    return new_keyword(lex, '!');
  case '#':
    if (next_char(lex, '#')) {
      return new_keyword(lex, OP_HASHHASH);
    }
    return new_keyword(lex, '#');
  case '(': case ')': case ',': case ';':
  case '[': case ']': case '{': case '}':
  case '?': case ':': case '~':
    return new_keyword(lex, c);
  case '.':
    c = lex_get_char(lex);
    lex_unget_char(lex, c);
    if (isdigit(c)) {
      lex_unget_char(lex, '.');
      return read_number(lex, 10);
    }
    if (next_char(lex, '.')) {
      if (next_char(lex, '.')) {
        return new_keyword(lex, TOKEN_KEYWORD_ELLIPSIS);
      }
      lex_unget_char(lex, '.');
    }
    return new_keyword(lex, '.');
  case '\0':
    return token_new(lex, TOKEN_KIND_EOF);
  }
  return read_unknown(lex, c);
}

void lex_unget_token(lex_t *lex, token_t *token) {
  file_t *f = lex_current_file(lex);
  vector_push(f->tbuf, token);
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

void lex_skip_whitespace(lex_t *lex) {
  for (;;) {
    char c = lex_get_char(lex);
    if (c == ' ' || c == '\t' || c == '\v') {
      continue;
    }
    if (c == '/') {
      c = lex_get_char(lex);
      if (c == '*') {
        for (;;) {
          c = lex_get_char(lex);
          if (c == '\0') {
            errorf("unterminated conditional directive");
          }
          if (c == '*') {
            c = lex_get_char(lex);
            if (c == '\0') {
              errorf("unterminated conditional directive");
            }
            if (c == '/') {
              break;
            }
          }
        }
        continue;
      }
      lex_unget_char(lex, c);
      c = '/';
    }
    lex_unget_char(lex, c);
    break;
  }
}

void lex_skip_char(lex_t *lex) {
  for (;;) {
    char c = lex_get_char(lex);
    if (c == '\\') {
      lex_get_char(lex);
      if (c == '\'') {
        continue;
      }
    }
    if (c == '\0') {
      errorf("unterminated character");
    }
    if (c == '\'') {
      break;
    }
  }
}

void lex_skip_string(lex_t *lex) {
  for (;;) {
    char c = lex_get_char(lex);
    if (c == '\\') {
      c = lex_get_char(lex);
      if (c == '"') {
        continue;
      }
    }
    if (c == '\0') {
      errorf("unterminated string");
    }
    if (c == '"') {
      break;
    }
  }
}

void lex_skip_line(lex_t *lex) {
  for (;;) {
    char c = lex_get_char(lex);
    if (c == '\0') {
      errorf("unterminated conditional directive");
    }
    if (c == '\n') {
      break;
    }
  }
}
