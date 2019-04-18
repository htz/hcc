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
  node->identifier = strdup(identifier);
  return node;
}

node_t *node_new_int(parse_t *parse, int ival) {
  node_t *node = node_new(parse, NODE_KIND_LITERAL_INT);
  node->ival = ival;
  return node;
}

node_t *node_new_string(parse_t *parse, string_t *sval, int sid) {
  node_t *node = node_new(parse, NODE_KIND_LITERAL_STRING);
  node->sval = string_dup(sval);
  node->sid = sid;
  return node;
}

node_t *node_new_binary_op(parse_t *parse, int op, node_t *left, node_t *right) {
  node_t *node = node_new(parse, NODE_KIND_BINARY_OP);
  node->op = op;
  node->left = left;
  node->right = right;
  return node;
}

node_t *node_new_call(parse_t *parse, node_t *func, vector_t *args) {
  node_t *node = node_new(parse, NODE_KIND_CALL);
  node->func = func;
  node->args = args;
  return node;
}

void node_free(node_t *node) {
  switch (node->kind) {
  case NODE_KIND_IDENTIFIER:
    free(node->identifier);
    break;
  case NODE_KIND_LITERAL_STRING:
    string_free(node->sval);
    break;
  case NODE_KIND_BINARY_OP:
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
    printf("%s", node->identifier);
    break;
  case NODE_KIND_LITERAL_INT:
    printf("%d", node->ival);
    break;
  case NODE_KIND_LITERAL_STRING:
    printf("\"");
    string_print_quote(node->sval, stdout);
    printf("\"");
    break;
  case NODE_KIND_BINARY_OP:
    printf("(%c ", node->op);
    node_debug(node->left);
    printf(" ");
    node_debug(node->right);
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
