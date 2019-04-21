// Copyright 2019 @htz. Released under the MIT license.

#include <stdarg.h>
#include <stdio.h>
#include "hcc.h"

static const char *REGS[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};

static void emitf_noindent(char *fmt, ...);
static void emitf(char *fmt, ...);
static void emit_push(parse_t *parse, const char *reg);
static void emit_pop(parse_t *parse, const char *reg);
static void emit_int(parse_t *parse, node_t *node);
static void emit_string(parse_t *parse, node_t *node);
static void emit_string_data(string_t *str);
static void emit_variable(parse_t *parse, node_t *node);
static void emit_assign(parse_t *parse, node_t *var, node_t *val);
static void emit_binary_op_expression(parse_t *parse, node_t *node);
static void emit_unary_op_expression(parse_t *parse, node_t *node);
static void emit_call(parse_t *parse, node_t *node);
static void emit_expression(parse_t *parse, node_t *node);
static void emit_data_section(parse_t *parse);

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

static void emit_push(parse_t *parse, const char *reg) {
  emitf("push %%%s", reg);
}

static void emit_pop(parse_t *parse, const char *reg) {
  emitf("pop %%%s", reg);
}

static void emit_int(parse_t *parse, node_t *node) {
  emitf("mov $%d, %%rax", node->ival);
}

static void emit_string(parse_t *parse, node_t *node) {
  emitf("lea .STR_%d(%%rip), %%rax", node->sid);
}

static void emit_string_data(string_t *str) {
  fprintf(stdout, "\t.string \"");
  string_print_quote(str, stdout);
  fprintf(stdout, "\"\n");
}

static void emit_variable(parse_t *parse, node_t *node) {
  switch (node->type->bytes) {
  case 1:
    emitf("movsbq %d(%%rbp), %%rax", -node->voffset);
    break;
  case 4:
    emitf("movslq %d(%%rbp), %%rax", -node->voffset);
    break;
  case 8:
    emitf("mov %d(%%rbp), %%rax", -node->voffset);
    break;
  default:
    errorf("invalid variable type");
  }
}

static void emit_assign(parse_t *parse, node_t *var, node_t *val) {
  emit_expression(parse, val);
  emitf("mov %%rax, %d(%%rbp)", -var->voffset);
}

static void emit_binary_op_expression(parse_t *parse, node_t *node) {
  if (node->op == '=') {
    assert(node->left->kind == NODE_KIND_VARIABLE);
    emit_assign(parse, node->left, node->right);
    return;
  }

  emit_expression(parse, node->left);
  emit_push(parse, "rcx");
  emit_push(parse, "rax");
  emit_expression(parse, node->right);
  emitf("mov %%rax, %%rcx");
  emit_pop(parse, "rax");
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
  emit_pop(parse, "rcx");
}

static void emit_unary_op_expression(parse_t *parse, node_t *node) {
  switch (node->op) {
  case '&':
    assert(node->operand->kind == NODE_KIND_VARIABLE);
    emitf("lea %d(%%rbp), %%rax", -node->operand->voffset);
    break;
  case '*':
    emit_expression(parse, node->operand);
    switch (node->type->bytes) {
    case 1:
      emitf("movzbq (%%rax), %%rax");
      break;
    case 4:
      emitf("mov (%%rax), %%eax");
      break;
    case 8:
      emitf("mov (%%rax), %%rax");
      break;
    default:
      errorf("invalid variable type");
    }
    break;
  }
}

static void emit_call(parse_t *parse, node_t *node) {
  int i;
  vector_t *iargs = vector_new();
  vector_t *rargs = vector_new();
  for (i = 0; i < node->args->size; i++) {
    node_t *n = (node_t *)node->args->data[i];
    vector_push(iargs->size < 6 ? iargs : rargs, n);
  }
  // store registers
  for (i = 0; i < iargs->size; i++) {
    emit_push(parse, REGS[i]);
  }
  // build arguments
  for (i = rargs->size - 1; i >= 0; i--) {
    node_t *n = (node_t *)rargs->data[i];
    emit_expression(parse, n);
    emit_push(parse, "rax");
  }
  for (i = 0; i < iargs->size; i++) {
    node_t *n = (node_t *)iargs->data[i];
    emit_expression(parse, n);
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
    emit_pop(parse, REGS[i]);
  }

  vector_free(iargs);
  vector_free(rargs);
}

static void emit_expression(parse_t *parse, node_t *node) {
  for (; node; node = node->next) {
    switch (node->kind) {
    case NODE_KIND_NOP:
      break;
    case NODE_KIND_BINARY_OP:
      emit_binary_op_expression(parse, node);
      break;
    case NODE_KIND_UNARY_OP:
      emit_unary_op_expression(parse, node);
      break;
    case NODE_KIND_LITERAL:
      switch (node->type->kind) {
      case TYPE_KIND_CHAR:
      case TYPE_KIND_INT:
        emit_int(parse, node);
        break;
      case TYPE_KIND_STRING:
        emit_string(parse, node);
        break;
      default:
        errorf("unknown literal type: %d", node->type);
      }
      break;
    case NODE_KIND_VARIABLE:
      emit_variable(parse, node);
      break;
    case NODE_KIND_DECLARATION:
      if (node->dec_init) {
        emit_assign(parse, node->dec_var, node->dec_init);
      }
      break;
    case NODE_KIND_CALL:
      emit_call(parse, node);
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
  int offset = 0;
  for (map_entry_t *e = parse->vars->top; e != NULL; e = e->next) {
    node_t *n = (node_t *)e->val;
    offset += 8;
    n->voffset = offset;
  }
  emit_data_section(parse);
  emitf(".text");
  emitf_noindent(".global mymain");
  emitf_noindent("mymain:");
  emit_push(parse, "rbp");
  emitf("mov %%rsp, %%rbp");
  emitf("sub $%d, %%rsp", offset);
  for (int i = 0; i < parse->statements->size; i++) {
    emit_expression(parse, (node_t *)parse->statements->data[i]);
  }
  emitf("leave");
  emitf("ret");
}
