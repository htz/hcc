// Copyright 2019 @htz. Released under the MIT license.

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hcc.h"

static node_t *external_declaration(parse_t *parse);
static node_t *function_definition(parse_t *parse, node_t *var);
static type_t *declaration_specifier(parse_t *parse, int *sclassp);
static type_t *struct_or_union_specifier(parse_t *parse, bool is_struct);
static void struct_declaration(parse_t *parse, type_t *type, type_t *field_type);
static void struct_declarator(parse_t *parse, type_t *type, type_t *field_type);
static type_t *declarator_array(parse_t *parse, type_t *type);
static node_t *variable_declarator(parse_t *parse, type_t *type);
static type_t *declarator(parse_t *parse, type_t *type, char **namep);
static type_t *direct_declarator(parse_t *parse, type_t *type, char **namep);
static node_t *eval_constant_expression(parse_t *parse, node_t *node);
static node_t *constant_expression(parse_t *parse);
static node_t *conditional_expression(parse_t *parse);
static node_t *logical_or_expression(parse_t *parse);
static node_t *logical_and_expression(parse_t *parse);
static node_t *inclusive_or_expression(parse_t *parse);
static node_t *exclusive_or_expression(parse_t *parse);
static node_t *and_expression(parse_t *parse);
static node_t *equality_expression(parse_t *parse);
static node_t *relational_expression(parse_t *parse);
static node_t *shift_expression(parse_t *parse);
static node_t *additive_expression(parse_t *parse);
static node_t *multiplicative_expression(parse_t *parse);
static node_t *cast_expression(parse_t *parse);
static node_t *unary_expression(parse_t *parse);
static vector_t *func_args(parse_t *parse);
static node_t *postfix_expression(parse_t *parse);
static node_t *primary_expression(parse_t *parse);
static node_t *expression(parse_t *parse);
static node_t *assignment_expression(parse_t *parse);
static type_t *type_name(parse_t *parse);
static type_t *abstract_declarator(parse_t *parse, type_t *type);
static type_t *enum_specifier(parse_t *parse);
static void enumerator_list(parse_t *parse, type_t *type);
static int enumerator(parse_t *parse, type_t *type, int n);
static node_t *declaration(parse_t *parse, type_t *type);
static node_t *init_declarator(parse_t *parse, node_t *var);
static node_t *initializer(parse_t *parse, type_t *type);
static node_t *compound_statement(parse_t *parse);
static node_t *statement(parse_t *parse);
static node_t *labeled_statement(parse_t *parse, int keyword);
static node_t *expression_statement(parse_t *parse);
static node_t *selection_statement(parse_t *parse, int keyword);
static node_t *iteration_statement(parse_t *parse, int keyword);
static node_t *jump_statement(parse_t *parse, int keyword);

static void add_var(parse_t *parse, node_t *var) {
  map_t *vars;
  if (parse->current_scope) {
    vars = parse->current_scope->vars;
  } else {
    vars = parse->vars;
  }
  map_entry_t *e = map_find(vars, var->vname);
  if (e != NULL) {
    errorf("already defined: %s", var->vname);
  }
  map_add(vars, var->vname, var);
}

static node_t *find_variable(parse_t *parse, node_t *scope, char *identifier) {
  map_entry_t *e;
  if (scope) {
    e = map_find(scope->vars, identifier);
    if (e == NULL) {
      return find_variable(parse, scope->parent_block, identifier);
    }
  } else {
    e = map_find(parse->vars, identifier);
    if (e == NULL) {
      return NULL;
    }
  }
  return (node_t *)e->val;
}

static type_t *op_result(parse_t *parse, int op, type_t *left, type_t *right) {
  if (left == right) {
    return left;
  }
  if (left->kind > right->kind) {
    type_t *tmp = left;
    left = right;
    right = tmp;
  }
  type_t *type = NULL;
  if (left->kind == right->kind) {
    assert(left->sign != right->sign);
    if (left->sign) {
      type = right;
    } else {
      type = left;
    }
  }
  switch (left->kind) {
  case TYPE_KIND_BOOL:
  case TYPE_KIND_CHAR:
  case TYPE_KIND_SHORT:
  case TYPE_KIND_INT:
  case TYPE_KIND_LONG:
  case TYPE_KIND_LLONG:
    switch (right->kind) {
    case TYPE_KIND_SHORT:
      if (left->sign != right->sign) {
        type = parse->type_ushort;
      } else {
        type = right;
      }
      break;
    case TYPE_KIND_INT:
      if (left->sign != right->sign) {
        type = parse->type_uint;
      } else {
        type = right;
      }
      break;
    case TYPE_KIND_LONG:
      if (left->sign != right->sign) {
        type = parse->type_ulong;
      } else {
        type = right;
      }
      break;
    case TYPE_KIND_LLONG:
      if (left->sign != right->sign) {
        type = parse->type_ullong;
      } else {
        type = right;
      }
      break;
    case TYPE_KIND_FLOAT:
    case TYPE_KIND_DOUBLE:
    case TYPE_KIND_LDOUBLE:
      type = right;
      break;
    case TYPE_KIND_PTR:
    case TYPE_KIND_ARRAY:
      if (
        op != '+' && op != ('+' | OP_ASSIGN_MASK) &&
        op != '-' && op != ('-' | OP_ASSIGN_MASK) &&
        op != '='
      ) {
        errorf("invalid operands to binary %c", op);
      }
      type = right;
      break;
    default:
      errorf("unknown type: %s", right->name);
    }
    break;
  case TYPE_KIND_FLOAT:
  case TYPE_KIND_DOUBLE:
  case TYPE_KIND_LDOUBLE:
    switch (right->kind) {
    case TYPE_KIND_FLOAT:
    case TYPE_KIND_DOUBLE:
    case TYPE_KIND_LDOUBLE:
      if (left->kind == TYPE_KIND_DOUBLE || left->kind == TYPE_KIND_LDOUBLE) {
        type = left;
      } else {
        type = right;
      }
      break;
    }
    break;
  case TYPE_KIND_PTR:
  case TYPE_KIND_ARRAY:
    if (op != '+' && op != '-') {
      errorf("invalid operands to binary %c", op);
    }
    type = right;
    break;
  }
  if (type == NULL) {
    errorf("invalid operands to binary expression ('%s' and '%s')", left->name, right->name);
  }
  return type;
}

static node_t *external_declaration(parse_t *parse) {
  int sclass = STORAGE_CLASS_NONE;
  type_t *type = declaration_specifier(parse, &sclass);
  if (type == NULL) {
    errorf("unknown type name");
  }
  if (sclass == STORAGE_CLASS_TYPEDEF) {
    for (;;) {
      node_t *var = variable_declarator(parse, type);
      type_add_typedef(parse, var->vname, var->type);
      if (lex_next_keyword_is(parse->lex, ';')) {
        break;
      }
      lex_expect_keyword_is(parse->lex, ',');
    }
    return node_new_nop(parse);
  }
  if (lex_next_keyword_is(parse->lex, ';')) {
    return node_new_nop(parse);
  }

  node_t *var = variable_declarator(parse, type);
  if (lex_next_keyword_is(parse->lex, '(')) {
    return function_definition(parse, var);
  }

  node_t *node = init_declarator(parse, var);
  node_t *prev = node;
  while (lex_next_keyword_is(parse->lex, ',')) {
    var = variable_declarator(parse, type);
    prev->next = init_declarator(parse, var);
    prev = prev->next;
  }
  lex_expect_keyword_is(parse->lex, ';');
  return node;
}

static node_t *function_parameter_declaration(parse_t *parse) {
  int sclass = STORAGE_CLASS_NONE;
  type_t *type = declaration_specifier(parse, &sclass);
  if (type == NULL) {
    errorf("unknown type name");
  }
  if (sclass == STORAGE_CLASS_TYPEDEF) {
    errorf("invalid storage class specifier in function declarator");
  }
  return variable_declarator(parse, type);
}

static node_t *function_definition(parse_t *parse, node_t *var) {
  vector_t *fargs = vector_new();
  node_t *scope = node_new_block(parse, BLOCK_KIND_DEFAULT, vector_new(), NULL);
  node_t *node = node_new_function(parse, var, fargs, NULL);
  parse->current_function = node;
  parse->current_scope = scope;
  parse->next_scope = scope;
  token_t *token = lex_next_token_is(parse->lex, TOKEN_KIND_IDENTIFIER);
  bool empty_args = false;
  if (token != NULL) {
    if (strcmp("void", token->identifier) == 0 && lex_next_keyword_is(parse->lex, ')')) {
      empty_args = true;
    } else {
      lex_unget_token(parse->lex, token);
    }
  }
  if (!empty_args && !lex_next_keyword_is(parse->lex, ')')) {
    for (;;) {
      node_t *arg = function_parameter_declaration(parse);
      if (arg->type->kind == TYPE_KIND_VOID) {
        errorf("void is not allowed");
      }
      add_var(parse, arg);
      vector_push(fargs, arg);
      if (lex_next_keyword_is(parse->lex, ')')) {
        break;
      }
      lex_expect_keyword_is(parse->lex, ',');
    }
  }
  lex_expect_keyword_is(parse->lex, '{');
  node->fbody = compound_statement(parse);
  parse->current_function = NULL;
  return node;
}

static type_t *select_type(parse_t *parse, type_t *type, int kind, int sign, int size) {
  if (kind == -1 && sign == -1 && size == -1) {
    if (type != NULL && type->kind == TYPE_KIND_ENUM) {
      return parse->type_int;
    }
    return type;
  }
  if (type != NULL) {
    errorf("two or more data types in declaration specifiers");
  }
  switch (kind) {
  case TYPE_KIND_VOID:
    if (size != -1) {
      errorf("'%s void' is invalid", type_kind_names_str(size));
    }
    if (sign != -1) {
      errorf("'void' cannot be signed or unsigned");
    }
    return parse->type_void;
  case TYPE_KIND_BOOL:
    if (size != -1) {
      errorf("'%s bool' is invalid", type_kind_names_str(size));
    }
    if (sign != -1) {
      errorf("'bool' cannot be signed or unsigned");
    }
    return parse->type_bool;
  case TYPE_KIND_CHAR:
    if (size != -1) {
      errorf("'%s char' is invalid", type_kind_names_str(size));
    }
    if (sign == -1) {
      return parse->type_char;
    }
    return sign == 0 ? parse->type_uchar : parse->type_schar;
  case TYPE_KIND_FLOAT:
    if (size != -1) {
      errorf("'%s float' is invalid", type_kind_names_str(size));
    }
    if (sign != -1) {
      errorf("'float' cannot be signed or unsigned");
    }
    return parse->type_float;
  case TYPE_KIND_DOUBLE:
    if (sign != -1) {
      errorf("'float' cannot be signed or unsigned");
    }
    if (size == TYPE_KIND_LONG) {
      return parse->type_ldouble;
    } else if (size != -1) {
      errorf("'%s float' is invalid", type_kind_names_str(size));
    }
    return parse->type_double;
  }
  if (size == TYPE_KIND_SHORT) {
    return sign == 0 ? parse->type_ushort : parse->type_short;
  }
  if (size == TYPE_KIND_LONG) {
    return sign == 0 ? parse->type_ulong : parse->type_long;
  }
  if (size == TYPE_KIND_LLONG) {
    return sign == 0 ? parse->type_ullong : parse->type_llong;
  }
  return sign == 0 ? parse->type_uint : parse->type_int;
}

static type_t *declaration_specifier(parse_t *parse, int *sclassp) {
  int kind = -1, sign = -1, size = -1;
  type_t *type = NULL;
  token_t *token = lex_next_token_is(parse->lex, TOKEN_KIND_IDENTIFIER);
  if (token != NULL) {
    lex_unget_token(parse->lex, token);
    if (find_variable(parse, parse->current_scope, token->identifier) != NULL) {
      return NULL;
    }
  }
  while ((token = lex_get_token(parse->lex)) != NULL) {
    if (token->kind == TOKEN_KIND_KEYWORD) {
      if (token->keyword == TOKEN_KEYWORD_TYPEDEF) {
        if (sclassp == NULL) {
          errorf("expected expression");
        }
        if (*sclassp != STORAGE_CLASS_NONE) {
          errorf("multiple storage classes in declaration specifiers");
        }
        *sclassp = STORAGE_CLASS_TYPEDEF;
        continue;
      } else if (token->keyword == TOKEN_KEYWORD_SIGNED) {
        sign = 1;
        continue;
      } else if (token->keyword == TOKEN_KEYWORD_UNSIGNED) {
        sign = 0;
        continue;
      } else if (token->keyword == TOKEN_KEYWORD_STRUCT || token->keyword == TOKEN_KEYWORD_UNION) {
        if (type != NULL) {
          errorf("cannot combine with previous '%s' declaration specifier", type->name);
        }
        type = struct_or_union_specifier(parse, token->keyword == TOKEN_KEYWORD_STRUCT);
        break;
      } else if (token->keyword == TOKEN_KEYWORD_ENUM) {
        if (type != NULL) {
          errorf("cannot combine with previous '%s' declaration specifier", type->name);
        }
        type = enum_specifier(parse);
        break;
      }
    } else if (token->kind == TOKEN_KIND_IDENTIFIER) {
      type_t *t = type_get(parse, token->identifier, NULL);
      if (t == parse->type_short) {
        size = TYPE_KIND_SHORT;
        continue;
      } else if (t == parse->type_long) {
        if (size == TYPE_KIND_LONG) {
          size = TYPE_KIND_LLONG;
        } else if (size == -1) {
          size = TYPE_KIND_LONG;
        } else {
          errorf("cannot combine with previous '%s' declaration specifier", type_kind_names_str(size));
        }
        continue;
      } else if (t != NULL) {
        if (t->is_typedef) {
          if (type != NULL || kind != -1) {
            lex_unget_token(parse->lex, token);
            break;
          }
          type = t;
        } else {
          if (kind != -1) {
            errorf("cannot combine with previous '%s' declaration specifier", type_kind_names_str(kind));
          }
          kind = t->kind;
        }
        continue;
      }
    }
    lex_unget_token(parse->lex, token);
    break;
  }
  return select_type(parse, type, kind, sign, size);
}

static type_t *make_empty_struct_type(parse_t *parse, token_t *tag, bool is_struct) {
  type_t *type = type_new_struct(NULL, is_struct);
  string_t *name = string_new_with(is_struct ? "struct " : "union ");
  if (tag != NULL) {
    string_append(name, tag->identifier);
  } else {
    string_appendf(name, "$%p", type);
  }
  type->name = strdup(name->buf);
  string_free(name);

  type_add(parse, type->name, type);
  if (tag != NULL) {
    type_add_by_tag(parse, tag->identifier, type);
  }

  return type;
}

static type_t *struct_or_union_specifier(parse_t *parse, bool is_struct) {
  token_t *tag = lex_next_token_is(parse->lex, TOKEN_KIND_IDENTIFIER);
  type_t *type = NULL;

  if (tag != NULL) {
    type = type_get_by_tag(parse, tag->identifier, false);
  }
  if (!lex_next_keyword_is(parse->lex, '{')) {
    if (type == NULL) {
      type = make_empty_struct_type(parse, tag, is_struct);
    } else if (type->is_struct != is_struct) {
      errorf("use of '%s' with tag type that does not match previous declaration", tag->identifier);
    }
    return type;
  }
  if (type == NULL || type_get_by_tag(parse, tag->identifier, true) == NULL) {
    type = make_empty_struct_type(parse, tag, is_struct);
  } else if (type->fields->size > 0) {
    errorf("redefinition of '%s'", tag->identifier);
  }
  while (!lex_next_keyword_is(parse->lex, '}')) {
    int sclass = STORAGE_CLASS_NONE;
    type_t *field_type = declaration_specifier(parse, &sclass);
    if (sclass != STORAGE_CLASS_NONE) {
      errorf("type name does not allow storage class to be specified");
    }
    struct_declaration(parse, type, field_type);
  }
  align(&type->total_size, type->align);

  return type;
}

static void struct_declaration(parse_t *parse, type_t *type, type_t *field_type) {
  for (;;) {
    struct_declarator(parse, type, field_type);
    if (lex_next_keyword_is(parse->lex, ';')) {
      break;
    }
    lex_expect_keyword_is(parse->lex, ',');
  }
  align(&type->total_size, type->align);
}

static void struct_declarator(parse_t *parse, type_t *type, type_t *field_type) {
  token_t *token = lex_next_keyword_is(parse->lex, ';');
  if (field_type->kind == TYPE_KIND_STRUCT && token != NULL) {
    lex_unget_token(parse->lex, token);
    align(&type->total_size, 4);
    for (map_entry_t *e = field_type->fields->top; e != NULL; e = e->next) {
      node_t *field = (node_t *)e->val;
      if (map_find(type->fields, field->vname) != NULL) {
        errorf("duplicate member: %s", field->vname);
      }
      node_t *node = node_new_variable(parse, field->type, field->vname, false);
      if (type->is_struct) {
        align(&type->total_size, field->type->align);
        node->voffset = type->total_size;
        type->total_size += field->type->total_size;
      } else if (type->total_size < field->type->total_size) {
        type->total_size = field->type->total_size;
      }
      map_add(type->fields, field->vname, node);
    }
    if (type->align < field_type->align) {
      type->align = field_type->align;
    }
    return;
  }
  if (token != NULL) {
    lex_unget_token(parse->lex, token);
    return;
  }

  char *name;
  field_type = declarator(parse, field_type, &name);
  if (field_type->kind == TYPE_KIND_VOID) {
    errorf("void is not allowed");
  }
  if (map_find(type->fields, name) != NULL) {
    errorf("duplicate member: %s", name);
  }
  node_t *node = node_new_variable(parse, field_type, name, false);
  if (type->is_struct) {
    align(&type->total_size, field_type->align);
    node->voffset = type->total_size;
    type->total_size += field_type->total_size;
  } else if (type->total_size < field_type->total_size) {
    type->total_size = field_type->total_size;
  }
  map_add(type->fields, name, node);
  if (type->align < field_type->align) {
    type->align = field_type->align;
  }
}

static type_t *declarator_array(parse_t *parse, type_t *type) {
  if (lex_next_keyword_is(parse->lex, '[')) {
    node_t *size = constant_expression(parse);
    lex_expect_keyword_is(parse->lex, ']');
    type_t *parent_type = declarator_array(parse, type);
    type = type_make_array(parse, parent_type, size->ival);
  }

  return type;
}

static node_t *variable_declarator(parse_t *parse, type_t *type) {
  char *name;
  type = declarator(parse, type, &name);
  if (parse->current_function) {
    return node_new_variable(parse, type, name, false);
  }
  return node_new_variable(parse, type, name, true);
}

static type_t *declarator(parse_t *parse, type_t *type, char **namep) {
  while (lex_next_keyword_is(parse->lex, '*')) {
    type = type_get_ptr(parse, type);
  }
  return direct_declarator(parse, type, namep);
}

static type_t *direct_declarator(parse_t *parse, type_t *type, char **namep) {
  if (lex_next_keyword_is(parse->lex, '(')) {
    type = declarator(parse, type, namep);
    lex_expect_keyword_is(parse->lex, ')');
    return type;
  }

  if (namep) {
    token_t *token = lex_expect_token_is(parse->lex, TOKEN_KIND_IDENTIFIER);
    *namep = token->identifier;
  }
  if (lex_next_keyword_is(parse->lex, '[')) {
    node_t *size = NULL;
    if (!lex_next_keyword_is(parse->lex, ']')) {
      size = constant_expression(parse);
      lex_expect_keyword_is(parse->lex, ']');
    }
    type_t *parent_type = declarator_array(parse, type);
    if (size == NULL) {
      type = type_make_array(parse, parent_type, -1);
    } else {
      type = type_make_array(parse, parent_type, size->ival);
    }
  }
  return type;
}

static node_t *eval_constant_binary_expression_int(parse_t *parse, int op, node_t *left, node_t *right) {
  assert(type_is_int(left->type) && type_is_int(right->type));
  long l = left->ival, r = right->ival;
  switch (op) {
  case '+':
    return node_new_int(parse, parse->type_long, l + r);
  case '-':
    return node_new_int(parse, parse->type_long, l - r);
  case '*':
    return node_new_int(parse, parse->type_long, l * r);
  case '^':
    return node_new_int(parse, parse->type_long, l ^ r);
  case '/':
    return node_new_int(parse, parse->type_long, l / r);
  case '%':
    return node_new_int(parse, parse->type_long, l % r);
  case OP_SAL:
    return node_new_int(parse, parse->type_long, l << r);
  case OP_SAR:
    return node_new_int(parse, parse->type_long, l >> r);
  case '&':
    return node_new_int(parse, parse->type_long, l & r);
  case '|':
    return node_new_int(parse, parse->type_long, l | r);
  case OP_EQ:
    return node_new_int(parse, parse->type_bool, l == r);
  case OP_NE:
    return node_new_int(parse, parse->type_bool, l != r);
  case '<':
    return node_new_int(parse, parse->type_bool, l < r);
  case OP_LE:
    return node_new_int(parse, parse->type_bool, l <= r);
  case '>':
    return node_new_int(parse, parse->type_bool, l > r);
  case OP_GE:
    return node_new_int(parse, parse->type_bool, l >= r);
  case OP_ANDAND:
    return node_new_int(parse, parse->type_bool, l && r);
  case OP_OROR:
    return node_new_int(parse, parse->type_bool, l || r);
  }
  errorf("unsupported int binary expression for constant expression");
}

static node_t *eval_constant_binary_expression_float(parse_t *parse, int op, node_t *left, node_t *right) {
  double l, r;
  if (type_is_float(left->type)) {
    l = left->fval;
  } else {
    l = (double)left->ival;
  }
  if (type_is_float(right->type)) {
    r = right->fval;
  } else {
    r = (double)right->ival;
  }
  switch (op) {
  case '+':
    return node_new_float(parse, parse->type_double, l + r, -1);
  case '-':
    return node_new_float(parse, parse->type_double, l - r, -1);
  case '*':
    return node_new_float(parse, parse->type_double, l * r, -1);
  case '/':
    return node_new_float(parse, parse->type_double, l / r, -1);
  case OP_EQ:
    return node_new_int(parse, parse->type_bool, l == r);
  case OP_NE:
    return node_new_int(parse, parse->type_bool, l != r);
  case '<':
    return node_new_int(parse, parse->type_bool, l < r);
  case OP_LE:
    return node_new_int(parse, parse->type_bool, l <= r);
  case '>':
    return node_new_int(parse, parse->type_bool, l > r);
  case OP_GE:
    return node_new_int(parse, parse->type_bool, l >= r);
  case OP_ANDAND:
    return node_new_int(parse, parse->type_bool, l && r);
  case OP_OROR:
    return node_new_int(parse, parse->type_bool, l || r);
  }
  errorf("unsupported float binary expression for constant expression");
}

static node_t *eval_constant_binary_expression(parse_t *parse, node_t *node) {
  node_t *left = eval_constant_expression(parse, node->left);
  node_t *right = eval_constant_expression(parse, node->right);
  if (left->kind != NODE_KIND_LITERAL || right->kind != NODE_KIND_LITERAL) {
    errorf("invalid constant expression");
  }
  if (type_is_int(node->type)) {
    return eval_constant_binary_expression_int(parse, node->op, left, right);
  } else {
    return eval_constant_binary_expression_float(parse, node->op, left, right);
  }
  errorf("unsupported binary expression for constant expression");
}

static node_t *eval_constant_unary_expression(parse_t *parse, node_t *node) {
  node_t *val = eval_constant_expression(parse, node->operand);
  if (val->kind != NODE_KIND_LITERAL) {
    errorf("invalid constant expression");
  }
  switch (node->op) {
  case '+':
    return val;
  case '-':
    if (type_is_int(val->type)) {
      val->ival = -val->ival;
    } else {
      val->fval = -val->fval;
    }
    return val;
  case '~':
    if (type_is_int(val->type)) {
      val->ival = ~val->ival;
    } else {
      errorf("invalid argument type '%s' to unary expression", val->type->name);
    }
    return val;
  case '!':
    if (type_is_int(val->type)) {
      val->ival = !val->ival;
    } else {
      val = node_new_int(parse, parse->type_bool, !val->fval);
    }
    return val;
  case OP_CAST:
    if (type_is_int(node->type)) {
      if (type_is_int(val->type)) {
        return val;
      }
      return node_new_int(parse, parse->type_long, val->fval);
    } else if (type_is_float(node->type)) {
      if (type_is_float(val->type)) {
        return val;
      }
      return node_new_float(parse, parse->type_double, val->fval, -1);
    }
    errorf("unsupported cast for constant expression");
    break;
  }
  errorf("unsupported unary expression for constant expression");
}

static node_t *eval_constant_expression(parse_t *parse, node_t *node) {
  switch (node->kind) {
  case NODE_KIND_BINARY_OP:
    return eval_constant_binary_expression(parse, node);
  case NODE_KIND_UNARY_OP:
    return eval_constant_unary_expression(parse, node);
  case NODE_KIND_LITERAL:
    switch (node->type->kind) {
    case TYPE_KIND_CHAR:
    case TYPE_KIND_SHORT:
    case TYPE_KIND_INT:
    case TYPE_KIND_LONG:
    case TYPE_KIND_LLONG:
    case TYPE_KIND_FLOAT:
    case TYPE_KIND_DOUBLE:
      return node;
    default:
      errorf("unsupported literal '%s' for constant expression", node->type->name);
    }
    break;
  case NODE_KIND_IF:
    if (eval_constant_expression(parse, node->cond)) {
      return eval_constant_expression(parse, node->then_body);
    }
    return eval_constant_expression(parse, node->else_body);
  }
  errorf("unsupported node type for constant expression");
}

static node_t *constant_expression(parse_t *parse) {
  node_t *node = conditional_expression(parse);
  node = eval_constant_expression(parse, node);
  if (node->kind != NODE_KIND_LITERAL || !type_is_int(node->type)) {
    errorf("integer expected, but got %d", node->kind);
  }
  return node;
}

static node_t *conditional_expression(parse_t *parse) {
  node_t *node = logical_or_expression(parse);
  if (lex_next_keyword_is(parse->lex, '?')) {
    node_t *then_body = expression(parse);
    lex_expect_keyword_is(parse->lex, ':');
    node_t *else_body = conditional_expression(parse);
    if (!type_is_assignable(then_body->type, else_body->type)) {
      errorf("incompatible operand types ('%s' and '%s')", then_body->type->name, else_body->type->name);
    }
    node = node_new_if(parse, node, then_body, else_body);
    node->type = then_body->type;
  }
  return node;
}

static node_t *logical_or_expression(parse_t *parse) {
  node_t *node = logical_and_expression(parse);
  for (;;) {
    if (!lex_next_keyword_is(parse->lex, OP_OROR)) {
      break;
    }
    node_t *right = logical_and_expression(parse);
    node = node_new_binary_op(parse, parse->type_bool, OP_OROR, node, right);
  }
  return node;
}

static node_t *logical_and_expression(parse_t *parse) {
  node_t *node = inclusive_or_expression(parse);
  for (;;) {
    if (!lex_next_keyword_is(parse->lex, OP_ANDAND)) {
      break;
    }
    node_t *right = inclusive_or_expression(parse);
    node = node_new_binary_op(parse, parse->type_bool, OP_ANDAND, node, right);
  }
  return node;
}

static node_t *inclusive_or_expression(parse_t *parse) {
  node_t *node = exclusive_or_expression(parse);
  for (;;) {
    if (!lex_next_keyword_is(parse->lex, '|')) {
      break;
    }
    node_t *right = exclusive_or_expression(parse);
    if (!type_is_int(node->type) || !type_is_int(right->type)) {
      errorf("invalid operands to binary expression ('%s' and '%s')", node->type->name, right->type->name);
    }
    node = node_new_binary_op(parse, parse->type_int, '|', node, right);
  }
  return node;
}

static node_t *exclusive_or_expression(parse_t *parse) {
  node_t *node = and_expression(parse);
  for (;;) {
    if (!lex_next_keyword_is(parse->lex, '^')) {
      break;
    }
    node_t *right = and_expression(parse);
    if (!type_is_int(node->type) || !type_is_int(right->type)) {
      errorf("invalid operands to binary expression ('%s' and '%s')", node->type->name, right->type->name);
    }
    node = node_new_binary_op(parse, parse->type_int, '^', node, right);
  }
  return node;
}

static node_t *and_expression(parse_t *parse) {
  node_t *node = equality_expression(parse);
  for (;;) {
    if (!lex_next_keyword_is(parse->lex, '&')) {
      break;
    }
    node_t *right = equality_expression(parse);
    if (!type_is_int(node->type) || !type_is_int(right->type)) {
      errorf("invalid operands to binary expression ('%s' and '%s')", node->type->name, right->type->name);
    }
    node = node_new_binary_op(parse, parse->type_int, '&', node, right);
  }
  return node;
}

static node_t *equality_expression(parse_t *parse) {
  node_t *node = relational_expression(parse);
  for (;;) {
    int op;
    if (lex_next_keyword_is(parse->lex, OP_EQ)) {
      op = OP_EQ;
    } else if (lex_next_keyword_is(parse->lex, OP_NE)) {
      op = OP_NE;
    } else {
      break;
    }
    node_t *right = relational_expression(parse);
    node = node_new_binary_op(parse, parse->type_bool, op, node, right);
  }
  return node;
}

static node_t *relational_expression(parse_t *parse) {
  node_t *node = shift_expression(parse);
  for (;;) {
    int op;
    if (lex_next_keyword_is(parse->lex, '<')) {
      op = '<';
    } else if (lex_next_keyword_is(parse->lex, OP_LE)) {
      op = OP_LE;
    } else if (lex_next_keyword_is(parse->lex, '>')) {
      op = '>';
    } else if (lex_next_keyword_is(parse->lex, OP_GE)) {
      op = OP_GE;
    } else {
      break;
    }
    node_t *right = shift_expression(parse);
    node = node_new_binary_op(parse, parse->type_bool, op, node, right);
  }
  return node;
}

static node_t *shift_expression(parse_t *parse) {
  node_t *node = additive_expression(parse);
  for (;;) {
    int op;
    if (lex_next_keyword_is(parse->lex, OP_SAL)) {
      op = OP_SAL;
    } else if (lex_next_keyword_is(parse->lex, OP_SAR)) {
      op = OP_SAR;
    } else {
      break;
    }
    node_t *right = additive_expression(parse);
    if (!type_is_int(node->type) || !type_is_int(right->type)) {
      errorf("invalid operands to binary expression ('%s' and '%s')", node->type->name, right->type->name);
    }
    node = node_new_binary_op(parse, node->type, op, node, right);
  }
  return node;
}

static node_t *additive_expression(parse_t *parse) {
  node_t *node = multiplicative_expression(parse);
  for (;;) {
    int op;
    if (lex_next_keyword_is(parse->lex, '+')) {
      op = '+';
    } else if (lex_next_keyword_is(parse->lex, '-')) {
      op = '-';
    } else {
      break;
    }
    node_t *right = multiplicative_expression(parse);
    type_t *type = op_result(parse, op, node->type, right->type);
    if (
      (node->type->kind != TYPE_KIND_PTR && node->type->kind != TYPE_KIND_ARRAY) &&
      (right->type->kind == TYPE_KIND_PTR || right->type->kind == TYPE_KIND_ARRAY)
    ) {
      node_t *tmp = node;
      node = right;
      right = tmp;
    }
    node = node_new_binary_op(parse, type, op, node, right);
  }
  return node;
}

static node_t *multiplicative_expression(parse_t *parse) {
  node_t *node = cast_expression(parse);
  for (;;) {
    int op;
    if (lex_next_keyword_is(parse->lex, '*')) {
      op = '*';
    } else if (lex_next_keyword_is(parse->lex, '/')) {
      op = '/';
    } else if (lex_next_keyword_is(parse->lex, '%')) {
      op = '%';
    } else {
      break;
    }
    node_t *right = cast_expression(parse);
    type_t *type = op_result(parse, op, node->type, right->type);
    node = node_new_binary_op(parse, type, op, node, right);
  }
  return node;
}

static node_t *cast_expression(parse_t *parse) {
  token_t *token = lex_next_keyword_is(parse->lex, '(');
  if (token == NULL) {
    return unary_expression(parse);
  }
  type_t *type = type_name(parse);
  if (type == NULL) {
    lex_unget_token(parse->lex, token);
    return unary_expression(parse);
  }
  lex_expect_keyword_is(parse->lex, ')');
  if (type->kind == TYPE_KIND_ARRAY) {
    errorf("cast to incomplete type '%s'", type->name);
  }
  node_t *node = cast_expression(parse);
  if (node->type->kind == TYPE_KIND_STRUCT) {
    errorf("operand of type '%s' where arithmetic or pointer type is required", node->type->name);
  }
  if (type->kind == TYPE_KIND_ARRAY || type->kind == TYPE_KIND_STRUCT) {
    errorf("used type '%s' where arithmetic or pointer type is required", type->name);
  }
  return node_new_unary_op(parse, type, OP_CAST, node);
}

static node_t *unary_expression(parse_t *parse) {
  if (lex_next_keyword_is(parse->lex, OP_INC)) {
    node_t *node = unary_expression(parse);
    if (node->kind == NODE_KIND_VARIABLE || (node->kind == NODE_KIND_UNARY_OP && node->op == '*') || (node->kind == NODE_KIND_BINARY_OP && node->op == '.')) {
      return node_new_unary_op(parse, node->type, OP_INC, node);
    } else {
      errorf("expression is not assignable");
    }
  } else if (lex_next_keyword_is(parse->lex, OP_DEC)) {
    node_t *node = unary_expression(parse);
    if (node->kind == NODE_KIND_VARIABLE || (node->kind == NODE_KIND_UNARY_OP && node->op == '*') || (node->kind == NODE_KIND_BINARY_OP && node->op == '.')) {
      return node_new_unary_op(parse, node->type, OP_DEC, node);
    } else {
      errorf("expression is not assignable");
    }
  } else if (lex_next_keyword_is(parse->lex, '+')) {
    node_t *node = unary_expression(parse);
    if (node->kind == NODE_KIND_LITERAL && node->type->kind == TYPE_KIND_INT) {
      return node;
    }
    return node_new_unary_op(parse, node->type, '+', node);
  } else if (lex_next_keyword_is(parse->lex, '-')) {
    node_t *node = unary_expression(parse);
    if (node->kind == NODE_KIND_LITERAL && node->type->kind == TYPE_KIND_INT) {
      node->ival *= -1;
      return node;
    }
    return node_new_unary_op(parse, node->type, '-', node);
  } else if (lex_next_keyword_is(parse->lex, '~')) {
    node_t *node = unary_expression(parse);
    if (node->kind == NODE_KIND_LITERAL && node->type->kind == TYPE_KIND_INT) {
      node->ival = ~node->ival;
      return node;
    }
    return node_new_unary_op(parse, node->type, '~', node);
  } else if (lex_next_keyword_is(parse->lex, '!')) {
    node_t *node = unary_expression(parse);
    if (node->kind == NODE_KIND_LITERAL && node->type->kind == TYPE_KIND_INT) {
      node->ival = !node->ival;
      return node;
    }
    return node_new_unary_op(parse, node->type, '!', node);
  } else if (lex_next_keyword_is(parse->lex, '*')) {
    node_t *node = unary_expression(parse);
    if (node->type->parent == NULL) {
      errorf("pointer type expected, but got %s", node->type->name);
    }
    return node_new_unary_op(parse, node->type->parent, '*', node);
  } else if (lex_next_keyword_is(parse->lex, '&')) {
    node_t *node = unary_expression(parse);
    type_t *type = type_get_ptr(parse, node->type);
    if (node->kind == NODE_KIND_VARIABLE || (node->kind == NODE_KIND_UNARY_OP && node->op == '*') || (node->kind == NODE_KIND_BINARY_OP && node->op == '.')) {
      return node_new_unary_op(parse, type, '&', node);
    } else {
      errorf("cannot take the address of an rvalue of type '%s'", type->name);
    }
  } else if (lex_next_keyword_is(parse->lex, OP_SIZEOF)) {
    type_t *type = NULL;
    token_t *token = lex_next_keyword_is(parse->lex, '(');
    if (token != NULL) {
      type = type_name(parse);
      if (type == NULL) {
        lex_unget_token(parse->lex, token);
      } else {
        lex_expect_keyword_is(parse->lex, ')');
      }
    }
    if (type == NULL) {
      node_t *node = unary_expression(parse);
      type = node->type;
    }
    return node_new_int(parse, parse->type_uint, type->total_size);
  }
  return postfix_expression(parse);
}

static vector_t *func_args(parse_t *parse) {
  vector_t *args = vector_new();
  if (lex_next_keyword_is(parse->lex, ')')) {
    return args;
  }
  for (;;) {
    node_t *arg = assignment_expression(parse);
    vector_push(args, arg);
    if (lex_next_keyword_is(parse->lex, ')')) {
      break;
    }
    lex_expect_keyword_is(parse->lex, ',');
  }
  return args;
}

static node_t *postfix_expression(parse_t *parse) {
  node_t *node = primary_expression(parse);
  for (;;) {
    if (lex_next_keyword_is(parse->lex, '(')) {
      vector_t *args = func_args(parse);
      node = node_new_call(parse, parse->type_int, node, args);
    } else if (node->kind == NODE_KIND_IDENTIFIER) {
      node_t *var = find_variable(parse, parse->current_scope, node->identifier);
      if (var == NULL) {
        errorf("Undefined varaible: %s", node->identifier);
      }
      node = var;
    } else if (lex_next_keyword_is(parse->lex, '[')) {
      if (node->type->parent == NULL) {
        errorf("pointer or array type expected, but got %s", node->type->name);
      }
      type_t *type = type_get_ptr(parse, node->type->parent);
      node = node_new_binary_op(parse, type, '+', node, expression(parse));
      lex_expect_keyword_is(parse->lex, ']');
      node = node_new_unary_op(parse, type->parent, '*', node);
    } else if (lex_next_keyword_is(parse->lex, '.')) {
      if (node->type->kind != TYPE_KIND_STRUCT) {
        errorf("member reference base type '%s' is not a structure or union", node->type->name);
      }
      token_t *token = lex_expect_token_is(parse->lex, TOKEN_KIND_IDENTIFIER);
      map_entry_t *e = map_find(node->type->fields, token->identifier);
      if (e == NULL) {
        errorf("no member named '%s' in '%s'", token->identifier, node->type->fields);
      }
      node_t *var = (node_t *)e->val;
      node = node_new_binary_op(parse, var->type, '.', node, var);
    } else if (lex_next_keyword_is(parse->lex, OP_ARROW)) {
      if (node->type->kind == TYPE_KIND_STRUCT) {
        errorf("member reference type '%s' is not a pointer; did you mean to use '.'?", node->type->name);
      }
      if (node->type->kind != TYPE_KIND_PTR || node->type->parent->kind != TYPE_KIND_STRUCT) {
        errorf("member reference base type '%s' is not a structure or union", node->type->name);
      }
      token_t *token = lex_expect_token_is(parse->lex, TOKEN_KIND_IDENTIFIER);
      map_entry_t *e = map_find(node->type->parent->fields, token->identifier);
      if (e == NULL) {
        errorf("no member named '%s' in '%s'", token->identifier, node->type->fields);
      }
      node_t *var = (node_t *)e->val;
      node = node_new_unary_op(parse, node->type->parent, '*', node);
      node = node_new_binary_op(parse, var->type, '.', node, var);
    } else if (lex_next_keyword_is(parse->lex, OP_INC)) {
      if (node->kind == NODE_KIND_VARIABLE || (node->kind == NODE_KIND_UNARY_OP && node->op == '*') || (node->kind == NODE_KIND_BINARY_OP && node->op == '.')) {
        node = node_new_unary_op(parse, node->type, OP_PINC, node);
      } else {
        errorf("expression is not assignable");
      }
    } else if (lex_next_keyword_is(parse->lex, OP_DEC)) {
      if (node->kind == NODE_KIND_VARIABLE || (node->kind == NODE_KIND_UNARY_OP && node->op == '*') || (node->kind == NODE_KIND_BINARY_OP && node->op == '.')) {
        node = node_new_unary_op(parse, node->type, OP_PDEC, node);
      } else {
        errorf("expression is not assignable");
      }
    } else {
      break;
    }
  }
  return node;
}

static node_t *primary_expression(parse_t *parse) {
  token_t *token = lex_get_token(parse->lex);
  node_t *node;
  switch (token->kind) {
  case TOKEN_KIND_KEYWORD:
    if (token->keyword == '(') {
      node = expression(parse);
      lex_expect_keyword_is(parse->lex, ')');
      return node;
    }
    errorf("unknown keyword");
  case TOKEN_KIND_IDENTIFIER:
    {
      node_t *var = find_variable(parse, parse->current_scope, token->identifier);
      if (var != NULL && var->kind == NODE_KIND_LITERAL && type_is_int(var->type)) {
        return node_new_int(parse, var->type, var->ival);
      }
      return node_new_identifier(parse, token->identifier);
    }
  case TOKEN_KIND_CHAR:
  case TOKEN_KIND_INT:
    return node_new_int(parse, parse->type_int, token->ival);
  case TOKEN_KIND_UINT:
    return node_new_int(parse, parse->type_uint, token->ival);
  case TOKEN_KIND_LONG:
    return node_new_int(parse, parse->type_long, token->ival);
  case TOKEN_KIND_ULONG:
    return node_new_int(parse, parse->type_ulong, token->ival);
  case TOKEN_KIND_LLONG:
    return node_new_int(parse, parse->type_llong, token->ival);
  case TOKEN_KIND_ULLONG:
    return node_new_int(parse, parse->type_ullong, token->ival);
  case TOKEN_KIND_FLOAT:
    if (parse->current_function) {
      node = node_new_float(parse, parse->type_float, token->fval, parse->data->size);
      vector_push(parse->data, node);
    } else {
      node = node_new_float(parse, parse->type_float, token->fval, -1);
    }
    return node;
  case TOKEN_KIND_DOUBLE:
    if (parse->current_function) {
      node = node_new_float(parse, parse->type_double, token->fval, parse->data->size);
      vector_push(parse->data, node);
    } else {
      node = node_new_float(parse, parse->type_double, token->fval, -1);
    }
    return node;
  case TOKEN_KIND_STRING:
    if (parse->current_function) {
      node = node_new_string(parse, token->sval, parse->data->size);
      vector_push(parse->data, node);
    } else {
      node = node_new_string(parse, token->sval, -1);
    }
    for (;;) {
      token = lex_get_token(parse->lex);
      if (token->kind != TOKEN_KIND_STRING) {
        lex_unget_token(parse->lex, token);
        break;
      }
      string_append(node->sval, token->sval->buf);
    }
    return node;
  }
  errorf("unknown token: %s", token_str(token));
}

static node_t *expression(parse_t *parse) {
  node_t *node = assignment_expression(parse);
  node_t *prev = node;
  while (lex_next_keyword_is(parse->lex, ',')) {
    prev->next = assignment_expression(parse);
    prev = prev->next;
  }
  return node;
}

static node_t *assignment_expression(parse_t *parse) {
  node_t *node = conditional_expression(parse);
  for (;;) {
    int op;
    if (lex_next_keyword_is(parse->lex, '=')) {
      op = '=';
    } else if (lex_next_keyword_is(parse->lex, '+' | OP_ASSIGN_MASK)) {
      op = '+' | OP_ASSIGN_MASK;
    } else if (lex_next_keyword_is(parse->lex, '-' | OP_ASSIGN_MASK)) {
      op = '-' | OP_ASSIGN_MASK;
    } else if (lex_next_keyword_is(parse->lex, '*' | OP_ASSIGN_MASK)) {
      op = '*' | OP_ASSIGN_MASK;
    } else if (lex_next_keyword_is(parse->lex, '/' | OP_ASSIGN_MASK)) {
      op = '/' | OP_ASSIGN_MASK;
    } else if (lex_next_keyword_is(parse->lex, '%' | OP_ASSIGN_MASK)) {
      op = '%' | OP_ASSIGN_MASK;
    } else if (lex_next_keyword_is(parse->lex, '&' | OP_ASSIGN_MASK)) {
      op = '&' | OP_ASSIGN_MASK;
    } else if (lex_next_keyword_is(parse->lex, '|' | OP_ASSIGN_MASK)) {
      op = '|' | OP_ASSIGN_MASK;
    } else if (lex_next_keyword_is(parse->lex, '^' | OP_ASSIGN_MASK)) {
      op = '^' | OP_ASSIGN_MASK;
    } else if (lex_next_keyword_is(parse->lex, OP_SAL | OP_ASSIGN_MASK)) {
      op = OP_SAL | OP_ASSIGN_MASK;
    } else if (lex_next_keyword_is(parse->lex, OP_SAR | OP_ASSIGN_MASK)) {
      op = OP_SAR | OP_ASSIGN_MASK;
    } else {
      break;
    }
    if (node->kind == NODE_KIND_VARIABLE || (node->kind == NODE_KIND_UNARY_OP && node->op == '*') || (node->kind == NODE_KIND_BINARY_OP && node->op == '.')) {
      node_t *right = assignment_expression(parse);
      type_t *type = op_result(parse, op, node->type, right->type);
      node = node_new_binary_op(parse, type, op, node, right);
    } else {
      errorf("expression is not assignable");
    }
  }
  return node;
}

static type_t *type_name(parse_t *parse) {
  type_t *type = declaration_specifier(parse, NULL);
  if (type == NULL) {
    return NULL;
  }
  type = abstract_declarator(parse, type);
  return type;
}

static type_t *abstract_declarator(parse_t *parse, type_t *type) {
  return declarator(parse, type, NULL);
}

static type_t *make_empty_enum_type(parse_t *parse, token_t *tag) {
  type_t *type = type_new_enum(NULL);
  string_t *name = string_new_with("enum ");
  if (tag != NULL) {
    string_append(name, tag->identifier);
  } else {
    string_appendf(name, "$%p", type);
  }
  type->name = strdup(name->buf);
  string_free(name);

  type_add(parse, type->name, type);
  if (tag != NULL) {
    type_add_by_tag(parse, tag->identifier, type);
  }

  return type;
}

static type_t *enum_specifier(parse_t *parse) {
  token_t *tag = lex_next_token_is(parse->lex, TOKEN_KIND_IDENTIFIER);
  type_t *type = NULL;

  if (tag != NULL) {
    type = type_get_by_tag(parse, tag->identifier, false);
  }
  if (!lex_next_keyword_is(parse->lex, '{')) {
    if (type == NULL) {
      type = make_empty_enum_type(parse, tag);
    }
    return type;
  }
  if (type == NULL || type_get_by_tag(parse, tag->identifier, true) == NULL) {
    type = make_empty_enum_type(parse, tag);
  } else if (type->fields->size > 0) { // ここ！！！！！！！！！！！
    errorf("redefinition of '%s'", tag->identifier);
  }
  enumerator_list(parse, type);

  return type;
}

static void enumerator_list(parse_t *parse, type_t *type) {
  for (int n = 0; !lex_next_keyword_is(parse->lex, '}'); n++) {
    n = enumerator(parse, type, n);
    if (!lex_next_keyword_is(parse->lex, ',')) {
      lex_expect_keyword_is(parse->lex, '}');
      break;
    }
  }
}

static int enumerator(parse_t *parse, type_t *type, int n) {
  token_t *name = lex_next_token_is(parse->lex, TOKEN_KIND_IDENTIFIER);
  node_t *val;
  if (lex_next_keyword_is(parse->lex, '=')) {
    val = constant_expression(parse);
  } else {
    val = node_new_int(parse, parse->type_int, n);
  }

  map_t *vars;
  if (parse->current_scope) {
    vars = parse->current_scope->vars;
  } else {
    vars = parse->vars;
  }
  map_entry_t *e = map_find(vars, name->identifier);
  if (e != NULL) {
    errorf("previous definition is here");
  }
  map_add(vars, name->identifier, val);

  return val->ival;
}

static node_t *declaration(parse_t *parse, type_t *type) {
  node_t *var = variable_declarator(parse, type);
  node_t *node = init_declarator(parse, var);
  node_t *prev = node;
  while (lex_next_keyword_is(parse->lex, ',')) {
    var = variable_declarator(parse, type);
    prev->next = init_declarator(parse, var);
    prev = prev->next;
  }
  lex_expect_keyword_is(parse->lex, ';');
  return node;
}

static node_t *init_declarator(parse_t *parse, node_t *var) {
  node_t *init = NULL;
  if (var->type->kind == TYPE_KIND_VOID) {
    errorf("void is not allowed");
  }
  if (lex_next_keyword_is(parse->lex, '=')) {
    init = initializer(parse, var->type);
  }
  if (var->type->kind == TYPE_KIND_ARRAY && var->type->size < 0) {
    if (init == NULL) {
      errorf("definition of variable with array type needs an explicit size or an initializer");
    }
    if (init->type->kind != TYPE_KIND_ARRAY) {
      errorf("array initializer must be an initializer list");
    }
    assert(var->type->parent != NULL);
    if (init->kind == NODE_KIND_STRING_LITERAL) {
      var->type = type_make_array(parse, var->type->parent, init->type->size);
    } else {
      var->type = type_make_array(parse, var->type->parent, init->init_list->size);
    }
  }
  add_var(parse, var);
  return node_new_declaration(parse, var->type, var, init);
}

static node_t *initializer(parse_t *parse, type_t *type) {
  node_t *init;
  if (lex_next_keyword_is(parse->lex, '{')) {
    vector_t *nodes = vector_new();
    if (type->kind == TYPE_KIND_ARRAY) {
      while (!lex_next_keyword_is(parse->lex, '}')) {
        node_t *node = initializer(parse, type->parent);
        vector_push(nodes, node);
        if (!lex_next_keyword_is(parse->lex, ',')) {
          lex_expect_keyword_is(parse->lex, '}');
          break;
        }
      }
    } else if (type->kind == TYPE_KIND_STRUCT) {
      map_entry_t *e = type->fields->top;
      while (!lex_next_keyword_is(parse->lex, '}')) {
        node_t *field = (node_t *)e->val;
        node_t *node = initializer(parse, field->type);
        vector_push(nodes, node);
        if (!lex_next_keyword_is(parse->lex, ',')) {
          lex_expect_keyword_is(parse->lex, '}');
          break;
        }
        e = e->next;
      }
    } else {
      errorf("pointer or array type expected, but got %s", type->name);
    }
    init = node_new_init_list(parse, type, nodes);
  } else {
    init = assignment_expression(parse);
    if (!type_is_assignable(type, init->type)) {
      errorf("different type of initialzation value, %s expected but got %s", type->name, init->type->name);
    }
  }
  return init;
}

static node_t *compound_statement(parse_t *parse) {
  node_t *node = parse->next_scope;
  if (node == NULL) {
    node = node_new_block(parse, BLOCK_KIND_DEFAULT, vector_new(), parse->current_scope);
    parse->current_scope = node;
  } else {
    assert(parse->current_scope == node);
    parse->next_scope = NULL;
  }
  vector_t *statements = node->statements;
  while (!lex_next_keyword_is(parse->lex, '}')) {
    int sclass = STORAGE_CLASS_NONE;
    type_t *type = declaration_specifier(parse, &sclass);
    if (type != NULL) {
      if (sclass == STORAGE_CLASS_TYPEDEF) {
        for (;;) {
          node_t *var = variable_declarator(parse, type);
          type_add_typedef(parse, var->vname, var->type);
          if (lex_next_keyword_is(parse->lex, ';')) {
            break;
          }
          lex_expect_keyword_is(parse->lex, ',');
        }
        continue;
      } else if (lex_next_keyword_is(parse->lex, ';')) {
        continue;
      }
      vector_push(statements, declaration(parse, type));
    } else {
      vector_push(statements, statement(parse));
    }
  }
  parse->current_scope = node->parent_block;
  return node;
}

static node_t *statement(parse_t *parse) {
  if (lex_next_keyword_is(parse->lex, TOKEN_KEYWORD_IF)) {
    return selection_statement(parse, TOKEN_KEYWORD_IF);
  }
  if (lex_next_keyword_is(parse->lex, TOKEN_KEYWORD_SWITCH)) {
    return selection_statement(parse, TOKEN_KEYWORD_SWITCH);
  }
  if (lex_next_keyword_is(parse->lex, TOKEN_KEYWORD_CASE)) {
    return labeled_statement(parse, TOKEN_KEYWORD_CASE);
  }
  if (lex_next_keyword_is(parse->lex, TOKEN_KEYWORD_DEFAULT)) {
    return labeled_statement(parse, TOKEN_KEYWORD_DEFAULT);
  }
  if (lex_next_keyword_is(parse->lex, TOKEN_KEYWORD_CONTINUE)) {
    return jump_statement(parse, TOKEN_KEYWORD_CONTINUE);
  }
  if (lex_next_keyword_is(parse->lex, TOKEN_KEYWORD_BREAK)) {
    return jump_statement(parse, TOKEN_KEYWORD_BREAK);
  }
  if (lex_next_keyword_is(parse->lex, TOKEN_KEYWORD_RETURN)) {
    return jump_statement(parse, TOKEN_KEYWORD_RETURN);
  }
  if (lex_next_keyword_is(parse->lex, TOKEN_KEYWORD_WHILE)) {
    return iteration_statement(parse, TOKEN_KEYWORD_WHILE);
  }
  if (lex_next_keyword_is(parse->lex, TOKEN_KEYWORD_DO)) {
    return iteration_statement(parse, TOKEN_KEYWORD_DO);
  }
  if (lex_next_keyword_is(parse->lex, TOKEN_KEYWORD_FOR)) {
    return iteration_statement(parse, TOKEN_KEYWORD_FOR);
  }
  if (lex_next_keyword_is(parse->lex, '{')) {
    return compound_statement(parse);
  }
  return expression_statement(parse);
}

static node_t *labeled_statement(parse_t *parse, int keyword) {
  if (keyword == TOKEN_KEYWORD_CASE) {
    node_t *exp = constant_expression(parse);
    lex_expect_keyword_is(parse->lex, ':');
    node_t *body = statement(parse);
    return node_new_case(parse, exp, body);
  } else if (keyword == TOKEN_KEYWORD_DEFAULT) {
    lex_expect_keyword_is(parse->lex, ':');
    node_t *body = statement(parse);
    return node_new_case(parse, NULL, body);
  }
  errorf("internal error");
}

static node_t *expression_statement(parse_t *parse) {
  if (lex_next_keyword_is(parse->lex, ';')) {
    return node_new_nop(parse);
  }
  node_t *node = expression(parse);
  lex_expect_keyword_is(parse->lex, ';');
  return node;
}

static node_t *selection_statement(parse_t *parse, int keyword) {
  if (keyword == TOKEN_KEYWORD_IF) {
    lex_expect_keyword_is(parse->lex, '(');
    node_t *cond = expression(parse);
    lex_expect_keyword_is(parse->lex, ')');
    node_t *then_body = statement(parse);
    node_t *else_body = NULL;
    if (lex_next_keyword_is(parse->lex, TOKEN_KEYWORD_ELSE)) {
      else_body = statement(parse);
    }
    return node_new_if(parse, cond, then_body, else_body);
  } else if (TOKEN_KEYWORD_SWITCH) {
    node_t *scope = node_new_block(parse, BLOCK_KIND_SWITCH, vector_new(), parse->current_scope);
    parse->current_scope = scope;
    parse->next_scope = scope;
    lex_expect_keyword_is(parse->lex, '(');
    node_t *expr = expression(parse);
    lex_expect_keyword_is(parse->lex, ')');
    node_t *node = node_new_switch(parse, expr, scope);
    if (lex_next_keyword_is(parse->lex, '{')) {
      compound_statement(parse);
    } else {
      parse->next_scope = NULL;
      vector_push(scope->statements,  statement(parse));
      parse->current_scope = scope->parent_block;
    }
    return node;
  }
  errorf("internal error");
}

static node_t *iteration_statement(parse_t *parse, int keyword) {
  if (keyword == TOKEN_KEYWORD_WHILE) {
    lex_expect_keyword_is(parse->lex, '(');
    node_t *cond = expression(parse);
    lex_expect_keyword_is(parse->lex, ')');
    node_t *body;
    if (lex_next_keyword_is(parse->lex, '{')) {
      node_t *scope = node_new_block(parse, BLOCK_KIND_LOOP, vector_new(), parse->current_scope);
      parse->current_scope = scope;
      parse->next_scope = scope;
      body = compound_statement(parse);
    } else {
      body = statement(parse);
    }
    return node_new_while(parse, cond, body);
  } else if (keyword == TOKEN_KEYWORD_DO) {
    node_t *body;
    if (lex_next_keyword_is(parse->lex, '{')) {
      node_t *scope = node_new_block(parse, BLOCK_KIND_LOOP, vector_new(), parse->current_scope);
      parse->current_scope = scope;
      parse->next_scope = scope;
      body = compound_statement(parse);
    } else {
      body = statement(parse);
    }
    lex_expect_keyword_is(parse->lex, TOKEN_KEYWORD_WHILE);
    lex_expect_keyword_is(parse->lex, '(');
    node_t *cond = expression(parse);
    lex_expect_keyword_is(parse->lex, ')');
    lex_expect_keyword_is(parse->lex, ';');
    return node_new_do(parse, cond, body);
  } else if (keyword == TOKEN_KEYWORD_FOR) {
    node_t *scope = node_new_block(parse, BLOCK_KIND_LOOP, vector_new(), parse->current_scope);
    parse->current_scope = scope;
    parse->next_scope = scope;
    node_t *init = NULL, *cond = NULL, *step = NULL;
    lex_expect_keyword_is(parse->lex, '(');
    if (!lex_next_keyword_is(parse->lex, ';')) {
      int sclass = STORAGE_CLASS_NONE;
      type_t *type = declaration_specifier(parse, &sclass);
      if (type != NULL) {
        if (sclass == STORAGE_CLASS_TYPEDEF) {
          errorf("type name does not allow storage class to be specified");
        }
        init = declaration(parse, type);
      } else {
        init = expression(parse);
        lex_expect_keyword_is(parse->lex, ';');
      }
    }
    if (!lex_next_keyword_is(parse->lex, ';')) {
      cond = expression(parse);
      lex_expect_keyword_is(parse->lex, ';');
    }
    if (!lex_next_keyword_is(parse->lex, ')')) {
      step = expression(parse);
      lex_expect_keyword_is(parse->lex, ')');
    }
    node_t *body = NULL;
    if (lex_next_keyword_is(parse->lex, '{')) {
      body = compound_statement(parse);
    } else {
      parse->next_scope = NULL;
      body = statement(parse);
      vector_push(scope->statements, body);
      parse->current_scope = scope->parent_block;
      body = scope;
    }
    return node_new_for(parse, init, cond, step, body);
  }
  errorf("internal error");
}

static node_t *jump_statement(parse_t *parse, int keyword) {
  assert(parse->current_function != NULL);
  switch (keyword) {
  case TOKEN_KEYWORD_CONTINUE:
    lex_expect_keyword_is(parse->lex, ';');
    return node_new_continue(parse);
  case TOKEN_KEYWORD_BREAK:
    lex_expect_keyword_is(parse->lex, ';');
    return node_new_break(parse);
  }

  assert(keyword == TOKEN_KEYWORD_RETURN);
  node_t *var = parse->current_function->fvar;
  assert(var != NULL);
  type_t *type = var->type;
  node_t *exp = NULL;
  if (!lex_next_keyword_is(parse->lex, ';')) {
    if (type->kind == TYPE_KIND_VOID) {
      errorf("void function '%s' should not return a value", var->vname);
    }
    exp = expression(parse);
    if (!type_is_assignable(exp->type, type)) {
      errorf("different returning expression type from '%s' to '%s'", exp->type->name, type->name);
    }
    lex_expect_keyword_is(parse->lex, ';');
  } else {
    if (type->kind != TYPE_KIND_VOID) {
      errorf("non-void function '%s' should return a value", var->vname);
    }
  }
  return node_new_return(parse, parse->current_function->type, exp);
}

static parse_t *parse_new(FILE *fp) {
  parse_t *parse = (parse_t *)malloc(sizeof (parse_t));
  parse->lex = lex_new(fp);
  parse->data = vector_new();
  parse->statements = vector_new();
  parse->nodes = vector_new();
  parse->vars = map_new();
  parse->types = map_new();
  parse->tags = map_new();
  parse->current_scope = NULL;
  parse->next_scope = NULL;

  parse->type_void = type_new("void", TYPE_KIND_VOID, false, NULL);
  map_add(parse->types, parse->type_void->name, parse->type_void);
  parse->type_bool = type_new("_Bool", TYPE_KIND_BOOL, true, NULL);
  type_add(parse, parse->type_bool->name, parse->type_bool);
  parse->type_char = type_new("char", TYPE_KIND_CHAR, true, NULL);
  type_add(parse, parse->type_char->name, parse->type_char);
  parse->type_schar = type_new("signed char", TYPE_KIND_CHAR, true, NULL);
  type_add(parse, parse->type_schar->name, parse->type_schar);
  parse->type_uchar = type_new("unsigned char", TYPE_KIND_CHAR, false, NULL);
  type_add(parse, parse->type_uchar->name, parse->type_uchar);
  parse->type_short = type_new("short", TYPE_KIND_SHORT, true, NULL);
  type_add(parse, parse->type_short->name, parse->type_short);
  parse->type_ushort = type_new("unsigned short", TYPE_KIND_SHORT, false, NULL);
  type_add(parse, parse->type_ushort->name, parse->type_ushort);
  parse->type_int = type_new("int", TYPE_KIND_INT, true, NULL);
  type_add(parse, parse->type_int->name, parse->type_int);
  parse->type_uint = type_new("unsigned int", TYPE_KIND_INT, false, NULL);
  type_add(parse, parse->type_uint->name, parse->type_uint);
  parse->type_long = type_new("long", TYPE_KIND_LONG, true, NULL);
  type_add(parse, parse->type_long->name, parse->type_long);
  parse->type_ulong = type_new("unsigned long", TYPE_KIND_LONG, false, NULL);
  type_add(parse, parse->type_ulong->name, parse->type_ulong);
  parse->type_llong = type_new("long long", TYPE_KIND_LLONG, true, NULL);
  type_add(parse, parse->type_llong->name, parse->type_llong);
  parse->type_ullong = type_new("unsigned long long", TYPE_KIND_LLONG, false, NULL);
  type_add(parse, parse->type_ullong->name, parse->type_ullong);
  parse->type_float = type_new("float", TYPE_KIND_FLOAT, false, NULL);
  type_add(parse, parse->type_float->name, parse->type_float);
  parse->type_double = type_new("double", TYPE_KIND_DOUBLE, false, NULL);
  type_add(parse, parse->type_double->name, parse->type_double);
  parse->type_ldouble = type_new("long double", TYPE_KIND_LDOUBLE, false, NULL);
  type_add(parse, parse->type_ldouble->name, parse->type_ldouble);

  type_add_typedef(parse, "bool", parse->type_bool);

  return parse;
}

void parse_free(parse_t *parse) {
  lex_free(parse->lex);
  vector_free(parse->data);
  vector_free(parse->statements);
  for (int i = 0; i < parse->nodes->size; i++) {
    node_free((node_t *)parse->nodes->data[i]);
  }
  vector_free(parse->nodes);
  map_free(parse->vars);
  parse->types->free_val_fn = (void (*)(void *))type_free;
  map_free(parse->types);
  map_free(parse->tags);
  free(parse);
}

parse_t *parse_file(FILE *fp) {
  parse_t *parse = parse_new(fp);
  for (;;) {
    if (lex_next_token_is(parse->lex, TOKEN_KIND_EOF)) {
      break;
    }
    vector_push(parse->statements, external_declaration(parse));
  }
  return parse;
}
