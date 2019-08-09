// Copyright 2019 @htz. Released under the MIT license.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hcc.h"

static node_t *node_new(parse_t *parse, int kind) {
  node_t *node = (node_t *)malloc(sizeof (node_t));
  node->kind = kind;
  node->next = NULL;
  vector_push(parse->nodes, node);
  return node;
}

node_t *node_new_nop(parse_t *parse) {
  return node_new(parse, NODE_KIND_NOP);
}

node_t *node_new_identifier(parse_t *parse, char *identifier) {
  node_t *node = node_new(parse, NODE_KIND_IDENTIFIER);
  node->type = NULL;
  node->identifier = strdup(identifier);
  return node;
}

node_t *node_new_int(parse_t *parse, type_t *type, int ival) {
  node_t *node = node_new(parse, NODE_KIND_LITERAL);
  node->type = type;
  node->ival = ival;
  return node;
}

node_t *node_new_string(parse_t *parse, string_t *sval, int sid) {
  node_t *node = node_new(parse, NODE_KIND_LITERAL);
  node->type = parse->type_string;
  node->sval = string_dup(sval);
  node->sid = sid;
  return node;
}

node_t *node_new_variable(parse_t *parse, type_t *type, char *vname) {
  node_t *node = node_new(parse, NODE_KIND_VARIABLE);
  node->type = type;
  node->vname = strdup(vname);
  node->voffset = 0;
  return node;
}

node_t *node_new_declaration(parse_t *parse, type_t *type, node_t *var, node_t *init) {
  node_t *node = node_new(parse, NODE_KIND_DECLARATION);
  node->type = type;
  node->dec_var = var;
  node->dec_init = init;
  return node;
}

node_t *node_new_binary_op(parse_t *parse, type_t *type, int op, node_t *left, node_t *right) {
  node_t *node = node_new(parse, NODE_KIND_BINARY_OP);
  node->type = type;
  node->op = op;
  node->left = left;
  node->right = right;
  return node;
}

node_t *node_new_unary_op(parse_t *parse, type_t *type, int op, node_t *operand) {
  node_t *node = node_new(parse, NODE_KIND_UNARY_OP);
  node->type = type;
  node->op = op;
  node->operand = operand;
  return node;
}

node_t *node_new_call(parse_t *parse, type_t *type, node_t *func, vector_t *args) {
  node_t *node = node_new(parse, NODE_KIND_CALL);
  node->type = type;
  node->func = func;
  node->args = args;
  return node;
}

void node_free(node_t *node) {
  switch (node->kind) {
  case NODE_KIND_IDENTIFIER:
    free(node->identifier);
    break;
  case NODE_KIND_LITERAL:
    switch (node->type->kind) {
      case TYPE_KIND_STRING:
        string_free(node->sval);
        break;
    }
    break;
  case NODE_KIND_VARIABLE:
    free(node->vname);
    break;
  case NODE_KIND_DECLARATION:
    break;
  case NODE_KIND_BINARY_OP:
    break;
  case NODE_KIND_UNARY_OP:
    break;
  case NODE_KIND_CALL:
    vector_free(node->args);
    break;
  }
  free(node);
}

void node_debug(node_t *node) {
  switch (node->kind) {
  case NODE_KIND_NOP:
    break;
  case NODE_KIND_IDENTIFIER:
    errorf("identifier node is internal use only");
  case NODE_KIND_LITERAL:
    switch (node->type->kind) {
      case TYPE_KIND_INT:
        printf("%d", node->ival);
        break;
      case TYPE_KIND_STRING:
        printf("\"");
        string_print_quote(node->sval, stdout);
        printf("\"");
        break;
      default:
        errorf("unknown literal type: %d", node->type->kind);
    }
    break;
  case NODE_KIND_VARIABLE:
    printf("%s", node->vname);
    break;
  case NODE_KIND_DECLARATION:
    printf("(decl %s %s", node->type->name, node->dec_var->vname);
    if (node->dec_init) {
      printf(" ");
      node_debug(node->dec_init);
    }
    printf(")");
    break;
  case NODE_KIND_BINARY_OP:
    printf("(%c ", node->op);
    node_debug(node->left);
    printf(" ");
    node_debug(node->right);
    printf(")");
    break;
  case NODE_KIND_UNARY_OP:
    printf("(%c ", node->op);
    node_debug(node->operand);
    printf(")");
    break;
  case NODE_KIND_CALL:
    printf("(%s", node->func->identifier);
    if (node->args->size > 0) {
      for (int i = 0; i < node->args->size; i++) {
        printf(" ");
        node_debug((node_t *)node->args->data[i]);
      }
    }
    printf(")");
    break;
  }
  if (node->next) {
    printf(", ");
    node_debug(node->next);
  }
}
