// Copyright 2019 @htz. Released under the MIT license.

#include <stdlib.h>
#include <string.h>
#include "hcc.h"

type_t *type_new(char *name, int kind, type_t *parent) {
  type_t *t = (type_t *)malloc(sizeof (type_t));
  t->name = strdup(name);
  t->kind = kind;
  t->parent = parent;
  switch (kind) {
  case TYPE_KIND_CHAR:
    t->bytes = 1;
    break;
  case TYPE_KIND_INT:
    t->bytes = 4;
    break;
  default:
    t->bytes = 8;
  }
  return t;
}

void type_free(type_t *t) {
  free(t->name);
  free(t);
}

type_t *type_get(parse_t *parse, char *name, type_t *parent) {
  map_entry_t *e = map_find(parse->types, name);
  type_t *type;
  if (e != NULL) {
    type = (type_t *)e->val;
    assert(type->parent == parent);
  } else if (parent != NULL) {
    type = type_new(name, TYPE_KIND_PTR, parent);
    map_add(parse->types, name, type);
  } else {
    type = NULL;
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
