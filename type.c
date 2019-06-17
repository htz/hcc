// Copyright 2019 @htz. Released under the MIT license.

#include <stdlib.h>
#include <string.h>
#include "hcc.h"

const char *type_kind_names[] = {
  "void",
  "_Bool",
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
  if (name != NULL) {
    t->name = strdup(name);
  } else {
    t->name = NULL;
  }
  t->kind = kind;
  t->sign = sign;
  t->sign = sign;
  t->parent = parent;
  switch (kind) {
  case TYPE_KIND_VOID:
    t->bytes = 1;
    t->align = 1;
    break;
  case TYPE_KIND_BOOL:
    t->bytes = 1;
    t->align = 1;
    break;
  case TYPE_KIND_CHAR:
    t->bytes = 1;
    t->align = 1;
    break;
  case TYPE_KIND_SHORT:
    t->bytes = 2;
    t->align = 2;
    break;
  case TYPE_KIND_INT:
  case TYPE_KIND_FLOAT:
    t->bytes = 4;
    t->align = 4;
    break;
  case TYPE_KIND_ARRAY:
    assert(t->parent != NULL);
    t->bytes = 0;
    t->align = t->parent->align;
    break;
  case TYPE_KIND_STRUCT:
    t->bytes = 0;
    t->align = 0;
    break;
  default:
    t->bytes = 8;
    t->align = 8;
  }
  t->size = size;
  if (size == 0) {
    t->total_size = t->bytes;
  } else {
    assert(t->parent != NULL);
    t->total_size = t->parent->total_size * size;
  }
  t->fields = NULL;
  t->is_struct = false;
  t->is_typedef = false;
  return t;
}

type_t *type_new(char *name, int kind, int sign, type_t *parent) {
  return type_new_with_size(name, kind, sign, parent, 0);
}

type_t *type_new_struct(char *name, bool is_struct) {
  type_t *t = type_new(name, TYPE_KIND_STRUCT, false, NULL);
  t->total_size = 0;
  t->fields = map_new();
  t->is_struct = is_struct;
  return t;
}

type_t *type_new_typedef(char *name, type_t *type) {
  type_t *t = (type_t *)malloc(sizeof (type_t));
  memcpy(t, type, sizeof(type_t));
  t->name = strdup(name);
  t->is_typedef = true;
  return t;
}

type_t *type_new_enum(char *name) {
  type_t *t = type_new(name, TYPE_KIND_ENUM, false, NULL);
  t->total_size = 0;
  return t;
}

type_t *type_new_stub() {
  type_t *t = type_new(NULL, TYPE_KIND_STUB, false, NULL);
  string_t *name = string_new();
  string_appendf(name, "$stub%p", t);
  t->name = strdup(name->buf);
  string_free(name);
  return t;
}

void type_free(type_t *t) {
  if (t->name != NULL) {
    free(t->name);
  }
  if (!t->is_typedef) {
    if (t->kind == TYPE_KIND_FUNCTION) {
      vector_free(t->argtypes);
    } else if (t->fields != NULL) {
      map_free(t->fields);
    }
  }
  free(t);
}

type_t *type_find(parse_t *parse, char *name) {
  for (node_t *scope = parse->current_scope; scope != NULL; scope = scope->parent_block) {
    type_t *type = (type_t *)map_get(scope->types, name);
    if (type != NULL) {
      return type;
    }
  }
  return (type_t *)map_get(parse->types, name);
}

void type_add(parse_t *parse, char *name, type_t *type) {
  map_t *types;
  if (parse->current_scope) {
    types = parse->current_scope->types;
  } else {
    types = parse->types;
  }
  assert(map_find(types, name) == NULL);
  map_add(types, name, type);
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

type_t *type_get_function(parse_t *parse, type_t *rettype, vector_t *argtypes) {
  string_t *name = string_new();
  string_appendf(name, "%s (*)(", rettype->name);
  for (int i = 0; i < argtypes->size; i++) {
    type_t *t = (type_t *)argtypes->data[i];
    if (i > 0) {
      string_append(name, ", ");
    }
    string_appendf(name, "%s", t->name);
  }
  string_append(name, ")");
  type_t *type = (type_t *)map_get(parse->types, name->buf);
  if (type == NULL) {
    type = type_new(name->buf, TYPE_KIND_FUNCTION, false, NULL);
    type->parent = rettype;
    type->argtypes = vector_dup(argtypes);
    map_add(parse->types, name->buf, type);
  }
  string_free(name);
  return type;
}

void type_add_by_tag(parse_t *parse, char *tag, type_t *type) {
  map_t *tags;
  if (parse->current_scope) {
    tags = parse->current_scope->tags;
  } else {
    tags = parse->tags;
  }
  map_add(tags, tag, type);
}

type_t *type_get_by_tag(parse_t *parse, char *tag, bool local_only) {
  if (local_only) {
    if (parse->current_scope) {
      return (type_t *)map_get(parse->current_scope->tags, tag);
    }
    return (type_t *)map_get(parse->tags, tag);
  }
  for (node_t *scope = parse->current_scope; scope != NULL; scope = scope->parent_block) {
    type_t *type = (type_t *)map_get(scope->tags, tag);
    if (type != NULL) {
      return type;
    }
  }
  return (type_t *)map_get(parse->tags, tag);
}

void type_add_typedef(parse_t *parse, char *name, type_t *type) {
  type = type_new_typedef(name, type);
  type_add(parse, name, type);
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

bool type_is_bool(type_t *type) {
  return type->kind == TYPE_KIND_BOOL;
}

bool type_is_int(type_t *type) {
  return
    type->kind == TYPE_KIND_BOOL ||
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

bool type_is_struct(type_t *type) {
  return type->kind == TYPE_KIND_STRUCT;
}

bool type_is_function(type_t *type) {
  return type->kind == TYPE_KIND_FUNCTION;
}
