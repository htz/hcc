// Copyright 2019 @htz. Released under the MIT license.

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hcc.h"

static type_t *declaration_specifier(parse_t *parse);
static node_t *variable_declarator(parse_t *parse, type_t *type);
static type_t *declarator(parse_t *parse, type_t *type, char **namep);
static type_t *direct_declarator(parse_t *parse, type_t *type, char **namep);
static node_t *additive_expression(parse_t *parse);
static node_t *multiplicative_expression(parse_t *parse);
static node_t *unary_expression(parse_t *parse);
static vector_t *func_args(parse_t *parse);
static node_t *postfix_expression(parse_t *parse);
static node_t *primary_expression(parse_t *parse);
static node_t *expression(parse_t *parse);
static node_t *assignment_expression(parse_t *parse);
static node_t *declaration(parse_t *parse, type_t *type);
static node_t *init_declarator(parse_t *parse, type_t *type);
static node_t *initializer(parse_t *parse, type_t *type);
static node_t *statement(parse_t *parse);
static node_t *expression_statement(parse_t *parse);

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

static type_t *op_result(parse_t *parse, int op, type_t *left, type_t *right) {
  if (left == right) {
    return left;
  }
  if (left->kind > right->kind) {
    type_t *tmp = left;
    left = right;
    right = tmp;
  }
  type_t *type = NULL;
  switch (left->kind) {
  case TYPE_KIND_CHAR:
    switch (right->kind) {
    case TYPE_KIND_INT:
      type = right;
      break;
    case TYPE_KIND_PTR:
    case TYPE_KIND_ARRAY:
      if (op != '+' && op != '-' && op != '=') {
        errorf("invalid operands to binary %c", op);
      }
      type = right;
      break;
    default:
      errorf("unknown type: %s", right->name);
    }
    break;
  case TYPE_KIND_INT:
    switch (right->kind) {
    case TYPE_KIND_PTR:
    case TYPE_KIND_ARRAY:
      if (op != '+' && op != '-') {
        errorf("invalid operands to binary %c", op);
      }
      type = right;
      break;
    default:
      errorf("unknown type: %s", right->name);
    }
    break;
  }
  if (type == NULL) {
    errorf("invalid operands to binary expression ('%s' and '%s')", left->name, right->name);
  }
  return type;
}

static type_t *declaration_specifier(parse_t *parse) {
  token_t *token = lex_next_token_is(parse->lex, TOKEN_KIND_IDENTIFIER);
  if (token == NULL) {
    return NULL;
  }
  type_t *type = type_get(parse, token->identifier, NULL);
  if (type == NULL) {
    lex_unget_token(parse->lex, token);
  }
  return type;
}

static type_t *declarator_array(parse_t *parse, type_t *type) {
  if (lex_next_keyword_is(parse->lex, '[')) {
    node_t *size = expression(parse);
    if (size->kind != NODE_KIND_LITERAL || size->type->kind != TYPE_KIND_INT) {
      errorf("integer expected, but got %d", size->kind);
    }
    lex_expect_keyword_is(parse->lex, ']');
    type_t *parent_type = declarator_array(parse, type);
    type = type_make_array(parse, parent_type, size->ival);
  }

  return type;
}

static node_t *variable_declarator(parse_t *parse, type_t *type) {
  char *name;
  type = declarator(parse, type, &name);
  return node_new_variable(parse, type, name);
}

static type_t *declarator(parse_t *parse, type_t *type, char **namep) {
  while (lex_next_keyword_is(parse->lex, '*')) {
    type = type_get_ptr(parse, type);
  }
  return direct_declarator(parse, type, namep);
}

static type_t *direct_declarator(parse_t *parse, type_t *type, char **namep) {
  if (lex_next_keyword_is(parse->lex, '(')) {
    type = declarator(parse, type, namep);
    lex_expect_keyword_is(parse->lex, ')');
    return type;
  }

  token_t *token = lex_expect_token_is(parse->lex, TOKEN_KIND_IDENTIFIER);
  *namep = token->identifier;
  if (lex_next_keyword_is(parse->lex, '[')) {
    node_t *size = NULL;
    if (!lex_next_keyword_is(parse->lex, ']')) {
      size = expression(parse);
      if (size->kind != NODE_KIND_LITERAL || size->type->kind != TYPE_KIND_INT) {
        errorf("integer expected, but got %d", size->kind);
      }
      lex_expect_keyword_is(parse->lex, ']');
    }
    type_t *parent_type = declarator_array(parse, type);
    if (size == NULL) {
      type = type_make_array(parse, parent_type, -1);
    } else {
      type = type_make_array(parse, parent_type, size->ival);
    }
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
    node_t *right = multiplicative_expression(parse);
    type_t *type = op_result(parse, op, node->type, right->type);
    if (
      (node->type->kind != TYPE_KIND_PTR && node->type->kind != TYPE_KIND_ARRAY) &&
      (right->type->kind == TYPE_KIND_PTR || right->type->kind == TYPE_KIND_ARRAY)
    ) {
      node_t *tmp = node;
      node = right;
      right = tmp;
    }
    node = node_new_binary_op(parse, type, op, node, right);
  }
  return node;
}

static node_t *multiplicative_expression(parse_t *parse) {
  node_t *node = unary_expression(parse);
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
    node_t *right = unary_expression(parse);
    type_t *type = op_result(parse, op, node->type, right->type);
    node = node_new_binary_op(parse, type, op, node, right);
  }
  return node;
}

static node_t *unary_expression(parse_t *parse) {
  if (lex_next_keyword_is(parse->lex, '*')) {
    node_t *node = unary_expression(parse);
    if (node->type->parent == NULL) {
      errorf("pointer type expected, but got %s", node->type->name);
    }
    return node_new_unary_op(parse, node->type->parent, '*', node);
  } else if (lex_next_keyword_is(parse->lex, '&')) {
    node_t *node = unary_expression(parse);
    type_t *type = type_get_ptr(parse, node->type);
    return node_new_unary_op(parse, type, '&', node);
  }
  return postfix_expression(parse);
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
  for (;;) {
    if (lex_next_keyword_is(parse->lex, '(')) {
      vector_t *args = func_args(parse);
      node = node_new_call(parse, parse->type_int, node, args);
    } else if (node->kind == NODE_KIND_IDENTIFIER) {
      node_t *var = find_variable(parse, node->identifier);
      if (var == NULL) {
        errorf("Undefined varaible: %s", node->identifier);
      }
      node = var;
    } else if (lex_next_keyword_is(parse->lex, '[')) {
      if (node->type->parent == NULL) {
        errorf("pointer or array type expected, but got %s", node->type->name);
      }
      type_t *type = type_get_ptr(parse, node->type->parent);
      node = node_new_binary_op(parse, type, '+', node, expression(parse));
      lex_expect_keyword_is(parse->lex, ']');
      node = node_new_unary_op(parse, type->parent, '*', node);
    } else {
      break;
    }
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
    return node_new_int(parse, parse->type_int, token->ival);
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
    node_t *right = assignment_expression(parse);
    node = node_new_binary_op(parse, right->type, '=', node, right);
  }
  return node;
}

static node_t *declaration(parse_t *parse, type_t *type) {
  node_t *node = init_declarator(parse, type);
  node_t *prev = node;
  while (lex_next_keyword_is(parse->lex, ',')) {
    prev->next = init_declarator(parse, type);
    prev = prev->next;
  }
  lex_expect_keyword_is(parse->lex, ';');
  return node;
}

static node_t *init_declarator(parse_t *parse, type_t *type) {
  node_t *var = variable_declarator(parse, type);
  node_t *init = NULL;
  if (lex_next_keyword_is(parse->lex, '=')) {
    init = initializer(parse, var->type);
  }
  if (var->type->kind == TYPE_KIND_ARRAY && var->type->size < 0) {
    if (init == NULL) {
      errorf("definition of variable with array type needs an explicit size or an initializer");
    }
    if (init->type->kind != TYPE_KIND_ARRAY) {
      errorf("array initializer must be an initializer list");
    }
    assert(var->type->parent != NULL);
    if (init->kind == NODE_KIND_STRING_LITERAL) {
      var->type = type_make_array(parse, var->type->parent, init->type->size);
    } else {
      var->type = type_make_array(parse, var->type->parent, init->init_list->size);
    }
  }
  add_var(parse, var);
  return node_new_declaration(parse, var->type, var, init);
}

static node_t *initializer(parse_t *parse, type_t *type) {
  node_t *init;
  if (lex_next_keyword_is(parse->lex, '{')) {
    vector_t *nodes = vector_new();
    if (type->parent == NULL) {
      errorf("pointer or array type expected, but got %s", type->name);
    }
    while (!lex_next_keyword_is(parse->lex, '}')) {
      node_t *node = initializer(parse, type->parent);
      vector_push(nodes, node);
      if (!lex_next_keyword_is(parse->lex, ',')) {
        lex_expect_keyword_is(parse->lex, '}');
        break;
      }
    }
    init = node_new_init_list(parse, type, nodes);
  } else {
    init = assignment_expression(parse);
    if (!type_is_assignable(type, init->type)) {
      errorf("different type of initialzation value, %s expected but got %s", type->name, init->type->name);
    }
  }
  return init;
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
  parse->types = map_new();

  parse->type_char = type_new("char", TYPE_KIND_CHAR, NULL);
  type_add(parse, parse->type_char->name, parse->type_char);
  parse->type_int = type_new("int", TYPE_KIND_INT, NULL);
  type_add(parse, parse->type_int->name, parse->type_int);

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
  parse->types->free_val_fn = (void (*)(void *))type_free;
  map_free(parse->types);
  free(parse);
}

parse_t *parse_file(FILE *fp) {
  parse_t *parse = parse_new(fp);
  for (;;) {
    if (lex_next_token_is(parse->lex, TOKEN_KIND_EOF)) {
      break;
    }
    type_t *type = declaration_specifier(parse);
    if (type != NULL) {
      vector_push(parse->statements, declaration(parse, type));
    } else {
      vector_push(parse->statements, statement(parse));
    }
  }
  return parse;
}
