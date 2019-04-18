// Copyright 2019 @htz. Released under the MIT license.

#include <stdarg.h>
#include <stdio.h>
#include "hcc.h"

static const char *REGS[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};

static void emitf_noindent(char *fmt, ...);
static void emitf(char *fmt, ...);
static void emit_push(const char *reg);
static void emit_pop(const char *reg);
static void emit_int(node_t *node);
static void emit_string(node_t *node);
static void emit_string_data(string_t *str);
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

static void emit_string(node_t *node) {
  emitf("lea .STR_%d(%%rip), %%rax", node->sid);
}

static void emit_string_data(string_t *str) {
  fprintf(stdout, "\t.string \"");
  string_print_quote(str, stdout);
  fprintf(stdout, "\"\n");
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

static void emit_call(node_t *node) {
  int i;
  vector_t *iargs = vector_new();
  vector_t *rargs = vector_new();
  for (i = 0; i < node->args->size; i++) {
    node_t *n = (node_t *)node->args->data[i];
    vector_push(iargs->size < 6 ? iargs : rargs, n);
  }
  // store registers
  for (i = 0; i < iargs->size; i++) {
    emit_push(REGS[i]);
  }
  // build arguments
  for (i = rargs->size - 1; i >= 0; i--) {
    node_t *n = (node_t *)rargs->data[i];
    emit_expression(n);
    emit_push("rax");
  }
  for (i = 0; i < iargs->size; i++) {
    node_t *n = (node_t *)iargs->data[i];
    emit_expression(n);
    emitf("mov %%rax, %%%s", REGS[i]);
  }
  // call function
  emitf("mov $0, %%eax");
  emitf("call %s", node->func->identifier);
  // restore registers
  int rsize = rargs->size * 8;
  if (rsize > 0) {
    emitf("add $%d, %%rsp", rsize);
  }
  for (i = iargs->size - 1; i >= 0; i--) {
    emit_pop(REGS[i]);
  }

  vector_free(iargs);
  vector_free(rargs);
}

static void emit_expression(node_t *node) {
  for (; node; node = node->next) {
    switch (node->kind) {
    case NODE_KIND_NOP:
      break;
    case NODE_KIND_BINARY_OP:
      emit_binary_op_expression(node);
      break;
    case NODE_KIND_LITERAL_INT:
      emit_int(node);
      break;
    case NODE_KIND_LITERAL_STRING:
      emit_string(node);
      break;
    case NODE_KIND_CALL:
      emit_call(node);
      break;
    default:
      errorf("unknown expression node: %d", node->kind);
    }
  }
}

static void emit_data_section(parse_t *parse) {
  vector_t *data = parse->data;
  if (data->size == 0) {
    return;
  }
  emitf(".data");
  for (int i = 0; i < data->size; i++) {
    node_t *n = (node_t *)data->data[i];
    emitf_noindent(".STR_%d:", n->sid);
    emit_string_data(n->sval);
  }
}

void gen(parse_t *parse) {
  emit_data_section(parse);
  emitf(".text");
  emitf_noindent(".global mymain");
  emitf_noindent("mymain:");
  emit_push("rbp");
  emitf("mov %%rsp, %%rbp");
  for (int i = 0; i < parse->statements->size; i++) {
    emit_expression((node_t *)parse->statements->data[i]);
  }
  emitf("leave");
  emitf("ret");
}
