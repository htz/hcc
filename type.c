// Copyright 2019 @htz. Released under the MIT license.

#include <stdlib.h>
#include <string.h>
#include "hcc.h"

const char *type_kind_names[] = {
  "void",
  "char",
  "short",
  "int",
  "long",
  "long long",
  "float",
  "double",
  "*",
  "[]",
};

type_t *type_new_with_size(char *name, int kind, int sign, type_t *parent, int size) {
  type_t *t = (type_t *)malloc(sizeof (type_t));
  t->name = strdup(name);
  t->kind = kind;
  t->sign = sign;
  t->sign = sign;
  t->parent = parent;
  switch (kind) {
  case TYPE_KIND_VOID:
    t->bytes = 1;
    break;
  case TYPE_KIND_CHAR:
    t->bytes = 1;
    break;
  case TYPE_KIND_SHORT:
    t->bytes = 2;
    break;
  case TYPE_KIND_INT:
  case TYPE_KIND_FLOAT:
    t->bytes = 4;
    break;
  default:
    t->bytes = 8;
  }
  t->size = size;
  if (size == 0) {
    t->total_size = t->bytes;
  } else {
    assert(t->parent != NULL);
    t->total_size = t->parent->total_size * size;
  }
  return t;
}

type_t *type_new(char *name, int kind, int sign, type_t *parent) {
  return type_new_with_size(name, kind, sign, parent, 0);
}

void type_free(type_t *t) {
  free(t->name);
  free(t);
}

type_t *type_find(parse_t *parse, char *name) {
  map_entry_t *e = map_find(parse->types, name);
  if (e == NULL) {
    return NULL;
  }
  return (type_t *)e->val;
}

void type_add(parse_t *parse, char *name, type_t *type) {
  assert(type_find(parse, name) == NULL);
  map_add(parse->types, name, type);
}

type_t *type_get(parse_t *parse, char *name, type_t *parent) {
  type_t *type = type_find(parse, name);
  if (type == NULL && parent != NULL) {
    type = type_new(name, TYPE_KIND_PTR, false, parent);
    type_add(parse, name, type);
  }
  return type;
}

type_t *type_get_ptr(parse_t *parse, type_t *type) {
  string_t *name = string_new_with(type->name);
  string_add(name, '*');
  type = type_get(parse, name->buf, type);
  if (type == NULL) {
    errorf("get pointer type error: &%s", type->name);
  }
  string_free(name);
  return type;
}

static char *array_leaf_type(type_t *type) {
  for (; type != NULL; type = type->parent) {
    if (type->kind != TYPE_KIND_ARRAY) {
      return type->name;
    }
  }
  errorf("array type error");
}

type_t *type_make_array(parse_t *parse, type_t *parent, int size) {
  string_t *str = string_new_with(array_leaf_type(parent));
  if (size >= 0) {
    string_appendf(str, "[%d]", size);
  } else {
    string_appendf(str, "[]");
  }
  for (type_t *p = parent; p != NULL; p = p->parent) {
    if (p->kind != TYPE_KIND_ARRAY) {
      break;
    }
    string_appendf(str, "[%d]", p->size);
  }
  type_t *type = type_find(parse, str->buf);
  if (type == NULL) {
    type = type_new_with_size(str->buf, TYPE_KIND_ARRAY, false, parent, size);
    type_add(parse, str->buf, type);
  }
  string_free(str);
  return type;
}

bool type_is_assignable(type_t *a, type_t *b) {
  if (a->kind == b->kind) {
    return true;
  }
  if (a->kind > b->kind) {
    type_t *tmp = a;
    a = b;
    b = tmp;
  }
  if (b->kind < TYPE_KIND_PTR) {
    return true;
  }
  if (type_is_int(a) && (b->kind == TYPE_KIND_PTR || b->kind == TYPE_KIND_ARRAY)) {
    return true;
  }
  if (
    (a->kind == TYPE_KIND_PTR || a->kind == TYPE_KIND_ARRAY) &&
    (b->kind == TYPE_KIND_PTR || b->kind == TYPE_KIND_ARRAY)
  ) {
    return type_is_assignable(a->parent, b->parent);
  }
  if (a->kind != TYPE_KIND_PTR && a->kind != TYPE_KIND_ARRAY) {
    return false;
  }
  return true;
}

const char *type_kind_names_str(int kind) {
  return type_kind_names[kind];
}

bool type_is_int(type_t *type) {
  return
    type->kind == TYPE_KIND_CHAR ||
    type->kind == TYPE_KIND_SHORT ||
    type->kind == TYPE_KIND_INT ||
    type->kind == TYPE_KIND_LONG ||
    type->kind == TYPE_KIND_LLONG;
}

bool type_is_float(type_t *type) {
  return
    type->kind == TYPE_KIND_FLOAT ||
    type->kind == TYPE_KIND_DOUBLE ||
    type->kind == TYPE_KIND_LDOUBLE;
}
