// Copyright 2019 @htz. Released under the MIT license.

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hcc.h"

static int declaration_specifier(parse_t *parse);
static node_t *additive_expression(parse_t *parse);
static node_t *multiplicative_expression(parse_t *parse);
static vector_t *func_args(parse_t *parse);
static node_t *postfix_expression(parse_t *parse);
static node_t *primary_expression(parse_t *parse);
static node_t *expression(parse_t *parse);
static node_t *assignment_expression(parse_t *parse);
static node_t *declaration(parse_t *parse, int type);
static node_t *init_declarator(parse_t *parse, int type);
static node_t *statement(parse_t *parse);
static node_t *expression_statement(parse_t *parse);

static int get_type(char *name) {
  if (strcmp(name, "int") == 0) {
    return TYPE_INT;
  }
  if (strcmp(name, "char") == 0) {
    return TYPE_CHAR;
  }
  return -1;
}

static void add_var(parse_t *parse, node_t *var) {
  map_entry_t *e = map_find(parse->vars, var->vname);
  if (e != NULL) {
    errorf("already defined: %s", var->vname);
  }
  map_add(parse->vars, var->vname, var);
}

static node_t *find_variable(parse_t *parse, char *identifier) {
  map_entry_t *e = map_find(parse->vars, identifier);
  if (e == NULL) {
    return NULL;
  }
  return (node_t *)e->val;
}

static int declaration_specifier(parse_t *parse) {
  token_t *token = lex_next_token_is(parse->lex, TOKEN_KIND_IDENTIFIER);
  if (token == NULL) {
    return -1;
  }
  int type = get_type(token->identifier);
  if (type == -1) {
    lex_unget_token(parse->lex, token);
  }
  return type;
}

static node_t *additive_expression(parse_t *parse) {
  node_t *node = multiplicative_expression(parse);
  for (;;) {
    int op;
    if (lex_next_keyword_is(parse->lex, '+')) {
      op = '+';
    } else if (lex_next_keyword_is(parse->lex, '-')) {
      op = '-';
    } else {
      break;
    }
    node = node_new_binary_op(parse, op, node, multiplicative_expression(parse));
  }
  return node;
}

static node_t *multiplicative_expression(parse_t *parse) {
  node_t *node = postfix_expression(parse);
  for (;;) {
    int op;
    if (lex_next_keyword_is(parse->lex, '*')) {
      op = '*';
    } else if (lex_next_keyword_is(parse->lex, '/')) {
      op = '/';
    } else if (lex_next_keyword_is(parse->lex, '%')) {
      op = '%';
    } else {
      break;
    }
    node = node_new_binary_op(parse, op, node, postfix_expression(parse));
  }
  return node;
}

static vector_t *func_args(parse_t *parse) {
  vector_t *args = vector_new();
  if (lex_next_keyword_is(parse->lex, ')')) {
    return args;
  }
  for (;;) {
    node_t *arg = assignment_expression(parse);
    vector_push(args, arg);
    if (lex_next_keyword_is(parse->lex, ')')) {
      break;
    }
    lex_expect_keyword_is(parse->lex, ',');
  }
  return args;
}

static node_t *postfix_expression(parse_t *parse) {
  node_t *node = primary_expression(parse);
  if (lex_next_keyword_is(parse->lex, '(')) {
    vector_t *args = func_args(parse);
    return node_new_call(parse, node, args);
  }
  if (node->kind == NODE_KIND_IDENTIFIER) {
    node_t *var = find_variable(parse, node->identifier);
    if (var == NULL) {
      errorf("Undefined varaible: %s", node->identifier);
    }
    node = var;
  }
  return node;
}

static node_t *primary_expression(parse_t *parse) {
  token_t *token = lex_get_token(parse->lex);
  node_t *node;
  switch (token->kind) {
  case TOKEN_KIND_KEYWORD:
    if (token->keyword == '(') {
      node = expression(parse);
      lex_expect_keyword_is(parse->lex, ')');
      return node;
    }
    errorf("unknown keyword");
  case TOKEN_KIND_IDENTIFIER:
    return node_new_identifier(parse, token->identifier);
  case TOKEN_KIND_CHAR:
  case TOKEN_KIND_INT:
    return node_new_int(parse, token->ival);
  case TOKEN_KIND_STRING:
    node = node_new_string(parse, token->sval, parse->data->size);
    for (;;) {
      token = lex_get_token(parse->lex);
      if (token->kind != TOKEN_KIND_STRING) {
        lex_unget_token(parse->lex, token);
        break;
      }
      string_append(node->sval, token->sval->buf);
    }
    vector_push(parse->data, node);
    return node;
  }
  errorf("unknown token: %s", token_str(token));
}

static node_t *expression(parse_t *parse) {
  node_t *node = assignment_expression(parse);
  node_t *prev = node;
  while (lex_next_keyword_is(parse->lex, ',')) {
    prev->next = assignment_expression(parse);
    prev = prev->next;
  }
  return node;
}

static node_t *assignment_expression(parse_t *parse) {
  node_t *node = additive_expression(parse);
  for (;;) {
    if (!lex_next_keyword_is(parse->lex, '=')) {
      break;
    }
    node = node_new_binary_op(parse, '=', node, assignment_expression(parse));
  }
  return node;
}

static node_t *declaration(parse_t *parse, int type) {
  node_t *node = init_declarator(parse, type);
  node_t *prev = node;
  while (lex_next_keyword_is(parse->lex, ',')) {
    prev->next = init_declarator(parse, type);
    prev = prev->next;
  }
  lex_expect_keyword_is(parse->lex, ';');
  return node;
}

static node_t *init_declarator(parse_t *parse, int type) {
  token_t *token = lex_expect_token_is(parse->lex, TOKEN_KIND_IDENTIFIER);
  node_t *var = node_new_variable(parse, token->identifier);
  add_var(parse, var);
  if (lex_next_keyword_is(parse->lex, '=')) {
    return node_new_declaration(parse, type, var, assignment_expression(parse));
  } else {
    return node_new_declaration(parse, type, var, NULL);
  }
}

static node_t *statement(parse_t *parse) {
  return expression_statement(parse);
}

static node_t *expression_statement(parse_t *parse) {
  if (lex_next_keyword_is(parse->lex, ';')) {
    return node_new_nop(parse);
  }
  node_t *node = expression(parse);
  lex_expect_keyword_is(parse->lex, ';');
  return node;
}

static parse_t *parse_new(FILE *fp) {
  parse_t *parse = (parse_t *)malloc(sizeof (parse_t));
  parse->lex = lex_new(fp);
  parse->data = vector_new();
  parse->statements = vector_new();
  parse->nodes = vector_new();
  parse->vars = map_new();
  return parse;
}

void parse_free(parse_t *parse) {
  lex_free(parse->lex);
  vector_free(parse->data);
  vector_free(parse->statements);
  for (int i = 0; i < parse->nodes->size; i++) {
    node_free((node_t *)parse->nodes->data[i]);
  }
  vector_free(parse->nodes);
  map_free(parse->vars);
  free(parse);
}

parse_t *parse_file(FILE *fp) {
  parse_t *parse = parse_new(fp);
  for (;;) {
    if (lex_next_token_is(parse->lex, TOKEN_KIND_EOF)) {
      break;
    }
    int type = declaration_specifier(parse);
    if (type != -1) {
      vector_push(parse->statements, declaration(parse, type));
    } else {
      vector_push(parse->statements, statement(parse));
    }
  }
  return parse;
}
