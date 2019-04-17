// Copyright 2019 @htz. Released under the MIT license.

#include <stdarg.h>
#include <stdio.h>
#include "hcc.h"

static void emitf_noindent(char *fmt, ...);
static void emitf(char *fmt, ...);
static void emit_push(const char *reg);
static void emit_pop(const char *reg);
static void emit_int(node_t *node);
static void emit_binary_op_expression(node_t *node);
static void emit_expression(node_t *node);

static void emitf_noindent(char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  vfprintf(stdout, fmt, args);
  va_end(args);
  fprintf(stdout, "\n");
}

static void emitf(char *fmt, ...) {
  va_list args;
  fprintf(stdout, "\t");
  va_start(args, fmt);
  vfprintf(stdout, fmt, args);
  va_end(args);
  fprintf(stdout, "\n");
}

static void emit_push(const char *reg) {
  emitf("push %%%s", reg);
}

static void emit_pop(const char *reg) {
  emitf("pop %%%s", reg);
}

static void emit_int(node_t *node) {
  emitf("mov $%d, %%rax", node->ival);
}

static void emit_binary_op_expression(node_t *node) {
  emit_expression(node->left);
  emit_push("rcx");
  emit_push("rax");
  emit_expression(node->right);
  emitf("mov %%rax, %%rcx");
  emit_pop("rax");
  if (node->op == '/' || node->op == '%') {
    emitf("cqto");
    emitf("idiv %%rcx");
    if (node->op == '%') {
      emitf("mov %%edx, %%eax");
    }
  } else {
    char *op;
    switch (node->op) {
    case '+': op = "add"; break;
    case '-': op = "sub"; break;
    case '*': op = "imul"; break;
    default:
      errorf("unknown op: %c\n", node->op);
    }
    emitf("%s %%rcx, %%rax", op);
  }
  emit_pop("rcx");
}

static void emit_expression(node_t *node) {
  if (node->kind == NODE_KIND_BINARY_OP) {
    emit_binary_op_expression(node);
  } else if (node->kind == NODE_KIND_LITERAL_INT) {
    emit_int(node);
  }
}

void gen(node_t *node) {
  emitf(".text");
  emitf_noindent(".global mymain");
  emitf_noindent("mymain:");
  emit_push("rbp");
  emitf("mov %%rsp, %%rbp");
  emit_expression(node);
  emitf("leave");
  emitf("ret");
}
