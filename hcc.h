// Copyright 2019 @htz. Released under the MIT license.

#ifndef HCC_H_
#define HCC_H_

#include <stdbool.h>
#include <stdio.h>
#include <stdnoreturn.h>

#define assert(expr)                                                 \
  do {                                                               \
    if (!(expr)) {                                                   \
      errorf("%s:%d: Assertion failed: " #expr, __FILE__, __LINE__); \
    }                                                                \
  } while (0)

typedef struct vector vector_t;
struct vector {
  void **data;
  int size;
  int capacity;
};

typedef struct map_entry map_entry_t;
struct map_entry {
  char *key;
  void *val;
  int flag;
  map_entry_t *prev;
  map_entry_t *next;
};

typedef struct map map_t;
struct map {
  map_entry_t *slot;
  int size;
  int occupied;
  int slot_size_bit;
  int slot_size;
  int inc;
  void (*free_val_fn)(void *);
  map_entry_t *top;
  map_entry_t *bottom;
};

typedef struct string string_t;
struct string {
  char *buf;
  int size;
  int capacity;
};

enum {
  TYPE_KIND_VOID,
  TYPE_KIND_CHAR,
  TYPE_KIND_INT,
  TYPE_KIND_PTR,
  TYPE_KIND_ARRAY,
};

typedef struct type type_t;
struct type {
  char *name;
  int kind;
  type_t *parent;
  int bytes;
  int size;
  int total_size;
};

enum {
  TOKEN_KIND_CHAR,
  TOKEN_KIND_INT,
  TOKEN_KIND_STRING,
  TOKEN_KIND_KEYWORD,
  TOKEN_KIND_IDENTIFIER,
  TOKEN_KIND_EOF,
  TOKEN_KIND_UNKNOWN,
};

enum {
  TOKEN_KEYWORD_IF = 256,
  TOKEN_KEYWORD_ELSE,
  TOKEN_KEYWORD_RETURN,
};

typedef struct token token_t;
struct token {
  int kind;
  int line;
  int column;
  union {
    int ival;
    string_t *sval;
    int keyword;
    char *identifier;
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
  NODE_KIND_NOP,
  NODE_KIND_IDENTIFIER,
  NODE_KIND_LITERAL,
  NODE_KIND_STRING_LITERAL,
  NODE_KIND_INIT_LIST,
  NODE_KIND_VARIABLE,
  NODE_KIND_DECLARATION,
  NODE_KIND_BINARY_OP,
  NODE_KIND_UNARY_OP,
  NODE_KIND_CALL,
  NODE_KIND_BLOCK,
  NODE_KIND_IF,
  NODE_KIND_FUNCTION,
  NODE_KIND_RETURN,
};

typedef struct node node_t;
struct node {
  int kind;
  type_t *type;
  node_t *next;
  union {
    char *identifier;
    int ival;
    // String literal
    struct {
      string_t *sval;
      int sid;
    };
    // Initialize array
    vector_t *init_list;
    // Variable
    struct {
      char *vname;
      bool global;
      int voffset;
    };
    // Binary/Unary operator
    struct {
      int op;
      // Binary operand
      struct {
        node_t *left;
        node_t *right;
      };
      // Unary operand
      struct {
        node_t *operand;
      };
    };
    // Declaration variable
    struct {
      node_t *dec_var;
      node_t *dec_init;
    };
    // Declaration function
    struct {
      node_t *fvar;
      vector_t *fargs;
      node_t *fbody;
    };
    // Function call
    struct {
      node_t *func;
      vector_t *args;
    };
    // Block
    struct {
      vector_t *statements;
      map_t *vars;
      node_t *parent_block;
      vector_t *child_blocks;
    };
    // If statement
    struct {
      node_t *cond;
      node_t *then_body;
      node_t *else_body;
    };
    // return statement
    node_t *retval;
  };
};

typedef struct parse parse_t;
struct parse {
  lex_t *lex;
  vector_t *statements;
  vector_t *data;
  vector_t *nodes;
  map_t *vars;
  map_t *types;
  node_t *current_function;
  node_t *current_scope;
  // builtin types
  type_t *type_void;
  type_t *type_char;
  type_t *type_int;
};

// vector.c
vector_t *vector_new(void);
void vector_free(vector_t *vec);
void vector_push(vector_t *vec, void *d);
void *vector_pop(vector_t *vec);

// map.c
map_t *map_new(void);
map_entry_t *map_find(map_t *map, void *key);
void map_free(map_t *map);
void map_clear(map_t *map);
void *map_get(map_t *map, char *key);
void map_add(map_t *map, char *key, void *val);
int map_delete(map_t *map, char *key);

// string.c
string_t *string_new_with(char *s);
string_t *string_new(void);
void string_free(string_t *str);
void string_add(string_t *str, char c);
void string_append(string_t *str, char *s);
void string_appendf(string_t *str, char *fmt, ...);
string_t *string_dup(string_t *str0);
void string_print_quote(string_t *str, FILE *out);

// util.c
int max(int a, int b);
void align(int *np, int a);

// type.c
type_t *type_new_with_size(char *name, int kind, type_t *parent, int size);
type_t *type_new(char *name, int kind, type_t *ptr);
void type_free(type_t *t);
type_t *type_find(parse_t *parse, char *name);
void type_add(parse_t *parse, char *name, type_t *type);
type_t *type_get(parse_t *parse, char *name, type_t *parent);
type_t *type_get_ptr(parse_t *parse, type_t *type);
type_t *type_make_array(parse_t *parse, type_t *parent, int size);
bool type_is_assignable(type_t *a, type_t *b);
bool type_is_int(type_t *type);

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
node_t *node_new_nop(parse_t *parse);
node_t *node_new_identifier(parse_t *parse, char *identifier);
node_t *node_new_int(parse_t *parse, type_t *type, int ival);
node_t *node_new_string(parse_t *parse, string_t *sval, int sid);
node_t *node_new_init_list(parse_t *parse, type_t *type, vector_t *init);
node_t *node_new_variable(parse_t *parse, type_t *type, char *vname, bool global);
node_t *node_new_declaration(parse_t *parse, type_t *type, node_t *var, node_t *init);
node_t *node_new_binary_op(parse_t *parse, type_t *type, int op, node_t *left, node_t *right);
node_t *node_new_unary_op(parse_t *parse, type_t *type, int op, node_t *operand);
node_t *node_new_call(parse_t *parse, type_t *type, node_t *func, vector_t *args);
node_t *node_new_block(parse_t *parse, vector_t *statements, node_t *parent);
node_t *node_new_if(parse_t *parse, node_t *cond, node_t *then_body, node_t *else_body);
node_t *node_new_function(parse_t *parse, node_t *fvar, vector_t *fargs, node_t *fbody);
node_t *node_new_return(parse_t *parse, type_t *type, node_t *retval);
void node_free(node_t *node);
void node_debug(node_t *node);

// parse.c
void parse_free(parse_t *parse);
parse_t *parse_file(FILE *fp);

// gen.c
void gen(parse_t *parse);

// error.c
enum {
  COLOR_RED = 31,
  COLOR_YELLOW = 33,
};

void error_vprintf(char *label, int color, char *fmt, va_list args);
noreturn void errorf(char *fmt, ...);
void warnf(char *fmt, ...);

#endif  // HCC_H_
