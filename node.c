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
  node_t *node = node_new(parse, NODE_KIND_STRING_LITERAL);
  int size = sval->size + 1;
  string_t *name = string_new();
  string_appendf(name, "char[%d]", size);
  map_entry_t *e = map_find(parse->types, name->buf);
  type_t *type;
  if (e == NULL) {
    type = type_new_with_size(name->buf, TYPE_KIND_ARRAY, parse->type_char, size);
    map_add(parse->types, name->buf, type);
  } else {
    type = (type_t *)e->val;
  }
  node->type = type;
  node->sval = string_dup(sval);
  node->sid = sid;
  string_free(name);
  return node;
}

node_t *node_new_init_list(parse_t *parse, type_t *type, vector_t *init) {
  node_t *node = node_new(parse, NODE_KIND_INIT_LIST);
  node->type = type;
  node->init_list = init;
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

node_t *node_new_block(parse_t *parse, vector_t *statements, node_t *block) {
  node_t *node = node_new(parse, NODE_KIND_BLOCK);
  node->statements = statements;
  node->vars = map_new();
  node->parent_block = block;
  if (block != NULL) {
    vector_push(block->child_blocks, node);
  }
  node->child_blocks = vector_new();
  return node;
}

node_t *node_new_if(parse_t *parse, node_t *cond, node_t *then_body, node_t *else_body) {
  node_t *node = node_new(parse, NODE_KIND_IF);
  node->cond = cond;
  node->then_body = then_body;
  node->else_body = else_body;
  return node;
}

void node_free(node_t *node) {
  switch (node->kind) {
  case NODE_KIND_IDENTIFIER:
    free(node->identifier);
    break;
  case NODE_KIND_LITERAL:
    switch (node->type->kind) {
      case TYPE_KIND_ARRAY:
        assert(node->type->size > 0 && node->type->parent && node->type->parent->kind == TYPE_KIND_CHAR);
        string_free(node->sval);
        break;
    }
    break;
  case NODE_KIND_STRING_LITERAL:
    string_free(node->sval);
    break;
  case NODE_KIND_INIT_LIST:
    vector_free(node->init_list);
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
  case NODE_KIND_BLOCK:
    vector_free(node->statements);
    map_free(node->vars);
    vector_free(node->child_blocks);
    break;
  case NODE_KIND_IF:
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
      default:
        errorf("unknown literal type: %d", node->type->kind);
    }
    break;
  case NODE_KIND_STRING_LITERAL:
    assert(node->type->kind == TYPE_KIND_ARRAY && node->type->size > 0);
    assert(node->type->parent && node->type->parent->kind == TYPE_KIND_CHAR);
    printf("\"");
    string_print_quote(node->sval, stdout);
    printf("\"");
    break;
  case NODE_KIND_INIT_LIST:
    printf("[");
    for (int i = 0; i < node->init_list->size; i++) {
      node_debug((node_t *)node->init_list->data[i]);
      if (i < node->init_list->size - 1) {
        printf(" ");
      }
    }
    printf("]");
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
  case NODE_KIND_BLOCK:
    printf("{");
    for (int i = 0; i < node->statements->size; i++) {
      node_debug((node_t *)node->statements->data[i]);
      printf(";");
    }
    printf("}");
    break;
  case NODE_KIND_IF:
    printf("(if ");
    node_debug(node->cond);
    printf(" ");
    node_debug(node->then_body);
    if (node->else_body) {
      printf(" ");
      node_debug(node->else_body);
    }
    printf(")");
    break;
  }
  if (node->next) {
    printf(", ");
    node_debug(node->next);
  }
}
