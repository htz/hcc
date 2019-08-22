// Copyright 2019 @htz. Released under the MIT license.

#ifndef HCC_H_
#define HCC_H_

#include <stdbool.h>
#include <stdio.h>
#include <stdnoreturn.h>

typedef struct vector vector_t;
struct vector {
  void **data;
  int size;
  int capacity;
};

typedef struct string string_t;
struct string {
  char *buf;
  int size;
  int capacity;
};

enum {
  TOKEN_KIND_INT,
  TOKEN_KIND_KEYWORD,
  TOKEN_KIND_EOF,
  TOKEN_KIND_UNKNOWN,
};

typedef struct token token_t;
struct token {
  int kind;
  int line;
  int column;
  union {
    int ival;
    int keyword;
  };
};

typedef struct lex lex_t;
struct lex {
  string_t *src;
  char *p;
  vector_t *tbuf;
  int line, mark_line;
  int column, mark_column;
  vector_t *tokens;
  vector_t *lines;
};

enum {
  NODE_KIND_LITERAL_INT,
  NODE_KIND_BINARY_OP,
};

typedef struct node node_t;
struct node {
  int kind;
  union {
    int ival;
    // Binary operator
    struct {
      int op;
      node_t *left;
      node_t *right;
    };
  };
};

// vector.c
vector_t *vector_new(void);
void vector_free(vector_t *vec);
void vector_push(vector_t *vec, void *d);
void *vector_pop(vector_t *vec);

// string.c
string_t *string_new_with(char *s);
string_t *string_new(void);
void string_free(string_t *str);
void string_add(string_t *str, char c);
void string_append(string_t *str, char *s);
void string_appendf(string_t *str, char *fmt, ...);

// util.c
int max(int a, int b);

// token.c
token_t *token_new(lex_t *lex, int kind);
void token_free(token_t *token);
const char *token_name(int kind);
const char *token_str(token_t *token);

// lex.c
lex_t *lex_new(FILE *fp);
lex_t *lex_new_string(string_t *str);
void lex_free(lex_t *lex);
token_t *lex_get_token(lex_t *lex);
void lex_unget_token(lex_t *lex, token_t *token);
token_t *lex_next_token_is(lex_t *lex, int kind);
token_t *lex_expect_token_is(lex_t *lex, int k);
token_t *lex_next_keyword_is(lex_t *lex, int k);
token_t *lex_expect_keyword_is(lex_t *lex, int k);

// node.c
node_t *node_new_int(int ival);
node_t *node_new_binary_op(int op, node_t *left, node_t *right);
void node_free(node_t *node);
void node_debug(node_t *node);

// parse.c
node_t *parse_file(FILE *fp);

// gen.c
void gen(node_t *node);

// error.c
enum {
  COLOR_RED = 31,
  COLOR_YELLOW = 33,
};

void error_vprintf(char *label, int color, char *fmt, va_list args);
noreturn void errorf(char *fmt, ...);
void warnf(char *fmt, ...);

#endif  // HCC_H_
