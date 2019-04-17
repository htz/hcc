// Copyright 2019 @htz. Released under the MIT license.

#include <stdio.h>
#include <stdlib.h>
#include "hcc.h"

static node_t *node_new(int kind) {
  node_t *node = (node_t *)malloc(sizeof (node_t));
  node->kind = kind;
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

void node_free(node_t *node) {
  switch (node->kind) {
  case NODE_KIND_BINARY_OP:
    node_free(node->left);
    node_free(node->right);
    break;
  }
  free(node);
}

void node_debug(node_t *node) {
  switch (node->kind) {
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
  }
}
