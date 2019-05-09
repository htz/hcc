// Copyright 2019 @htz. Released under the MIT license.

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hcc.h"

static node_t *external_declaration(parse_t *parse);
static node_t *function_definition(parse_t *parse, node_t *var);
static type_t *declaration_specifier(parse_t *parse);
static node_t *variable_declarator(parse_t *parse, type_t *type);
static type_t *declarator(parse_t *parse, type_t *type, char **namep);
static type_t *direct_declarator(parse_t *parse, type_t *type, char **namep);
static node_t *conditional_expression(parse_t *parse);
static node_t *logical_or_expression(parse_t *parse);
static node_t *logical_and_expression(parse_t *parse);
static node_t *inclusive_or_expression(parse_t *parse);
static node_t *exclusive_or_expression(parse_t *parse);
static node_t *and_expression(parse_t *parse);
static node_t *equality_expression(parse_t *parse);
static node_t *relational_expression(parse_t *parse);
static node_t *shift_expression(parse_t *parse);
static node_t *additive_expression(parse_t *parse);
static node_t *multiplicative_expression(parse_t *parse);
static node_t *unary_expression(parse_t *parse);
static vector_t *func_args(parse_t *parse);
static node_t *postfix_expression(parse_t *parse);
static node_t *primary_expression(parse_t *parse);
static node_t *expression(parse_t *parse);
static node_t *assignment_expression(parse_t *parse);
static node_t *declaration(parse_t *parse, type_t *type);
static node_t *init_declarator(parse_t *parse, node_t *var);
static node_t *initializer(parse_t *parse, type_t *type);
static node_t *compound_statement(parse_t *parse);
static node_t *statement(parse_t *parse);
static node_t *expression_statement(parse_t *parse);
static node_t *selection_statement(parse_t *parse);
static node_t *jump_statement(parse_t *parse, int keyword);

static void add_var(parse_t *parse, node_t *var) {
  map_t *vars;
  if (parse->current_scope) {
    vars = parse->current_scope->vars;
  } else {
    vars = parse->vars;
  }
  map_entry_t *e = map_find(vars, var->vname);
  if (e != NULL) {
    errorf("already defined: %s", var->vname);
  }
  map_add(vars, var->vname, var);
}

static node_t *find_variable(parse_t *parse, node_t *scope, char *identifier) {
  map_entry_t *e;
  if (scope) {
    e = map_find(scope->vars, identifier);
    if (e == NULL) {
      return find_variable(parse, scope->parent_block, identifier);
    }
  } else {
    e = map_find(parse->vars, identifier);
    if (e == NULL) {
      return NULL;
    }
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
      if (
        op != '+' && op != ('+' | OP_ASSIGN_MASK) &&
        op != '-' && op != ('-' | OP_ASSIGN_MASK) &&
        op != '='
      ) {
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

static node_t *external_declaration(parse_t *parse) {
  type_t *type = declaration_specifier(parse);
  if (type == NULL) {
    errorf("unknown type name");
  }
  if (lex_next_keyword_is(parse->lex, ';')) {
    return node_new_nop(parse);
  }

  node_t *var = variable_declarator(parse, type);
  if (lex_next_keyword_is(parse->lex, '(')) {
    return function_definition(parse, var);
  }

  node_t *node = init_declarator(parse, var);
  node_t *prev = node;
  while (lex_next_keyword_is(parse->lex, ',')) {
    var = variable_declarator(parse, type);
    prev->next = init_declarator(parse, var);
    prev = prev->next;
  }
  lex_expect_keyword_is(parse->lex, ';');
  return node;
}

static node_t *function_parameter_declaration(parse_t *parse) {
  type_t *type = declaration_specifier(parse);
  if (type == NULL) {
    errorf("unknown type name");
  }
  return variable_declarator(parse, type);
}

static node_t *function_definition(parse_t *parse, node_t *var) {
  vector_t *fargs = vector_new();
  node_t *node = node_new_function(parse, var, fargs, NULL);
  parse->current_function = node;
  token_t *token = lex_next_token_is(parse->lex, TOKEN_KIND_IDENTIFIER);
  bool empty_args = false;
  if (token != NULL) {
    if (strcmp("void", token->identifier) == 0 && lex_next_keyword_is(parse->lex, ')')) {
      empty_args = true;
    } else {
      lex_unget_token(parse->lex, token);
    }
  }
  if (!empty_args && !lex_next_keyword_is(parse->lex, ')')) {
    for (;;) {
      node_t *arg = function_parameter_declaration(parse);
      if (arg->type->kind == TYPE_KIND_VOID) {
        errorf("void is not allowed");
      }
      add_var(parse, arg);
      vector_push(fargs, arg);
      if (lex_next_keyword_is(parse->lex, ')')) {
        break;
      }
      lex_expect_keyword_is(parse->lex, ',');
    }
  }
  lex_expect_keyword_is(parse->lex, '{');
  node->fbody = compound_statement(parse);
  parse->current_function = NULL;
  return node;
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
  if (parse->current_function) {
    return node_new_variable(parse, type, name, false);
  }
  return node_new_variable(parse, type, name, true);
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

static node_t *conditional_expression(parse_t *parse) {
  node_t *node = logical_or_expression(parse);
  if (lex_next_keyword_is(parse->lex, '?')) {
    node_t *then_body = expression(parse);
    lex_expect_keyword_is(parse->lex, ':');
    node_t *else_body = conditional_expression(parse);
    if (!type_is_assignable(then_body->type, else_body->type)) {
      errorf("incompatible operand types ('%s' and '%s')", then_body->type->name, else_body->type->name);
    }
    node = node_new_if(parse, node, then_body, else_body);
    node->type = then_body->type;
  }
  return node;
}

static node_t *logical_or_expression(parse_t *parse) {
  node_t *node = logical_and_expression(parse);
  for (;;) {
    if (!lex_next_keyword_is(parse->lex, OP_OROR)) {
      break;
    }
    node_t *right = logical_and_expression(parse);
    node = node_new_binary_op(parse, parse->type_int, OP_OROR, node, right);
  }
  return node;
}

static node_t *logical_and_expression(parse_t *parse) {
  node_t *node = inclusive_or_expression(parse);
  for (;;) {
    if (!lex_next_keyword_is(parse->lex, OP_ANDAND)) {
      break;
    }
    node_t *right = inclusive_or_expression(parse);
    node = node_new_binary_op(parse, parse->type_int, OP_ANDAND, node, right);
  }
  return node;
}

static node_t *inclusive_or_expression(parse_t *parse) {
  node_t *node = exclusive_or_expression(parse);
  for (;;) {
    if (!lex_next_keyword_is(parse->lex, '|')) {
      break;
    }
    node_t *right = exclusive_or_expression(parse);
    node = node_new_binary_op(parse, parse->type_int, '|', node, right);
  }
  return node;
}

static node_t *exclusive_or_expression(parse_t *parse) {
  node_t *node = and_expression(parse);
  for (;;) {
    if (!lex_next_keyword_is(parse->lex, '^')) {
      break;
    }
    node_t *right = and_expression(parse);
    node = node_new_binary_op(parse, parse->type_int, '^', node, right);
  }
  return node;
}

static node_t *and_expression(parse_t *parse) {
  node_t *node = equality_expression(parse);
  for (;;) {
    if (!lex_next_keyword_is(parse->lex, '&')) {
      break;
    }
    node_t *right = equality_expression(parse);
    node = node_new_binary_op(parse, parse->type_int, '&', node, right);
  }
  return node;
}

static node_t *equality_expression(parse_t *parse) {
  node_t *node = relational_expression(parse);
  for (;;) {
    int op;
    if (lex_next_keyword_is(parse->lex, OP_EQ)) {
      op = OP_EQ;
    } else if (lex_next_keyword_is(parse->lex, OP_NE)) {
      op = OP_NE;
    } else {
      break;
    }
    node_t *right = relational_expression(parse);
    node = node_new_binary_op(parse, parse->type_int, op, node, right);
  }
  return node;
}

static node_t *relational_expression(parse_t *parse) {
  node_t *node = shift_expression(parse);
  for (;;) {
    int op;
    if (lex_next_keyword_is(parse->lex, '<')) {
      op = '<';
    } else if (lex_next_keyword_is(parse->lex, OP_LE)) {
      op = OP_LE;
    } else if (lex_next_keyword_is(parse->lex, '>')) {
      op = '>';
    } else if (lex_next_keyword_is(parse->lex, OP_GE)) {
      op = OP_GE;
    } else {
      break;
    }
    node_t *right = shift_expression(parse);
    node = node_new_binary_op(parse, parse->type_int, op, node, right);
  }
  return node;
}

static node_t *shift_expression(parse_t *parse) {
  node_t *node = additive_expression(parse);
  for (;;) {
    int op;
    if (lex_next_keyword_is(parse->lex, OP_SAL)) {
      op = OP_SAL;
    } else if (lex_next_keyword_is(parse->lex, OP_SAR)) {
      op = OP_SAR;
    } else {
      break;
    }
    node_t *right = additive_expression(parse);
    node = node_new_binary_op(parse, node->type, op, node, right);
  }
  return node;
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
  if (lex_next_keyword_is(parse->lex, OP_INC)) {
    node_t *node = unary_expression(parse);
    if (node->kind == NODE_KIND_VARIABLE || (node->kind == NODE_KIND_UNARY_OP && node->op == '*')) {
      return node_new_unary_op(parse, node->type, OP_INC, node);
    } else {
      errorf("expression is not assignable");
    }
  } else if (lex_next_keyword_is(parse->lex, OP_DEC)) {
    node_t *node = unary_expression(parse);
    if (node->kind == NODE_KIND_VARIABLE || (node->kind == NODE_KIND_UNARY_OP && node->op == '*')) {
      return node_new_unary_op(parse, node->type, OP_DEC, node);
    } else {
      errorf("expression is not assignable");
    }
  } else if (lex_next_keyword_is(parse->lex, '+')) {
    node_t *node = unary_expression(parse);
    if (node->kind == NODE_KIND_LITERAL && node->type->kind == TYPE_KIND_INT) {
      return node;
    }
    return node_new_unary_op(parse, node->type, '+', node);
  } else if (lex_next_keyword_is(parse->lex, '-')) {
    node_t *node = unary_expression(parse);
    if (node->kind == NODE_KIND_LITERAL && node->type->kind == TYPE_KIND_INT) {
      node->ival *= -1;
      return node;
    }
    return node_new_unary_op(parse, node->type, '-', node);
  } else if (lex_next_keyword_is(parse->lex, '~')) {
    node_t *node = unary_expression(parse);
    if (node->kind == NODE_KIND_LITERAL && node->type->kind == TYPE_KIND_INT) {
      node->ival = ~node->ival;
      return node;
    }
    return node_new_unary_op(parse, node->type, '~', node);
  } else if (lex_next_keyword_is(parse->lex, '!')) {
    node_t *node = unary_expression(parse);
    if (node->kind == NODE_KIND_LITERAL && node->type->kind == TYPE_KIND_INT) {
      node->ival = !node->ival;
      return node;
    }
    return node_new_unary_op(parse, node->type, '!', node);
  } else if (lex_next_keyword_is(parse->lex, '*')) {
    node_t *node = unary_expression(parse);
    if (node->type->parent == NULL) {
      errorf("pointer type expected, but got %s", node->type->name);
    }
    return node_new_unary_op(parse, node->type->parent, '*', node);
  } else if (lex_next_keyword_is(parse->lex, '&')) {
    node_t *node = unary_expression(parse);
    type_t *type = type_get_ptr(parse, node->type);
    if (node->kind == NODE_KIND_VARIABLE || (node->kind == NODE_KIND_UNARY_OP && node->op == '*')) {
      return node_new_unary_op(parse, type, '&', node);
    } else {
      errorf("cannot take the address of an rvalue of type '%s'", type->name);
    }
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
      node_t *var = find_variable(parse, parse->current_scope, node->identifier);
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
    } else if (lex_next_keyword_is(parse->lex, OP_INC)) {
      if (node->kind == NODE_KIND_VARIABLE || (node->kind == NODE_KIND_UNARY_OP && node->op == '*')) {
        node = node_new_unary_op(parse, node->type, OP_PINC, node);
      } else {
        errorf("expression is not assignable");
      }
    } else if (lex_next_keyword_is(parse->lex, OP_DEC)) {
      if (node->kind == NODE_KIND_VARIABLE || (node->kind == NODE_KIND_UNARY_OP && node->op == '*')) {
        node = node_new_unary_op(parse, node->type, OP_PDEC, node);
      } else {
        errorf("expression is not assignable");
      }
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
    if (parse->current_function) {
      node = node_new_string(parse, token->sval, parse->data->size);
      vector_push(parse->data, node);
    } else {
      node = node_new_string(parse, token->sval, -1);
    }
    for (;;) {
      token = lex_get_token(parse->lex);
      if (token->kind != TOKEN_KIND_STRING) {
        lex_unget_token(parse->lex, token);
        break;
      }
      string_append(node->sval, token->sval->buf);
    }
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
  node_t *node = conditional_expression(parse);
  for (;;) {
    int op;
    if (lex_next_keyword_is(parse->lex, '=')) {
      op = '=';
    } else if (lex_next_keyword_is(parse->lex, '+' | OP_ASSIGN_MASK)) {
      op = '+' | OP_ASSIGN_MASK;
    } else if (lex_next_keyword_is(parse->lex, '-' | OP_ASSIGN_MASK)) {
      op = '-' | OP_ASSIGN_MASK;
    } else if (lex_next_keyword_is(parse->lex, '*' | OP_ASSIGN_MASK)) {
      op = '*' | OP_ASSIGN_MASK;
    } else if (lex_next_keyword_is(parse->lex, '/' | OP_ASSIGN_MASK)) {
      op = '/' | OP_ASSIGN_MASK;
    } else if (lex_next_keyword_is(parse->lex, '%' | OP_ASSIGN_MASK)) {
      op = '%' | OP_ASSIGN_MASK;
    } else if (lex_next_keyword_is(parse->lex, '&' | OP_ASSIGN_MASK)) {
      op = '&' | OP_ASSIGN_MASK;
    } else if (lex_next_keyword_is(parse->lex, '|' | OP_ASSIGN_MASK)) {
      op = '|' | OP_ASSIGN_MASK;
    } else if (lex_next_keyword_is(parse->lex, '^' | OP_ASSIGN_MASK)) {
      op = '^' | OP_ASSIGN_MASK;
    } else if (lex_next_keyword_is(parse->lex, OP_SAL | OP_ASSIGN_MASK)) {
      op = OP_SAL | OP_ASSIGN_MASK;
    } else if (lex_next_keyword_is(parse->lex, OP_SAR | OP_ASSIGN_MASK)) {
      op = OP_SAR | OP_ASSIGN_MASK;
    } else {
      break;
    }
    if (node->kind == NODE_KIND_VARIABLE || (node->kind == NODE_KIND_UNARY_OP && node->op == '*')) {
      node_t *right = assignment_expression(parse);
      type_t *type = op_result(parse, op, node->type, right->type);
      node = node_new_binary_op(parse, type, op, node, right);
    } else {
      errorf("expression is not assignable");
    }
  }
  return node;
}

static node_t *declaration(parse_t *parse, type_t *type) {
  node_t *var = variable_declarator(parse, type);
  node_t *node = init_declarator(parse, var);
  node_t *prev = node;
  while (lex_next_keyword_is(parse->lex, ',')) {
    var = variable_declarator(parse, type);
    prev->next = init_declarator(parse, var);
    prev = prev->next;
  }
  lex_expect_keyword_is(parse->lex, ';');
  return node;
}

static node_t *init_declarator(parse_t *parse, node_t *var) {
  node_t *init = NULL;
  if (var->type->kind == TYPE_KIND_VOID) {
    errorf("void is not allowed");
  }
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

static node_t *compound_statement(parse_t *parse) {
  vector_t *statements = vector_new();
  node_t *node = node_new_block(parse, statements, parse->current_scope);
  parse->current_scope = node;
  while (!lex_next_keyword_is(parse->lex, '}')) {
    type_t *type = declaration_specifier(parse);
    if (type != NULL) {
      if (lex_next_keyword_is(parse->lex, ';')) {
        continue;
      }
      vector_push(statements, declaration(parse, type));
    } else {
      vector_push(statements, statement(parse));
    }
  }
  parse->current_scope = node->parent_block;
  return node;
}

static node_t *statement(parse_t *parse) {
  if (lex_next_keyword_is(parse->lex, TOKEN_KEYWORD_IF)) {
    return selection_statement(parse);
  }
  if (lex_next_keyword_is(parse->lex, TOKEN_KEYWORD_RETURN)) {
    return jump_statement(parse, TOKEN_KEYWORD_RETURN);
  }
  if (lex_next_keyword_is(parse->lex, '{')) {
    return compound_statement(parse);
  }
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

static node_t *selection_statement(parse_t *parse) {
  lex_expect_keyword_is(parse->lex, '(');
  node_t *cond = expression(parse);
  lex_expect_keyword_is(parse->lex, ')');
  node_t *then_body = statement(parse);
  node_t *else_body = NULL;
  if (lex_next_keyword_is(parse->lex, TOKEN_KEYWORD_ELSE)) {
    else_body = statement(parse);
  }
  return node_new_if(parse, cond, then_body, else_body);
}

static node_t *jump_statement(parse_t *parse, int keyword) {
  assert(parse->current_function != NULL);
  if (keyword == TOKEN_KEYWORD_RETURN) {
    node_t *var = parse->current_function->fvar;
    assert(var != NULL);
    type_t *type = var->type;
    node_t *exp = NULL;
    if (!lex_next_keyword_is(parse->lex, ';')) {
      if (type->kind == TYPE_KIND_VOID) {
        errorf("void function '%s' should not return a value", var->vname);
      }
      exp = expression(parse);
      if (!type_is_assignable(exp->type, type)) {
        errorf("different returning expression type from '%s' to '%s'", exp->type->name, type->name);
      }
      lex_expect_keyword_is(parse->lex, ';');
    } else {
      if (type->kind != TYPE_KIND_VOID) {
        errorf("non-void function '%s' should return a value", var->vname);
      }
    }
    return node_new_return(parse, parse->current_function->type, exp);
  }
  errorf("not implemented");
}

static parse_t *parse_new(FILE *fp) {
  parse_t *parse = (parse_t *)malloc(sizeof (parse_t));
  parse->lex = lex_new(fp);
  parse->data = vector_new();
  parse->statements = vector_new();
  parse->nodes = vector_new();
  parse->vars = map_new();
  parse->types = map_new();
  parse->current_scope = NULL;

  parse->type_void = type_new("void", TYPE_KIND_VOID, NULL);
  map_add(parse->types, parse->type_void->name, parse->type_void);
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
    vector_push(parse->statements, external_declaration(parse));
  }
  return parse;
}
