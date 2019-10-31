// Copyright 2019 @htz. Released under the MIT license.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hcc.h"

static map_t *alloc_map = NULL;

typedef struct alloc_info alloc_info_t;
struct alloc_info {
  char *file;
  int line;
  size_t size;
  void *ptr;
};

void *debug_malloc(const char *file, int line, size_t size) {
  alloc_info_t *info = (alloc_info_t *)malloc(sizeof (alloc_info_t));
  info->file = strdup(file);
  info->line = line;
  info->size = size;
  info->ptr = malloc(size);
  if (alloc_map == NULL) {
    alloc_map = map_new();
  }
  char key[20];
  sprintf(key, "%p", info->ptr);
  map_add(alloc_map, key, info);
  return info->ptr;
}

void *debug_realloc(const char *file, int line, void *ptr, size_t size) {
  char key[20];
  sprintf(key, "%p", ptr);
  alloc_info_t *info = (alloc_info_t *)map_get(alloc_map, key);

  if (info == NULL) {
    errorf("realloc: already freed pointer, or a non malloced pointer (file: %s, line: %d, ptr: %p)", file, line, ptr);
  }

  free(info->file);
  info->file = strdup(file);
  info->line = line;
  info->size = size;
  info->ptr = realloc(info->ptr, size);
  map_delete(alloc_map, key);
  sprintf(key, "%p", info->ptr);
  map_add(alloc_map, key, info);
  return info->ptr;
}

void debug_free(const char *file, int line, void *ptr) {
  char key[20];
  sprintf(key, "%p", ptr);
  map_entry_t *e = map_find(alloc_map, key);

  if (e == NULL) {
    errorf("free: already freed pointer, or a non malloced pointer (file: %s, line: %d, ptr: %p) [%s]", file, line, ptr, ((char *)ptr));
  }

  alloc_info_t *info = (alloc_info_t *)e->val;
  free(info->file);
  free(info->ptr);
  map_delete(alloc_map, key);
  free(info);
}

char *debug_strdup(const char *file, int line, char *str0) {
  int len = strlen(str0);
  char *str = (char *)debug_malloc(file, line, len + 1);
  strncpy(str, str0, len);
  str[len] = '\0';
  return str;
}

void debug_dump_memory() {
  for (map_entry_t *e = alloc_map->top; e != NULL; e = e->next) {
    alloc_info_t *info = (alloc_info_t *)e->val;
    fprintf(stderr, "%s:%d size=%lu, ptr=%p\n", info->file, info->line, info->size, info->ptr);
  }
}
