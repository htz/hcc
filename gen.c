// Copyright 2019 @htz. Released under the MIT license.

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hcc.h"

static const char *REGS[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};
static const char *MREGS[] = {"edi", "esi", "edx", "ecx", "r8d", "r9d"};

static void emitf_noindent(char *fmt, ...);
static void emitf(char *fmt, ...);
static void emit_push(parse_t *parse, const char *reg);
static void emit_pop(parse_t *parse, const char *reg);
static void emit_push_xmm(parse_t *parse, int n);
static void emit_pop_xmm(parse_t *parse, int n);
static void emit_add_rsp(parse_t *parse, int n);
static void emit_int(parse_t *parse, node_t *node);
static void emit_float(parse_t *parse, node_t *node);
static void emit_string(parse_t *parse, node_t *node);
static void emit_string_data(string_t *str);
static void emit_global_variable(parse_t *parse, node_t *node);
static void emit_variable(parse_t *parse, node_t *node);
static void emit_store(parse_t *parse, node_t *var, type_t *type);
static void emit_cast(parse_t *parse, type_t *to, type_t *from);
static void emit_binary_op_expression(parse_t *parse, node_t *node);
static void emit_unary_op_expression(parse_t *parse, node_t *node);
static void emit_declaration_init_array(parse_t *parse, node_t *var, type_t *type, vector_t *vals, int offset);
static void emit_declaration_init_struct(parse_t *parse, node_t *var, type_t *type, vector_t *vals, int offset);
static void emit_declaration_init(parse_t *parse, node_t *var, node_t *init);
static void emit_call(parse_t *parse, node_t *node);
static void emit_if(parse_t *parse, node_t *node);
static void emit_while(parse_t *parse, node_t *node);
static void emit_do(parse_t *parse, node_t *node);
static void emit_for(parse_t *parse, node_t *node);
static void emit_switch(parse_t *parse, node_t *node);
static void emit_case(parse_t *parse, node_t *node);
static void emit_continue(parse_t *parse, node_t *node);
static void emit_break(parse_t *parse, node_t *node);
static void emit_return(parse_t *parse, node_t *node);
static void emit_expression(parse_t *parse, node_t *node);
static void emit_function(parse_t *parse, node_t *node);
static void emit_global(parse_t *parse, node_t *node);
static void emit_data_section(parse_t *parse);
static void emit_builtin_va_start(parse_t *parse, node_t *func);

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
  parse->stackpos += 8;
}

static void emit_pop(parse_t *parse, const char *reg) {
  emitf("pop %%%s", reg);
  parse->stackpos -= 8;
  assert(parse->stackpos >= 0);
}

static void emit_push_xmm(parse_t *parse, int n) {
  emit_add_rsp(parse, -8);
  emitf("movsd %%xmm%d, (%%rsp)", n);
}

static void emit_pop_xmm(parse_t *parse, int n) {
  emitf("movsd (%%rsp), %%xmm%d", n);
  emit_add_rsp(parse, 8);
}

static void emit_add_rsp(parse_t *parse, int n) {
  if (n > 0) {
    emitf("add $%d, %%rsp", n);
  } else if (n < 0) {
    emitf("sub $%d, %%rsp", -n);
  }
  parse->stackpos -= n;
  assert(parse->stackpos >= 0);
}

static void emit_int(parse_t *parse, node_t *node) {
  emitf("mov $%ld, %%rax", node->ival);
}

static void emit_float(parse_t *parse, node_t *node) {
  if (node->type->kind == TYPE_KIND_FLOAT) {
    emitf("movss .FLT_%d(%%rip), %%xmm0", node->fid);
  } else {
    emitf("movsd .DBL_%d(%%rip), %%xmm0", node->fid);
  }
}

static void emit_string(parse_t *parse, node_t *node) {
  assert(node->type->kind == TYPE_KIND_ARRAY && node->type->size > 0);
  assert(node->type->parent && node->type->parent->kind == TYPE_KIND_CHAR);
  emitf("lea .STR_%d(%%rip), %%rax", node->sid);
}

static void emit_string_data(string_t *str) {
  fprintf(stdout, "\t.string \"");
  string_print_quote(str, stdout);
  fprintf(stdout, "\"\n");
}

static void emit_global_variable(parse_t *parse, node_t *node) {
  switch (node->type->kind) {
  case TYPE_KIND_ARRAY:
  case TYPE_KIND_FUNCTION:
    assert(node->type->parent != NULL);
    emitf("lea %s(%%rip), %%rax", node->vname);
    break;
  case TYPE_KIND_STRUCT:
    emitf("lea %s(%%rip), %%rax", node->vname);
    break;
  case TYPE_KIND_FLOAT:
    emitf("movss %s(%%rip), %%xmm0", node->vname);
    break;
  case TYPE_KIND_DOUBLE:
  case TYPE_KIND_LDOUBLE:
    emitf("movsd %s(%%rip), %%xmm0", node->vname);
    break;
  default:
    switch (node->type->bytes) {
    case 1:
      emitf("movsbq %s(%%rip), %%rax", node->vname);
      break;
    case 2:
      emitf("movswq %s(%%rip), %%rax", node->vname);
      break;
    case 4:
      emitf("movslq %s(%%rip), %%rax", node->vname);
      break;
    case 8:
      emitf("mov %s(%%rip), %%rax", node->vname);
      break;
    default:
      errorf("invalid variable type");
    }
  }
}

static void emit_variable(parse_t *parse, node_t *node) {
  if (node->global) {
    emit_global_variable(parse, node);
    return;
  }
  switch (node->type->kind) {
  case TYPE_KIND_ARRAY:
    assert(node->type->parent != NULL);
    emitf("lea %d(%%rbp), %%rax", -node->voffset);
    break;
  case TYPE_KIND_STRUCT:
    emitf("lea %d(%%rbp), %%rax", -node->voffset);
    break;
  case TYPE_KIND_FLOAT:
    emitf("movss %d(%%rbp), %%xmm0", -node->voffset);
    break;
  case TYPE_KIND_DOUBLE:
  case TYPE_KIND_LDOUBLE:
    emitf("movsd %d(%%rbp), %%xmm0", -node->voffset);
    break;
  default:
    switch (node->type->bytes) {
    case 1:
      emitf("movsbq %d(%%rbp), %%rax", -node->voffset);
      break;
    case 2:
      emitf("movswq %d(%%rbp), %%rax", -node->voffset);
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
}

static void emit_save_global_to(parse_t *parse, type_t *type, const char *name, int offset) {
  switch (type->kind) {
  case TYPE_KIND_FLOAT:
    emitf("movss %%xmm0, %s%+d(%%rip)", name, offset);
    break;
  case TYPE_KIND_DOUBLE:
  case TYPE_KIND_LDOUBLE:
    emitf("movsd %%xmm0, %s%+d(%%rip)", name, offset);
    break;
  case TYPE_KIND_STRUCT:
    emit_push(parse, "rsi");
    emit_push(parse, "rdi");
    emit_push(parse, "rcx");
    emitf("mov %%rax, %%rsi");
    emitf("lea %s%+d(%%rip), %%rdi", name, offset);
    emitf("mov $%d, %%rcx", type->total_size);
    emitf("rep movsb");
    emit_pop(parse, "rcx");
    emit_pop(parse, "rdi");
    emit_pop(parse, "rsi");
    break;
  default:
    switch (type->bytes) {
    case 1:
      emitf("mov %%al, %s%+d(%%rip)", name, offset);
      break;
    case 2:
      emitf("mov %%ax, %s%+d(%%rip)", name, offset);
      break;
    case 4:
      emitf("mov %%eax, %s%+d(%%rip)", name, offset);
      break;
    case 8:
      emitf("mov %%rax, %s%+d(%%rip)", name, offset);
      break;
    default:
      errorf("invalid variable type");
    }
  }
}

static void emit_save_to(parse_t *parse, node_t *var, type_t *type, int offset) {
  if (var->global) {
    emit_save_global_to(parse, type, var->vname, offset);
    return;
  }
  switch (type->kind) {
  case TYPE_KIND_FLOAT:
    emitf("movss %%xmm0, %d(%%rbp)", -var->voffset + offset);
    break;
  case TYPE_KIND_DOUBLE:
  case TYPE_KIND_LDOUBLE:
    emitf("movsd %%xmm0, %d(%%rbp)", -var->voffset + offset);
    break;
  case TYPE_KIND_STRUCT:
    emit_push(parse, "rsi");
    emit_push(parse, "rdi");
    emit_push(parse, "rcx");
    emitf("mov %%rax, %%rsi");
    emitf("lea %d(%%rbp), %%rdi", -var->voffset + offset);
    emitf("mov $%d, %%rcx", type->total_size);
    emitf("rep movsb");
    emit_pop(parse, "rcx");
    emit_pop(parse, "rdi");
    emit_pop(parse, "rsi");
    break;
  default:
    switch (type->bytes) {
    case 1:
      emitf("mov %%al, %d(%%rbp)", -var->voffset + offset);
      break;
    case 2:
      emitf("mov %%ax, %d(%%rbp)", -var->voffset + offset);
      break;
    case 4:
      emitf("mov %%eax, %d(%%rbp)", -var->voffset + offset);
      break;
    case 8:
      emitf("mov %%rax, %d(%%rbp)", -var->voffset + offset);
      break;
    default:
      errorf("invalid variable type");
    }
  }
}

static void emit_save(parse_t *parse, node_t *var, type_t *type, int base, int at) {
  emit_save_to(parse, var, type, base + type->bytes * at);
}

static void emit_store_struct(parse_t *parse, node_t *var, type_t *type, int offset) {
  if (var->kind == NODE_KIND_UNARY_OP && var->op == '*') {
    if (type_is_float(type)) {
      emit_push_xmm(parse, 0);
    } else {
      emit_push(parse, "rax");
    }
    emit_expression(parse, var->operand);
    emitf("mov %%rax, %%rcx");
    if (type_is_float(type)) {
      emit_pop_xmm(parse, 0);
    } else {
      emit_pop(parse, "rax");
    }
    switch (type->kind) {
    case TYPE_KIND_FLOAT:
      emitf("movss %%xmm0, %d(%%rcx)", offset);
      break;
    case TYPE_KIND_DOUBLE:
      emitf("movsd %%xmm0, %d(%%rcx)", offset);
      break;
    default:
      switch (type->bytes) {
      case 1:
        emitf("mov %%al, %d(%%rcx)", offset);
        break;
      case 2:
        emitf("mov %%ax, %d(%%rcx)", offset);
        break;
      case 4:
        emitf("mov %%eax, %d(%%rcx)", offset);
        break;
      case 8:
        emitf("mov %%rax, %d(%%rcx)", offset);
        break;
      default:
        errorf("invalid variable type");
      }
    }
    return;
  } else if (var->kind == NODE_KIND_BINARY_OP && var->op == '.') {
    emit_store_struct(parse, var->left, type, offset + var->right->voffset);
    return;
  }
  assert(var->kind == NODE_KIND_VARIABLE);
  emit_save_to(parse, var, type, offset);
}

static void emit_store(parse_t *parse, node_t *var, type_t *type) {
  emit_cast(parse, var->type, type);
  if (var->kind == NODE_KIND_UNARY_OP && var->op == '*') {
    if (type_is_float(var->type)) {
      emit_push_xmm(parse, 0);
    } else {
      emit_push(parse, "rax");
    }
    emit_expression(parse, var->operand);
    emitf("mov %%rax, %%rcx");
    if (type_is_float(var->type)) {
      emit_pop_xmm(parse, 0);
    } else {
      emit_pop(parse, "rax");
    }
    switch (var->type->kind) {
    case TYPE_KIND_FLOAT:
      emitf("movss %%xmm0, (%%rcx)");
      break;
    case TYPE_KIND_DOUBLE:
    case TYPE_KIND_LDOUBLE:
      emitf("movsd %%xmm0, (%%rcx)");
      break;
    default:
      switch (var->type->bytes) {
      case 1:
        emitf("mov %%al, (%%rcx)");
        break;
      case 2:
        emitf("mov %%ax, (%%rcx)");
        break;
      case 4:
        emitf("mov %%eax, (%%rcx)");
        break;
      case 8:
        emitf("mov %%rax, (%%rcx)");
        break;
      default:
        errorf("invalid variable type");
      }
    }
    return;
  } else if (var->kind == NODE_KIND_BINARY_OP && var->op == '.') {
    emit_store_struct(parse, var->left, var->type, var->right->voffset);
    return;
  }
  assert(var->kind == NODE_KIND_VARIABLE);
  emit_save(parse, var, var->type, 0, 0);
}

static void emit_cast_to_bool(parse_t *parse, type_t *type) {
  if (type_is_float(type)) {
    emit_push_xmm(parse, 1);
    emitf("xorpd %%xmm1, %%xmm1");
    emitf("%s %%xmm1, %%xmm0", type->kind == TYPE_KIND_FLOAT ? "ucomiss" : "ucomisd");
    emitf("setne %%al");
    emit_pop_xmm(parse, 1);
  } else {
    emitf("cmp $0, %%rax");
    emitf("setne %%al");
  }
  emitf("movzb %%al, %%eax");
}

static void emit_cast_to_int(parse_t *parse, type_t *type) {
  switch (type->kind) {
  case TYPE_KIND_CHAR:
    emitf("%s %%al, %%rax", type->sign ? "movsbq" : "movzbq");
    break;
  case TYPE_KIND_SHORT:
    emitf("%s %%ax, %%rax", type->sign ? "movswq" : "movzwq");
    break;
  case TYPE_KIND_INT:
    if (type->sign) {
      emitf("cltq");
    } else {
      emitf("mov %%eax, %%eax");
    }
    break;
  case TYPE_KIND_LONG:
  case TYPE_KIND_LLONG:
    break;
  case TYPE_KIND_FLOAT:
    emitf("cvttss2si %%xmm0, %%rax");
    break;
  case TYPE_KIND_DOUBLE:
  case TYPE_KIND_LDOUBLE:
    emitf("cvttsd2si %%xmm0, %%rax");
    break;
  }
}

static void emit_cast(parse_t *parse, type_t *to, type_t *from) {
  if (type_is_bool(to)) {
    emit_cast_to_bool(parse, from);
  } else if (type_is_int(to)) {
    emit_cast_to_int(parse, from);
  } else if (to->kind == TYPE_KIND_FLOAT) {
    if (type_is_int(from)) {
      emitf("cvtsi2ss %%rax, %%xmm0");
    } else if (from->kind == TYPE_KIND_DOUBLE || from->kind == TYPE_KIND_LDOUBLE) {
      emitf("cvtsd2ss %%xmm0, %%xmm0");
    }
  } else if (to->kind == TYPE_KIND_DOUBLE || to->kind == TYPE_KIND_LDOUBLE) {
    if (type_is_int(from)) {
      emitf("cvtsi2sd %%rax, %%xmm0");
    } else if (from->kind == TYPE_KIND_FLOAT) {
      emitf("cvtss2sd %%xmm0, %%xmm0");
    }
  }
}

static void emit_arithmetic_int(parse_t *parse, int op, node_t *node) {
  if (node->type->kind == TYPE_KIND_PTR || node->type->kind == TYPE_KIND_ARRAY) {
    assert(node->type->parent != NULL && (op == '+' || op == '-'));
    if (node->type->parent->total_size > 1) {
      emitf("imul $%d, %%rax", node->type->parent->total_size);
    }
  }
  emitf("mov %%rax, %%rcx");
  emit_pop(parse, "rax");
  if (op == '/' || op == '%') {
    if (node->type->sign) {
      emitf("cqto");
      emitf("idiv %%rcx");
    } else {
      emitf("xor %%edx, %%edx");
      emitf("div %%rcx");
    }
    if (op == '%') {
      emitf("mov %%edx, %%eax");
    }
  } else {
    switch (op) {
    case '+':
      emitf("add %%rcx, %%rax");
        break;
    case '-':
      emitf("sub %%rcx, %%rax");
      break;
    case '*':
      emitf("imul %%rcx, %%rax");
      break;
    case '^':
      emitf("xor %%rcx, %%rax");
      break;
    default:
      errorf("unknown operator");
    }
  }
}

static void emit_arithmetic_float(parse_t *parse, int op, bool isdouble) {
  emitf("%s %%xmm0, %%xmm1", isdouble ? "movsd" : "movss");
  emit_pop_xmm(parse, 0);
  switch (op) {
  case '+':
    emitf("%s %%xmm1, %%xmm0", isdouble ? "addsd" : "addss");
      break;
  case '-':
    emitf("%s %%xmm1, %%xmm0", isdouble ? "subsd" : "subss");
    break;
  case '*':
    emitf("%s %%xmm1, %%xmm0", isdouble ? "mulsd" : "mulss");
    break;
  case '/':
    emitf("%s %%xmm1, %%xmm0", isdouble ? "divsd" : "divss");
    break;
  default:
    errorf("unknown operator");
  }
}

static void emit_bitshift(parse_t *parse, int op, node_t *node) {
  emitf("mov %%rax, %%rcx");
  emit_pop(parse, "rax");
  const char *opstr;
  if (op == OP_SAL) {
    opstr = "sal";
  } else if (node->type->sign) {
    opstr = "sar";
  } else {
    opstr = "shr";
  }
  switch (node->type->bytes) {
  case 1:
    emitf("%s %%cl, %%al", opstr);
    break;
  case 2:
    emitf("%s %%cl, %%ax", opstr);
    break;
  case 4:
    emitf("%s %%cl, %%eax", opstr);
    break;
  case 8:
    emitf("%s %%cl, %%rax", opstr);
    break;
  default:
    errorf("invalid variable type");
  }
}

static void emit_arithmetic_bit(parse_t *parse, int op) {
  switch (op) {
  case '&':
    emit_pop(parse, "rcx");
    emitf("and %%rcx, %%rax");
    break;
  case '|':
    emit_pop(parse, "rcx");
    emitf("or %%rcx, %%rax");
    break;
  default:
    errorf("unknown operator");
  }
}

static void emit_comp_int(parse_t *parse, int op, bool sign) {
  emit_pop(parse, "rcx");
  emitf("cmp %%rax, %%rcx");
  switch (op) {
  case OP_EQ:
    emitf("sete %%al");
    break;
  case OP_NE:
    emitf("setne %%al");
    break;
  case '<':
    emitf("%s %%al", sign ? "setl" : "setb");
    break;
  case OP_LE:
    emitf("%s %%al", sign ? "setle" : "setbe");
    break;
  case '>':
    emitf("%s %%al", sign ? "setg" : "seta");
    break;
  case OP_GE:
    emitf("%s %%al", sign ? "setge" : "setae");
    break;
  }
  emitf("movzb %%al, %%eax");
}

static void emit_comp_float(parse_t *parse, int op, bool isdouble) {
  emitf("%s %%xmm0, %%xmm1", isdouble ? "movsd" : "movss");
  emit_pop_xmm(parse, 0);
  emitf("%s %%xmm1, %%xmm0", isdouble ? "ucomisd" : "ucomiss");
  switch (op) {
  case OP_EQ:
    emitf("sete %%al");
    break;
  case OP_NE:
    emitf("setne %%al");
    break;
  case '<':
    emitf("setb %%al");
    break;
  case OP_LE:
    emitf("setbe %%al");
    break;
  case '>':
    emitf("seta %%al");
    break;
  case OP_GE:
    emitf("setae %%al");
    break;
  }
  emitf("movzb %%al, %%eax");
}

static void emit_logical(parse_t *parse, node_t *node) {
  emit_expression(parse, node->left);
  emitf("test %%rax, %%rax");
  if (node->op == OP_ANDAND) {
    emitf("mov $0, %%rax");
    emitf("je .L%p", node);
  } else {
    emitf("mov $1, %%rax");
    emitf("jne .L%p", node);
  }
  emit_expression(parse, node->right);
  emitf("test %%rax, %%rax");
  if (node->op == OP_ANDAND) {
    emitf("mov $0, %%rax");
    emitf("je .L%p", node);
    emitf("mov $1, %%rax");
  } else {
    emitf("mov $1, %%rax");
    emitf("jne .L%p", node);
    emitf("mov $0, %%rax");
  }
  emitf(".L%p:", node);
}

static void emit_binary_op_expression(parse_t *parse, node_t *node) {
  int op = node->op;
  if (op == '=') {
    emit_expression(parse, node->right);
    emit_store(parse, node->left, node->right->type);
    return;
  } else if (op == '.') {
    emit_expression(parse, node->left);
    if (node->type->kind == TYPE_KIND_ARRAY || node->type->kind == TYPE_KIND_STRUCT) {
      emitf("lea %d(%%rax), %%rax", node->right->voffset);
    } else if (node->type->kind == TYPE_KIND_FLOAT) {
      emitf("movss %d(%%rax), %%xmm0", node->right->voffset);
    } else if (node->type->kind == TYPE_KIND_DOUBLE) {
      emitf("movsd %d(%%rax), %%xmm0", node->right->voffset);
    } else {
      switch (node->type->bytes) {
      case 1:
        emitf("movzbq %d(%%rax), %%rax", node->right->voffset);
        break;
      case 2:
        emitf("movzwq %d(%%rax), %%rax", node->right->voffset);
        break;
      case 4:
        emitf("mov %d(%%rax), %%eax", node->right->voffset);
        break;
      case 8:
        emitf("mov %d(%%rax), %%rax", node->right->voffset);
        break;
      default:
        errorf("invalid variable type");
      }
    }
    return;
  }
  if (op & OP_ASSIGN_MASK) {
    node_t *tmp = node_new_binary_op(parse, node->type, op & ~OP_ASSIGN_MASK, node->left, node->right);
    emit_expression(parse, tmp);
    emit_store(parse, node->left, node->type);
    return;
  }
  if (op == OP_ANDAND || op == OP_OROR) {
    emit_logical(parse, node);
    return;
  }

  emit_expression(parse, node->left);
  if (type_is_float(node->type)) {
    emit_cast(parse, node->type, node->left->type);
    emit_push_xmm(parse, 0);
    emit_expression(parse, node->right);
    emit_cast(parse, node->type, node->right->type);
    emit_arithmetic_float(parse, op, node->type->kind == TYPE_KIND_DOUBLE || node->type->kind == TYPE_KIND_LDOUBLE);
  } else if (op == OP_SAL || op == OP_SAR) {
    emit_push(parse, "rcx");
    emit_push(parse, "rax");
    emit_expression(parse, node->right);
    emit_bitshift(parse, op, node->left);
    emit_pop(parse, "rcx");
  } else if (op == '&' || op == '|') {
    emit_push(parse, "rcx");
    emit_push(parse, "rax");
    emit_expression(parse, node->right);
    emit_arithmetic_bit(parse, op);
    emit_pop(parse, "rcx");
  } else if (op == OP_EQ || op == OP_NE || op == '<' || op == OP_LE || op == '>' || op == OP_GE) {
    if (type_is_float(node->left->type) || type_is_float(node->right->type)) {
      type_t *t = node->left->type;
      if (type_is_int(t)) {
        t = node->right->type;
      } else if (node->right->type->kind == TYPE_KIND_DOUBLE || node->right->type->kind == TYPE_KIND_LDOUBLE) {
        t = node->right->type;
      }
      assert(type_is_float(t));
      emit_cast(parse, t, node->left->type);
      emit_push_xmm(parse, 0);
      emit_expression(parse, node->right);
      emit_cast(parse, t, node->right->type);
      emit_comp_float(parse, op, t->kind == TYPE_KIND_DOUBLE || t->kind == TYPE_KIND_LDOUBLE);
    } else {
      emit_push(parse, "rcx");
      emit_push(parse, "rax");
      emit_expression(parse, node->right);
      emit_comp_int(parse, op, node->left->type->sign && node->right->type->sign);
      emit_pop(parse, "rcx");
    }
  } else {
    emit_push(parse, "rcx");
    emit_push(parse, "rax");
    emit_expression(parse, node->right);
    emit_arithmetic_int(parse, op, node->left);
    emit_pop(parse, "rcx");
  }
}

static void emit_unary_op_expression(parse_t *parse, node_t *node) {
  switch (node->op) {
  case OP_INC: case OP_DEC:
  case OP_PINC: case OP_PDEC:
    emit_expression(parse, node->operand);
    int n = 1;
    if (node->type->kind == TYPE_KIND_PTR) {
      n = node->type->parent->bytes;
    }
    if (node->op == OP_PINC || node->op == OP_PDEC) {
      emit_push(parse, "rax");
    }
    if (node->op == OP_INC || node->op == OP_PINC) {
      emitf("add $%d, %%rax", n);
    } else {
      emitf("sub $%d, %%rax", n);
    }
    emit_store(parse, node->operand, node->operand->type);
    if (node->op == OP_PINC || node->op == OP_PDEC) {
      emit_pop(parse, "rax");
    }
    break;
  case '+': case '-': case '~': case '!':
    emit_expression(parse, node->operand);
    switch (node->op) {
    case '-':
      emitf("neg %%rax");
      break;
    case '~':
      emitf("not %%rax");
      break;
    case '!':
      emitf("cmp $0, %%rax");
      emitf("sete %%al");
      emitf("movzb %%al, %%eax");
      break;
    }
    break;
  case '&':
    if (node->operand->kind == NODE_KIND_VARIABLE) {
      if (node->operand->global) {
        emitf("lea %s(%%rip), %%rax", node->operand->vname);
      } else {
        emitf("lea %d(%%rbp), %%rax", -node->operand->voffset);
      }
    } else if (node->operand->kind == NODE_KIND_BINARY_OP && node->operand->op == '.') {
      emit_expression(parse, node->operand->left);
      emitf("lea %d(%%rax), %%rax", node->operand->right->voffset);
    }
    break;
  case '*':
    emit_expression(parse, node->operand);
    if (node->type->kind == TYPE_KIND_ARRAY || node->type->kind == TYPE_KIND_STRUCT) {
      return;
    }
    if (node->type->kind == TYPE_KIND_FLOAT) {
      emitf("movss (%%rax), %%xmm0");
    } else if (node->type->kind == TYPE_KIND_DOUBLE || node->type->kind == TYPE_KIND_LDOUBLE) {
      emitf("movsd (%%rax), %%xmm0");
    } else {
      switch (node->type->bytes) {
      case 1:
        emitf("movzbq (%%rax), %%rax");
        break;
      case 2:
        emitf("movzwq (%%rax), %%rax");
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
    }
    break;
  case OP_CAST:
    emit_expression(parse, node->operand);
    emit_cast(parse, node->type, node->operand->type);
    break;
  }
}

static void emit_declaration_init_array(parse_t *parse, node_t *var, type_t *type, vector_t *vals, int offset) {
  assert(type->parent != NULL);
  for (int i = 0; i < vals->size; i++) {
    node_t *val = (node_t *)vals->data[i];
    if (val->kind == NODE_KIND_INIT_LIST) {
      if (type->parent->kind == TYPE_KIND_ARRAY) {
        emit_declaration_init_array(parse, var, type->parent, val->init_list, offset);
      } else {
        emit_declaration_init_struct(parse, var, type->parent, val->init_list, offset);
      }
    } else {
      emit_expression(parse, val);
      emit_cast(parse, type->parent, val->type);
      emit_save_to(parse, var, type->parent, offset);
    }
    offset += type->parent->total_size;
  }
}

static void emit_declaration_init_struct(parse_t *parse, node_t *var, type_t *type, vector_t *vals, int offset) {
  map_entry_t *e = type->fields->top;
  for (int i = 0; i < vals->size && e != NULL; i++, e = e->next) {
    node_t *val = (node_t *)vals->data[i];
    node_t *field = (node_t *)e->val;
    if (val->kind == NODE_KIND_INIT_LIST) {
      if (field->type->kind == TYPE_KIND_ARRAY) {
        emit_declaration_init_array(parse, var, field->type, val->init_list, offset + field->voffset);
      } else {
        emit_declaration_init_struct(parse, var, field->type, val->init_list, offset + field->voffset);
      }
    } else {
      emit_expression(parse, val);
      emit_cast(parse, field->type, val->type);
      emit_save_to(parse, var, field->type, offset + field->voffset);
    }
  }
}

static void emit_declaration_init(parse_t *parse, node_t *var, node_t *init) {
  if (var->type->kind == TYPE_KIND_ARRAY && init->kind == NODE_KIND_STRING_LITERAL) {
    int i = 0;
    char *p = init->sval->buf;
    for (i = 0; i < var->type->size - 1 && *p != '\0'; i++, p++) {
      emitf("mov $%d, %%al", *p);
      emit_save(parse, var, parse->type_char, 0, i);
    }
    emitf("mov $%d, %%al", '\0');
    emit_save(parse, var, parse->type_char, 0, i);
  } else if (init->kind == NODE_KIND_INIT_LIST) {
    if (var->type->kind == TYPE_KIND_ARRAY) {
      emit_declaration_init_array(parse, var, var->type, init->init_list, 0);
    } else {
      emit_declaration_init_struct(parse, var, var->type, init->init_list, 0);
    }
  } else {
    emit_expression(parse, init);
    emit_store(parse, var, init->type);
  }
}

static void emit_call(parse_t *parse, node_t *node) {
  int i;
  vector_t *iargs = vector_new(), *iargtypes = vector_new();
  vector_t *xargs = vector_new(), *xargtypes = vector_new();
  vector_t *rargs = vector_new(), *rargtypes = vector_new();
  for (i = 0; i < node->args->size; i++) {
    node_t *n = (node_t *)node->args->data[i];
    type_t *t = n->type;
    if (node->func->type != NULL && i < node->func->type->argtypes->size) {
      t = (type_t *)node->func->type->argtypes->data[i];
    }
    if (type_is_struct(t)) {
      vector_push(rargs, n);
      vector_push(rargtypes, t);
    } else if (type_is_float(t)) {
      vector_push(xargs->size < 8 ? xargs : rargs, n);
      vector_push(xargtypes->size < 8 ? xargtypes: rargtypes, t);
    } else {
      vector_push(iargs->size < 6 ? iargs : rargs, n);
      vector_push(iargtypes->size < 6 ? iargtypes : rargtypes, t);
    }
  }
  int old_stackpos = parse->stackpos;
  // store registers
  for (i = 0; i < iargs->size; i++) {
    emit_push(parse, REGS[i]);
  }
  for (i = 1; i < xargs->size; i++) {
    emit_push_xmm(parse, i);
  }
  // fix sp
  int padding = parse->stackpos % 16;
  emit_add_rsp(parse, -padding);
  // build arguments
  for (i = rargs->size - 1; i >= 0; i--) {
    node_t *n = (node_t *)rargs->data[i];
    type_t *t = (type_t *)rargtypes->data[i];
    emit_expression(parse, n);
    emit_cast(parse, t, n->type);
    if (type_is_struct(t)) {
      int size = t->total_size;
      align(&size, 8);
      emitf("mov %%rax, %%rsi");
      emit_add_rsp(parse, -size);
      emitf("mov %%rsp, %%rdi");
      emitf("mov $%d, %%rcx", size);
      emitf("rep movsb");
    } else if (type_is_float(t)) {
      emit_push_xmm(parse, 0);
    } else {
      emit_push(parse, "rax");
    }
  }
  for (i = xargs->size - 1; i >= 0 ; i--) {
    node_t *n = (node_t *)xargs->data[i];
    type_t *t = (type_t *)xargtypes->data[i];
    emit_expression(parse, n);
    emit_cast(parse, t, n->type);
    if (i != 0) {
      if (t->kind == TYPE_KIND_DOUBLE || t->kind == TYPE_KIND_LDOUBLE) {
        emitf("movsd %%xmm0, %%xmm%d", i);
      } else {
        emitf("movss %%xmm0, %%xmm%d", i);
      }
    }
  }
  for (i = 0; i < iargs->size; i++) {
    node_t *n = (node_t *)iargs->data[i];
    type_t *t = (type_t *)iargtypes->data[i];
    emit_expression(parse, n);
    emit_cast(parse, t, n->type);
    emitf("mov %%rax, %%%s", REGS[i]);
  }
  // call function
  emitf("mov $%d, %%eax", xargs->size);
  if (node->func->kind == NODE_KIND_IDENTIFIER) {
    emitf("call %s", node->func->identifier);
  } else if (node->func->kind == NODE_KIND_VARIABLE) {
    emitf("call %s", node->func->vname);
  } else {
    assert(node->func->kind == NODE_KIND_UNARY_OP && node->func->op == '*');
    emit_expression(parse, node->func->operand);
    emitf("call *%%rax");
  }
  // restore registers
  int rsize = padding;
  for (int i = 0; i < rargtypes->size; i++) {
    type_t *t = (type_t *)rargtypes->data[i];
    if (type_is_struct(t)) {
      int size = t->total_size;
      align(&size, 8);
      rsize += size;
    } else {
      rsize += 8;
    }
  }
  emit_add_rsp(parse, rsize);
  for (i = xargs->size - 1; i > 0; i--) {
    emit_pop_xmm(parse, i);
  }
  for (i = iargs->size - 1; i >= 0; i--) {
    emit_pop(parse, REGS[i]);
  }
  assert(old_stackpos == parse->stackpos);

  vector_free(iargs);
  vector_free(iargtypes);
  vector_free(xargs);
  vector_free(xargtypes);
  vector_free(rargs);
  vector_free(rargtypes);
}

static void emit_if(parse_t *parse, node_t *node) {
  emit_expression(parse, node->cond);
  emitf("test %%rax, %%rax");
  emitf("je .L%p", node->then_body);
  emit_expression(parse, node->then_body);
  if (node->else_body) {
    emitf("jmp .L%p", node->else_body);
    emitf(".L%p:", node->then_body);
    emit_expression(parse, node->else_body);
    emitf(".L%p:", node->else_body);
  } else {
    emitf(".L%p:", node->then_body);
  }
}

static void emit_while(parse_t *parse, node_t *node) {
  emitf(".L%p:", node);
  emit_expression(parse, node->lcond);
  emitf("test %%rax, %%rax");
  emitf("je .L%p", node->lbody);
  emit_expression(parse, node->lbody);
  emitf("jmp .L%p", node);
  emitf(".L%p:", node->lbody);
}

static void emit_do(parse_t *parse, node_t *node) {
  emitf(".L%p:", node);
  emit_expression(parse, node->lbody);
  emit_expression(parse, node->lcond);
  emitf("test %%rax, %%rax");
  emitf("je .L%p", node->lbody);
  emitf("jmp .L%p", node);
  emitf(".L%p:", node->lbody);
}

static void emit_for(parse_t *parse, node_t *node) {
  if (node->linit) {
    emit_expression(parse, node->linit);
  }
  if (node->lcond) {
    emitf("jmp .L%p", node->lcond);
  }
  emitf(".L%p:", node);
  emit_expression(parse, node->lbody);
  if (node->lstep) {
    emitf(".L%p:", node->lstep);
    emit_expression(parse, node->lstep);
  }
  if (node->lcond) {
    emitf(".L%p:", node->lcond);
    emit_expression(parse, node->lcond);
    emitf("test %%rax, %%rax");
    emitf("jne .L%p", node);
  }
  emitf(".L%p:", node->lbody);
}

static void emit_switch(parse_t *parse, node_t *node) {
  emit_expression(parse, node->sexpr);
  for (int i = 0; i < node->cases->size; i++) {
    node_t *n = (node_t *)node->cases->data[i];
    emitf("cmp $%ld, %%rax", n->cval->ival);
    emitf("je .L%p", n);
  }
  if (node->default_case != NULL) {
    emitf("jmp .L%p", node->default_case);
  } else {
    emitf("jmp .L%p", node->sbody);
  }
  emit_expression(parse, node->sbody);
  emitf(".L%p:", node->sbody);
}

static void emit_case(parse_t *parse, node_t *node) {
  emitf(".L%p:", node);
  emit_expression(parse, node->cstmt);
}

static void emit_continue(parse_t *parse, node_t *node) {
  assert(node->cscope->parent_node != NULL);
  node_t *loop_node = node->cscope->parent_node;
  if (loop_node->kind == NODE_KIND_FOR && loop_node->lstep != NULL) {
    emitf("jmp .L%p", loop_node->lstep);
  } else {
    emitf("jmp .L%p", loop_node);
  }
}

static void emit_break(parse_t *parse, node_t *node) {
  assert(node->cscope->parent_node != NULL);
  if (node->cscope->parent_node->kind == NODE_KIND_SWITCH) {
    emitf("jmp .L%p", node->cscope->parent_node->sbody);
  } else {
    emitf("jmp .L%p", node->cscope->parent_node->lbody);
  }
}

static void emit_return(parse_t *parse, node_t *node) {
  if (node->retval) {
    emit_expression(parse, node->retval);
  }
  emitf("leave");
  emitf("ret");
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
      case TYPE_KIND_SHORT:
      case TYPE_KIND_INT:
      case TYPE_KIND_LONG:
      case TYPE_KIND_LLONG:
        emit_int(parse, node);
        break;
      case TYPE_KIND_FLOAT:
      case TYPE_KIND_DOUBLE:
      case TYPE_KIND_LDOUBLE:
        emit_float(parse, node);
        break;
      default:
        errorf("unknown literal type: %d", node->type);
      }
      break;
    case NODE_KIND_STRING_LITERAL:
      emit_string(parse, node);
      break;
    case NODE_KIND_VARIABLE:
      emit_variable(parse, node);
      break;
    case NODE_KIND_DECLARATION:
      if (node->dec_init) {
        emit_declaration_init(parse, node->dec_var, node->dec_init);
      }
      break;
    case NODE_KIND_CALL:
      if (node->func->kind == NODE_KIND_VARIABLE) {
        if (strcmp("__builtin_va_start", node->func->vname) == 0) {
          emit_builtin_va_start(parse, node);
          break;
        }
      }
      emit_call(parse, node);
      break;
    case NODE_KIND_BLOCK:
      for (int i = 0; i < node->statements->size; i++) {
        emit_expression(parse, (node_t *)node->statements->data[i]);
      }
      break;
    case NODE_KIND_IF:
      emit_if(parse, node);
      break;
    case NODE_KIND_CONTINUE:
      emit_continue(parse, node);
      break;
    case NODE_KIND_BREAK:
      emit_break(parse, node);
      break;
    case NODE_KIND_RETURN:
      emit_return(parse, node);
      break;
    case NODE_KIND_WHILE:
      emit_while(parse, node);
      break;
    case NODE_KIND_DO:
      emit_do(parse, node);
      break;
    case NODE_KIND_FOR:
      emit_for(parse, node);
      break;
    case NODE_KIND_SWITCH:
      emit_switch(parse, node);
      break;
    case NODE_KIND_CASE:
      emit_case(parse, node);
      break;
    default:
      errorf("unknown expression node: %d", node->kind);
    }
  }
}

static int placement_variables(node_t *node, int offset) {
  for (map_entry_t *e = node->vars->top; e != NULL; e = e->next) {
    node_t *n = (node_t *)e->val;
    if (n->kind != NODE_KIND_VARIABLE || n->voffset != 0) {
      continue;
    }
    offset += n->type->total_size;
    align(&offset, n->type->align);
    n->voffset = offset;
  }
  int max_offset = offset;
  for (int i = 0; i < node->child_blocks->size; i++) {
    node_t *b = (node_t *)node->child_blocks->data[i];
    max_offset = max(max_offset, placement_variables(b, offset));
  }
  return max_offset;
}

static void emit_function(parse_t *parse, node_t *node) {
  node_t *old_function = parse->current_function;
  parse->current_function = node;

  parse->stackpos = 8;
  emitf(".text");
  node_t *var = node->fvar;
  if (var->sclass != STORAGE_CLASS_STATIC) {
    emitf_noindent(".global %s", var->vname);
  }
  emitf_noindent("%s:", var->vname);
  emit_push(parse, "rbp");
  emitf("mov %%rsp, %%rbp");

  vector_t *iregs = vector_new();
  vector_t *xregs = vector_new();
  vector_t *stack = vector_new();
  int offset = 0;

  for (int i = 0; i < node->fargs->size; i++) {
    node_t *n = (node_t *)node->fargs->data[i];
    if (type_is_float(n->type) && xregs->size < 8) {
      vector_push(xregs, n);
    } else if (!type_is_struct(n->type) && iregs->size < 6) {
      vector_push(iregs, n);
    } else {
      vector_push(stack, n);
    }
  }

  if (var->type->is_vaargs) {
    emit_add_rsp(parse, -(8 * 6 + 16 * 8));
  } else {
    emit_add_rsp(parse, -(8 * iregs->size + 16 * xregs->size));
  }

  // fix xregs positions
  if (var->type->is_vaargs) {
    for (int i = 8 - 1; i >= xregs->size; i--) {
      offset += 16;
      emitf("movsd %%xmm%d, %d(%%rbp)", i, -offset);
    }
  }
  for (int i = xregs->size - 1; i >= 0; i--) {
    node_t *n = (node_t *)xregs->data[i];
    offset += 16;
    n->voffset = offset;
  }
  for (int i = 0; i < xregs->size; i++) {
    node_t *n = (node_t *)xregs->data[i];
    if (n->type->kind == TYPE_KIND_FLOAT) {
      emitf("movss %%xmm%d, %d(%%rbp)", i, -n->voffset);
    } else {
      emitf("movsd %%xmm%d, %d(%%rbp)", i, -n->voffset);
    }
  }
  // fix iregs positions
  if (var->type->is_vaargs) {
    for (int i = 6 - 1; i >= iregs->size; i--) {
      offset += 8;
      emitf("movq %%%s, %d(%%rbp)", REGS[i], -offset);
    }
  }
  for (int i = iregs->size - 1; i >= 0 ; i--) {
    node_t *n = (node_t *)iregs->data[i];
    offset += 8;
    n->voffset = offset;
  }
  for (int i = 0; i < iregs->size; i++) {
    node_t *n = (node_t *)iregs->data[i];
    switch (n->type->bytes) {
    case 1:
      emitf("movl %%%s, %%eax", MREGS[i]);
      emitf("movb %%al, %d(%%rbp)", -n->voffset);
      break;
    case 2:
      emitf("movl %%%s, %%eax", MREGS[i]);
      emitf("movw %%ax, %d(%%rbp)", -n->voffset);
      break;
    case 4:
      emitf("movl %%%s, %d(%%rbp)", MREGS[i], -n->voffset);
      break;
    case 8:
      emitf("movq %%%s, %d(%%rbp)", REGS[i], -n->voffset);
      break;
    default:
      errorf("invalid variable type");
    }
  }
  // fix stack fargs positions
  int spoffset = 16;
  for (int i = 0; i < stack->size; i++) {
    node_t *n = (node_t *)stack->data[i];
    if (type_is_float(n->type)) {
      n->voffset = -spoffset;
      spoffset += 8;
    } else if (type_is_struct(n->type)) {
      int size = n->type->total_size;
      align(&size, 8);
      n->voffset = -spoffset;
      spoffset += size;
    } else {
      n->voffset = -spoffset;
      spoffset += 8;
    }
  }

  vector_free(iregs);
  vector_free(xregs);
  vector_free(stack);

  offset = placement_variables(node->fbody, offset);
  align(&offset, 8);

  emit_add_rsp(parse, -offset);
  emit_expression(parse, node->fbody);
  emitf("leave");
  emitf("ret");
  parse->current_function = old_function;
}

static void emit_global_data(parse_t *parse, type_t *type, node_t *val) {
  if (val == NULL) {
    emitf(".zero %d", type->total_size);
    return;
  }

  if (val->kind == NODE_KIND_LITERAL) {
    long n;
    switch (val->type->kind) {
    case TYPE_KIND_CHAR:
    case TYPE_KIND_INT:
      n = val->ival;
      break;
    default:
      errorf("unsupported global variable initialize type");
    }
    switch (type->bytes) {
    case 1:
      emitf(".byte %ld", n);
      break;
    case 2:
      emitf(".short %ld", n);
      break;
    case 4:
      emitf(".long %ld", n);
      break;
    case 8:
      emitf(".quad %ld", n);
      break;
    default:
      errorf("invalid variable type");
    }
  } else if (val->kind == NODE_KIND_STRING_LITERAL) {
    emitf_noindent(".STR_%d:", val->sid);
    emit_string_data(val->sval);
  } else {
    errorf("invalid data node type");
  }
}

static void emit_global_data_array(parse_t *parse, type_t *type, node_t *val) {
  int size = 0;

  if (val) {
    if (val->type->parent->kind == TYPE_KIND_CHAR) {
      size = val->type->size;
      emit_string_data(val->sval);
    } else {
      size = val->init_list->size;
      for (int i = 0; i < size; i++) {
        assert(type->parent != NULL);
        if (type->parent->kind == TYPE_KIND_ARRAY) {
          emit_global_data_array(parse, type->parent, (node_t *)val->init_list->data[i]);
        } else {
          emit_global_data(parse, type->parent, (node_t *)val->init_list->data[i]);
        }
      }
    }
  }
  int zero = type->total_size - type->parent->total_size * size;
  if (zero > 0) {
    emitf(".zero %d", zero);
  }
}

static void emit_global(parse_t *parse, node_t *node) {
  node_t *var = node->dec_var;
  if (var->sclass == STORAGE_CLASS_EXTERN) {
    return;
  }
  if (var->sclass != STORAGE_CLASS_STATIC) {
    emitf_noindent(".global %s", node->dec_var->vname);
  }
  emitf_noindent("%s:", node->dec_var->vname);
  if (node->type->kind == TYPE_KIND_ARRAY) {
    emit_global_data_array(parse, node->type, node->dec_init);
  } else {
    emit_global_data(parse, node->type, node->dec_init);
  }
}

static void emit_data_section(parse_t *parse) {
  vector_t *data = parse->data;
  emitf(".data");
  for (int i = 0; i < data->size; i++) {
    node_t *n = (node_t *)data->data[i];
    if (n->kind == NODE_KIND_LITERAL) {
      if (n->type->kind == TYPE_KIND_FLOAT) {
        float fval = n->fval;
        emitf_noindent(".FLT_%d:", n->fid);
        emitf(".long %d", *(uint32_t *)&fval);
      } else if (n->type->kind == TYPE_KIND_DOUBLE || n->type->kind == TYPE_KIND_LDOUBLE) {
        emitf_noindent(".DBL_%d:", n->fid);
        emitf(".quad %ld", *(uint64_t *)&n->fval);
      } else {
        errorf("the literal type is not supported yet: %s", n->type->name);
      }
    } else if (n->kind == NODE_KIND_STRING_LITERAL) {
      emitf_noindent(".STR_%d:", n->sid);
      emit_string_data(n->sval);
    } else {
      errorf("invalid data node type");
    }
  }
  for (int i = 0; i < parse->statements->size; i++) {
    node_t *node = (node_t *)parse->statements->data[i];
    if (node->kind != NODE_KIND_DECLARATION) {
      continue;
    }
    emit_global(parse, node);
  }
}

static void emit_builtin_va_start(parse_t *parse, node_t *node) {
  assert(parse->current_function != NULL);
  assert(node->args->size == 1);

  int gp = 0, fp = 0, last_offset = 8 * 6 + 16 * 8;
  vector_t *fargs = parse->current_function->fargs;

  for (int i = 0; i < fargs->size; i++) {
    node_t *n = (node_t *)fargs->data[i];
    if (type_is_struct(n->type)) {
      continue;
    }
    if (type_is_float(n->type)) {
      fp++;
    } else {
      if (gp < 6) {
        last_offset = n->voffset;
      }
      gp++;
    }
  }

  node_t *arg0 = (node_t *)node->args->data[0];
  emitf("movl $%d, %d(%%rbp)", gp * 8, -arg0->voffset); // gp_offset
  emitf("movl $%d, %d(%%rbp)", 8 * 6 + fp * 16, -arg0->voffset + 4); // fp_offset
  emitf("leaq 16(%%rbp), %%rax");
  emitf("movq %%rax, %d(%%rbp)", -arg0->voffset + 8); // overflow_arg_area
  emitf("leaq %d(%%rbp), %%rax", -last_offset);
  emitf("movq %%rax, %d(%%rbp)", -arg0->voffset + 16); // reg_save_area
}

void gen(parse_t *parse) {
  emit_data_section(parse);
  for (int i = 0; i < parse->statements->size; i++) {
    node_t *node = (node_t *)parse->statements->data[i];
    switch (node->kind) {
    case NODE_KIND_NOP:
      break;
    case NODE_KIND_FUNCTION:
      emit_function(parse, node);
      break;
    case NODE_KIND_DECLARATION:
      break;
    default:
      errorf("the node type is not supported at toplevel");
    }
  }
}
