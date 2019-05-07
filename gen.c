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
static void emit_global_variable(parse_t *parse, node_t *node);
static void emit_variable(parse_t *parse, node_t *node);
static void emit_store(parse_t *parse, node_t *var);
static void emit_binary_op_expression(parse_t *parse, node_t *node);
static void emit_unary_op_expression(parse_t *parse, node_t *node);
static void emit_declaration_init(parse_t *parse, node_t *var, node_t *init);
static void emit_call(parse_t *parse, node_t *node);
static void emit_if(parse_t *parse, node_t *node);
static void emit_return(parse_t *parse, node_t *node);
static void emit_expression(parse_t *parse, node_t *node);
static void emit_function(parse_t *parse, node_t *node);
static void emit_global(parse_t *parse, node_t *node);
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
  if (node->type->kind == TYPE_KIND_ARRAY) {
    assert(node->type->parent != NULL);
    emitf("lea %s(%%rip), %%rax", node->vname);
    return;
  }
  switch (node->type->bytes) {
  case 1:
    emitf("movsbq %s(%%rip), %%rax", node->vname);
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

static void emit_variable(parse_t *parse, node_t *node) {
  if (node->global) {
    emit_global_variable(parse, node);
    return;
  }
  if (node->type->kind == TYPE_KIND_ARRAY) {
    assert(node->type->parent != NULL);
    emitf("lea %d(%%rbp), %%rax", -node->voffset);
    return;
  }
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

static void emit_save_global(parse_t *parse, node_t *var, type_t *type) {
  assert(type->kind != TYPE_KIND_ARRAY);
  switch (type->bytes) {
  case 1:
    emitf("mov %%al, %s(%%rip)", var->vname);
    break;
  case 4:
    emitf("mov %%eax, %s(%%rip)", var->vname);
    break;
  case 8:
    emitf("mov %%rax, %s(%%rip)", var->vname);
    break;
  default:
    errorf("invalid variable type");
  }
}

static void emit_save(parse_t *parse, node_t *var, type_t *type, int base, int at) {
  if (var->global) {
    assert(base == 0 && at == 0);
    emit_save_global(parse, var, type);
    return;
  }
  switch (type->bytes) {
  case 1:
    emitf("mov %%al, %d(%%rbp)", -var->voffset + base + type->bytes * at);
    break;
  case 4:
    emitf("mov %%eax, %d(%%rbp)", -var->voffset + base + type->bytes * at);
    break;
  case 8:
    emitf("mov %%rax, %d(%%rbp)", -var->voffset + base + type->bytes * at);
    break;
  default:
    errorf("invalid variable type");
  }
}

static void emit_store(parse_t *parse, node_t *var) {
  if (var->kind == NODE_KIND_UNARY_OP && var->op == '*') {
    emit_push(parse, "rax");
    emit_expression(parse, var->operand);
    emitf("mov %%rax, %%rcx");
    emit_pop(parse, "rax");
    switch (var->type->bytes) {
    case 1:
      emitf("mov %%al, (%%rcx)");
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
    return;
  }
  assert(var->kind == NODE_KIND_VARIABLE);
  emit_save(parse, var, var->type, 0, 0);
}

static void emit_binary_op_expression(parse_t *parse, node_t *node) {
  if (node->op == '=') {
    emit_expression(parse, node->right);
    emit_store(parse, node->left);
    return;
  }

  emit_expression(parse, node->left);
  emit_push(parse, "rcx");
  emit_push(parse, "rax");
  emit_expression(parse, node->right);
  if (node->left->type->kind == TYPE_KIND_PTR || node->left->type->kind == TYPE_KIND_ARRAY) {
    assert(node->left->type->parent != NULL);
    if (node->left->type->parent->total_size > 1) {
      emitf("imul $%d, %%rax", node->left->type->parent->total_size);
    }
  }
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
    if (node->operand->global) {
      emitf("lea %s(%%rip), %%rax", node->operand->vname);
    } else {
      emitf("lea %d(%%rbp), %%rax", -node->operand->voffset);
    }
    break;
  case '*':
    emit_expression(parse, node->operand);
    if (node->type->kind == TYPE_KIND_ARRAY) {
      return;
    }
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

static void emit_declaration_init_array(parse_t *parse, node_t *var, type_t *type, vector_t *vals, int offset) {
  assert(type->parent != NULL);
  for (int i = 0; i < vals->size; i++) {
    node_t *val = (node_t *)vals->data[i];
    if (val->kind != NODE_KIND_INIT_LIST) {
      emit_expression(parse, val);
      emit_save(parse, var, type->parent, offset, i);
    } else {
      emit_declaration_init_array(parse, var, type->parent, val->init_list, offset + type->parent->total_size * i);
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
  } else if (init->kind != NODE_KIND_INIT_LIST) {
    emit_expression(parse, init);
    emit_store(parse, var);
  } else {
    emit_declaration_init_array(parse, var, var->type, init->init_list, 0);
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
      case TYPE_KIND_INT:
        emit_int(parse, node);
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
    case NODE_KIND_RETURN:
      emit_return(parse, node);
      break;
    default:
      errorf("unknown expression node: %d", node->kind);
    }
  }
}

static int placement_variables(node_t *node, int offset) {
  for (map_entry_t *e = node->vars->top; e != NULL; e = e->next) {
    node_t *n = (node_t *)e->val;
    if (n->voffset != 0) {
      continue;
    }
    offset += n->type->total_size;
    align(&offset, 8);
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
  emitf(".text");
  emitf_noindent(".global %s", node->fvar->vname);
  emitf_noindent("%s:", node->fvar->vname);
  emit_push(parse, "rbp");
  emitf("mov %%rsp, %%rbp");

  int offset = 0, spoffset = 16;
  int i, iargs = 0;
  for (i = 0; i < node->fargs->size; i++) {
    node_t *n = (node_t *)node->fargs->data[i];
    if (iargs < 6) {
      offset += 8;
      align(&offset, 8);
      n->voffset = offset;
      emitf("mov %%%s, %d(%%rbp)", REGS[iargs++], -offset);
    } else {
      n->voffset = -spoffset;
      spoffset += 8;
    }
  }
  offset = placement_variables(node->fbody, offset);
  align(&offset, 8);

  emitf("sub $%d, %%rsp", offset);
  emit_expression(parse, node->fbody);
  emitf("leave");
  emitf("ret");
}

static void emit_global_data(parse_t *parse, type_t *type, node_t *val) {
  if (val == NULL) {
    emitf(".zero %d", type->total_size);
    return;
  }

  if (val->kind == NODE_KIND_LITERAL) {
    int n;
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
      emitf(".byte %d", n);
      break;
    case 4:
      emitf(".long %d", n);
      break;
    case 8:
      emitf(".quad %d", n);
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
  emitf_noindent(".global %s", node->dec_var->vname);
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
    emitf_noindent(".STR_%d:", n->sid);
    emit_string_data(n->sval);
  }
  for (int i = 0; i < parse->statements->size; i++) {
    node_t *node = (node_t *)parse->statements->data[i];
    if (node->kind != NODE_KIND_DECLARATION) {
      continue;
    }
    emit_global(parse, node);
  }
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
