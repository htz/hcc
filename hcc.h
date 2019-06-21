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
  TYPE_KIND_BOOL,
  TYPE_KIND_CHAR,
  TYPE_KIND_SHORT,
  TYPE_KIND_INT,
  TYPE_KIND_LONG,
  TYPE_KIND_LLONG,
  TYPE_KIND_FLOAT,
  TYPE_KIND_DOUBLE,
  TYPE_KIND_LDOUBLE,
  TYPE_KIND_PTR,
  TYPE_KIND_ARRAY,
  TYPE_KIND_STRUCT,
  TYPE_KIND_ENUM,
  TYPE_KIND_FUNCTION,
  TYPE_KIND_STUB,
};

typedef struct type type_t;
struct type {
  char *name;
  int kind;
  bool sign;
  bool is_const;
  type_t *parent;
  int bytes;
  int size;
  int align;
  int total_size;
  union {
    // struct/union
    struct {
      map_t *fields;
      bool is_struct;
      bool is_typedef;
    };
    // function
    vector_t *argtypes;
  };
};

enum {
  TOKEN_KIND_CHAR,
  TOKEN_KIND_INT,
  TOKEN_KIND_UINT,
  TOKEN_KIND_LONG,
  TOKEN_KIND_ULONG,
  TOKEN_KIND_LLONG,
  TOKEN_KIND_ULLONG,
  TOKEN_KIND_FLOAT,
  TOKEN_KIND_DOUBLE,
  TOKEN_KIND_STRING,
  TOKEN_KIND_KEYWORD,
  TOKEN_KIND_IDENTIFIER,
  TOKEN_KIND_NEWLINE,
  TOKEN_KIND_EOF,
  TOKEN_KIND_UNKNOWN,
};

enum {
  TOKEN_KEYWORD_IF = 256,
  TOKEN_KEYWORD_ELSE,
  TOKEN_KEYWORD_RETURN,
  TOKEN_KEYWORD_WHILE,
  TOKEN_KEYWORD_DO,
  TOKEN_KEYWORD_FOR,
  TOKEN_KEYWORD_BREAK,
  TOKEN_KEYWORD_CONTINUE,
  TOKEN_KEYWORD_SIGNED,
  TOKEN_KEYWORD_UNSIGNED,
  TOKEN_KEYWORD_STRUCT,
  TOKEN_KEYWORD_UNION,
  TOKEN_KEYWORD_ENUM,
  TOKEN_KEYWORD_SWITCH,
  TOKEN_KEYWORD_CASE,
  TOKEN_KEYWORD_DEFAULT,
  TOKEN_KEYWORD_TYPEDEF,
  TOKEN_KEYWORD_STATIC,
  TOKEN_KEYWORD_EXTERN,
  TOKEN_KEYWORD_CONST,
  OP_SAL,    // <<
  OP_SAR,    // >>
  OP_EQ,     // ==
  OP_NE,     // !=
  OP_LE,     // <=
  OP_GE,     // >=
  OP_INC,    // ++@
  OP_DEC,    // --@
  OP_PINC,   // @++
  OP_PDEC,   // @--
  OP_ANDAND, // &&
  OP_OROR,   // ||
  OP_ARROW,  // ->
  OP_CAST,
  OP_SIZEOF, // sizeof
  OP_ASSIGN_MASK = 0x1000,
};

typedef struct token token_t;
struct token {
  int kind;
  int line;
  int column;
  vector_t *hideset;
  union {
    long ival;
    double fval;
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
  NODE_KIND_CONTINUE,
  NODE_KIND_BREAK,
  NODE_KIND_RETURN,
  NODE_KIND_DO,
  NODE_KIND_WHILE,
  NODE_KIND_FOR,
  NODE_KIND_SWITCH,
  NODE_KIND_CASE,
};

enum {
  BLOCK_KIND_DEFAULT,
  BLOCK_KIND_LOOP,
  BLOCK_KIND_SWITCH,
};

enum {
  STORAGE_CLASS_NONE,
  STORAGE_CLASS_TYPEDEF,
  STORAGE_CLASS_STATIC,
  STORAGE_CLASS_EXTERN,
};

typedef struct node node_t;
struct node {
  int kind;
  type_t *type;
  node_t *next;
  union {
    char *identifier;
    long ival;
    // float/double Literal
    struct {
      double fval;
      int fid;
    };
    // String literal
    struct {
      string_t *sval;
      int sid;
    };
    // Initialize array/struct/union
    vector_t *init_list;
    // Variable
    struct {
      char *vname;
      bool global;
      int sclass;
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
      int bkind;
      vector_t *statements;
      map_t *vars;
      map_t *types;
      map_t *tags;
      node_t *parent_block;
      vector_t *child_blocks;
      node_t *parent_node;
    };
    // If statement
    struct {
      node_t *cond;
      node_t *then_body;
      node_t *else_body;
    };
    // break/continue
    struct {
      node_t *cscope;
    };
    // return statement
    node_t *retval;
    // while/do/for
    struct {
      node_t *linit;
      node_t *lcond;
      node_t *lstep;
      node_t *lbody;
    };
    // switch
    struct {
      node_t *sexpr;
      node_t *sbody;
      vector_t *cases;
      node_t *default_case;
    };
    // case/default
    struct {
      node_t *cval;
      node_t *cstmt;
    };
  };
};

typedef struct macro macro_t;
struct macro {
  vector_t *tokens;
};

typedef struct parse parse_t;
struct parse {
  lex_t *lex;
  vector_t *statements;
  vector_t *data;
  vector_t *nodes;
  map_t *vars;
  map_t *types;
  map_t *tags;
  map_t *macros;
  node_t *current_function;
  node_t *current_scope;
  node_t *next_scope;
  // builtin types
  type_t *type_void;
  type_t *type_char;
  type_t *type_bool;
  type_t *type_schar;
  type_t *type_short;
  type_t *type_ushort;
  type_t *type_uchar;
  type_t *type_int;
  type_t *type_uint;
  type_t *type_long;
  type_t *type_ulong;
  type_t *type_llong;
  type_t *type_ullong;
  type_t *type_float;
  type_t *type_double;
  type_t *type_ldouble;
  // gen state
  int stackpos;
};

// vector.c
vector_t *vector_new(void);
void vector_free(vector_t *vec);
void vector_push(vector_t *vec, void *d);
void *vector_pop(vector_t *vec);
void *vector_dup(vector_t *vec);
bool vector_exists(vector_t *vec, void *d);

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
type_t *type_new_with_size(char *name, int kind, int sign, type_t *parent, int size);
type_t *type_new(char *name, int kind, int sign, type_t *ptr);
type_t *type_new_struct(char *name, bool is_struct);
type_t *type_new_typedef(char *name, type_t *type);
type_t *type_new_enum(char *name);
type_t *type_new_const(type_t *type);
type_t *type_new_stub();
void type_free(type_t *t);
type_t *type_find(parse_t *parse, char *name);
void type_add(parse_t *parse, char *name, type_t *type);
type_t *type_get(parse_t *parse, char *name, type_t *parent);
type_t *type_get_ptr(parse_t *parse, type_t *type);
type_t *type_get_function(parse_t *parse, type_t *rettype, vector_t *argtypes);
type_t *type_get_const(parse_t *parse, type_t *type);
void type_add_by_tag(parse_t *parse, char *tag, type_t *type);
type_t *type_get_by_tag(parse_t *parse, char *tag, bool local_only);
void type_add_typedef(parse_t *parse, char *name, type_t *type);
type_t *type_make_array(parse_t *parse, type_t *parent, int size);
bool type_is_assignable(type_t *a, type_t *b);
const char *type_kind_names_str(int kind);
bool type_is_bool(type_t *type);
bool type_is_int(type_t *type);
bool type_is_float(type_t *type);
bool type_is_struct(type_t *type);
bool type_is_function(type_t *type);

// token.c
token_t *token_new(lex_t *lex, int kind);
void token_free(token_t *token);
token_t *token_dup(lex_t *lex, token_t *token);
void token_add_hideset(token_t *token, macro_t *macro);
bool token_exists_hideset(token_t *token, macro_t *macro);
void token_union_hideset(token_t *token, vector_t *hideset);
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
node_t *node_new_int(parse_t *parse, type_t *type, long ival);
node_t *node_new_float(parse_t *parse, type_t *type, double fval, int fid);
node_t *node_new_string(parse_t *parse, string_t *sval, int sid);
node_t *node_new_init_list(parse_t *parse, type_t *type, vector_t *init);
node_t *node_new_variable(parse_t *parse, type_t *type, char *vname, int sclass, bool global);
node_t *node_new_declaration(parse_t *parse, type_t *type, node_t *var, node_t *init);
node_t *node_new_binary_op(parse_t *parse, type_t *type, int op, node_t *left, node_t *right);
node_t *node_new_unary_op(parse_t *parse, type_t *type, int op, node_t *operand);
node_t *node_new_call(parse_t *parse, type_t *type, node_t *func, vector_t *args);
node_t *node_new_block(parse_t *parse, int kind, vector_t *statements, node_t *parent);
node_t *node_new_if(parse_t *parse, node_t *cond, node_t *then_body, node_t *else_body);
node_t *node_new_function(parse_t *parse, node_t *fvar, vector_t *fargs, node_t *fbody);
node_t *node_new_continue(parse_t *parse);
node_t *node_new_break(parse_t *parse);
node_t *node_new_return(parse_t *parse, type_t *type, node_t *retval);
node_t *node_new_while(parse_t *parse, node_t *cond, node_t *body);
node_t *node_new_do(parse_t *parse, node_t *cond, node_t *body);
node_t *node_new_for(parse_t *parse, node_t *init, node_t *cond, node_t *step, node_t *body);
node_t *node_new_switch(parse_t *parse, node_t *expr, node_t *body);
node_t *node_new_case(parse_t *parse, node_t *val, node_t *body);
void node_free(node_t *node);
void node_debug(node_t *node);

// parse.c
void parse_free(parse_t *parse);
parse_t *parse_file(FILE *fp);

// macro.c
macro_t *macro_new();
void macro_free(macro_t *m);

// cpp.c
token_t *cpp_get_token(parse_t *parse);
void cpp_unget_token(parse_t *parse, token_t *token);
token_t *cpp_next_token_is(parse_t *parse, int kind);
token_t *cpp_expect_token_is(parse_t *parse, int k);
token_t *cpp_next_keyword_is(parse_t *parse, int k);
void cpp_expect_keyword_is(parse_t *parse, int k);

// gen.c
void gen(parse_t *parse);

// error.c
noreturn void errorf(char *fmt, ...);
void warnf(char *fmt, ...);

#endif  // HCC_H_
