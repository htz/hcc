// Copyright 2019 @htz. Released under the MIT license.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hcc.h"

static node_t *node_new(int kind) {
  node_t *node = (node_t *)malloc(sizeof (node_t));
  node->kind = kind;
  node->next = NULL;
  return node;
}

node_t *node_new_identifier(char *identifier) {
  node_t *node = node_new(NODE_KIND_IDENTIFIER);
  node->identifier = strdup(identifier);
  return node;
}

node_t *node_new_int(int ival) {
  node_t *node = node_new(NODE_KIND_LITERAL_INT);
  node->ival = ival;
  return node;
}

node_t *node_new_binary_op(int op, node_t *left, node_t *right) {
  node_t *node = node_new(NODE_KIND_BINARY_OP);
  node->op = op;
  node->left = left;
  node->right = right;
  return node;
}

node_t *node_new_call(node_t *func, vector_t *args) {
  node_t *node = node_new(NODE_KIND_CALL);
  node->func = func;
  node->args = args;
  return node;
}

void node_free(node_t *node) {
  int i;
  switch (node->kind) {
  case NODE_KIND_IDENTIFIER:
    free(node->identifier);
    break;
  case NODE_KIND_BINARY_OP:
    node_free(node->left);
    node_free(node->right);
    break;
  case NODE_KIND_CALL:
    node_free(node->func);
    for (i = 0; i < node->args->size; i++) {
      node_free((node_t *)node->args->data[i]);
    }
    vector_free(node->args);
    break;
  }
  if (node->next) {
    node_free(node->next);
  }
  free(node);
}

void node_debug(node_t *node) {
  switch (node->kind) {
  case NODE_KIND_IDENTIFIER:
    printf("%s", node->identifier);
    break;
  case NODE_KIND_LITERAL_INT:
    printf("%d", node->ival);
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
