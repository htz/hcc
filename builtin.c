// Copyright 2019 @htz. Released under the MIT license.

#include "hcc.h"

/*
 * Define builtin `struct __builtin_va_list` type
 *
 * struct __builtin_va_list {
 *   unsigned int gp_offset;
 *   unsigned int fp_offset;
 *   void *overflow_arg_area;
 *   void *reg_save_area;
 * };
 * typedef struct __builtin_va_list __builtin_va_list;
 */
static void init_builtin_va_list(parse_t *parse) {
  type_t *type = parse_make_empty_struct_type(parse, "__builtin_va_list", true);

  node_t *gp_offset_field = node_new_variable(parse, parse->type_uint, "gp_offset", STORAGE_CLASS_NONE, false);
  align(&type->total_size, gp_offset_field->type->align);
  gp_offset_field->voffset = type->total_size;
  type->total_size += gp_offset_field->type->total_size;
  map_add(type->fields, gp_offset_field->vname, gp_offset_field);

  node_t *fp_offset_field = node_new_variable(parse, parse->type_uint, "fp_offset", STORAGE_CLASS_NONE, false);
  align(&type->total_size, fp_offset_field->type->align);
  fp_offset_field->voffset = type->total_size;
  type->total_size += fp_offset_field->type->total_size;
  map_add(type->fields, fp_offset_field->vname, fp_offset_field);

  node_t *overflow_arg_area_field = node_new_variable(parse, parse->type_voidp, "overflow_arg_area", STORAGE_CLASS_NONE, false);
  align(&type->total_size, overflow_arg_area_field->type->align);
  overflow_arg_area_field->voffset = type->total_size;
  type->total_size += overflow_arg_area_field->type->total_size;
  map_add(type->fields, overflow_arg_area_field->vname, overflow_arg_area_field);

  node_t *reg_save_area_field = node_new_variable(parse, parse->type_voidp, "reg_save_area", STORAGE_CLASS_NONE, false);
  align(&type->total_size, reg_save_area_field->type->align);
  reg_save_area_field->voffset = type->total_size;
  type->total_size += reg_save_area_field->type->total_size;
  map_add(type->fields, reg_save_area_field->vname, reg_save_area_field);

  type->align = parse->type_voidp->align;
  align(&type->total_size, type->align);

  parse->type_va_list = type_add_typedef(parse, "__builtin_va_list", type);
  parse->type_va_listp = type_get_ptr(parse, parse->type_va_list);
}

// Declarate `void `__builtin_va_start(struct __builtin_va_list *);``
static void init_builtin_va_start(parse_t *parse) {
  vector_t *argtypes = vector_new();
  vector_push(argtypes, parse->type_va_listp);

  type_t *type = type_get_function(parse, parse->type_void, argtypes, false);
  vector_free(argtypes);

  node_t *var = node_new_variable(parse, type, "__builtin_va_start", STORAGE_CLASS_NONE, false);
  map_add(parse->vars, var->vname, var);
}

void builtin_init(parse_t *parse) {
  init_builtin_va_list(parse);
  init_builtin_va_start(parse);
}
