// Copyright 2019 @htz. Released under the MIT license.

#include <stdlib.h>
#include <string.h>
#include "hcc.h"

#define MAP_MIN_SLOT_SIZE_BIT        8
#define MAP_THRESHOLD                0.75
#define MAP_DEFAULT_ENTRY_SIZE_INDEX 10
#define MAP_NEXT(map, i)             (i + map->inc) & (map->slot_size - 1)
#define MAP_ENTRY_IS_EMPTY(entry)    ((entry)->flag == 0)
#define MAP_ENTRY_IS_EXIST(entry)    ((entry)->flag & 0x01)
#define MAP_ENTRY_IS_DELETE(entry)   ((entry)->flag & 0x02)
#define MAP_ENTRY_TO_EMPTY(entry)    (entry)->flag = 0
#define MAP_ENTRY_TO_EXIST(entry)    (entry)->flag = 0x01
#define MAP_ENTRY_TO_DELETE(entry)   (entry)->flag = 0x02

static map_t *map_new_with(int slot_size_bit) {
  map_t *map = (map_t *)malloc(sizeof (map_t));
  map->size = 0;
  map->occupied = 0;
  map->slot_size_bit = slot_size_bit;
  map->slot_size = 1 << (slot_size_bit - 1);
  map->slot = (map_entry_t *)malloc(sizeof (map_entry_t) * map->slot_size);
  memset(map->slot, 0, sizeof(map_entry_t) * map->slot_size);
  map->inc = map->slot_size / 2 - 1;
  map->free_val_fn = NULL;
  map->top = NULL;
  map->bottom = NULL;
  return map;
}

map_t *map_new() {
  return map_new_with(MAP_MIN_SLOT_SIZE_BIT);
}

static int map_comp_str(char *str1, char *str2) {
  if (str1 == str2) {
    return 0;
  }
  if (str1 == 0) {
    return -1;
  }
  if (str2 == 0) {
    return 1;
  }

  return strcmp((char *)str1, (char *)str2);
}

static int map_calc_str(map_t *map, char *str) {
  int sum = 0;
  char *p = (char *)str;

  for (; *p; p++) {
    sum = (sum << 5) - sum + *p;
  }

  return sum & (map->slot_size - 1);
}

map_entry_t *map_find(map_t *map, void *key) {
  map_entry_t *entry;
  int i = map_calc_str(map, key);

  for (entry = map->slot + i; !MAP_ENTRY_IS_EMPTY(entry); entry = map->slot + i) {
    if (!MAP_ENTRY_IS_DELETE(entry)) {
      if (map_comp_str(key, entry->key) == 0) {
        return entry;
      }
    }
    i = MAP_NEXT(map, i);
  }

  return NULL;
}

void map_free(map_t *map) {
  map_clear(map);
  free(map->slot);
  free(map);
}

static void clear_entry(map_t *map, map_entry_t *entry) {
  free(entry->key);
  if (map->free_val_fn) {
    (*map->free_val_fn)(entry->val);
  }
  MAP_ENTRY_TO_EMPTY(entry);
  entry->prev = entry->next = NULL;
}

void map_clear(map_t *map) {
  map_entry_t *entry = NULL;
  int i;

  for (i = 0, entry = map->slot; i < map->slot_size; i++, entry++) {
    if (MAP_ENTRY_IS_EXIST(entry)) {
      clear_entry(map, entry);
    } else if (MAP_ENTRY_IS_DELETE(entry)) {
      MAP_ENTRY_TO_EMPTY(entry);
    }
  }
  map->size = 0;
  map->occupied = 0;
  map->top = NULL;
  map->bottom = NULL;
}

static void rehash(map_t *map) {
  map_entry_t *old_entry = map->slot, *old_top = map->top;

  map->slot_size_bit++;
  map->slot_size = 1 << (map->slot_size_bit - 1);
  map->slot = (map_entry_t *)malloc(sizeof(map_entry_t) * map->slot_size);
  memset(map->slot, 0, sizeof(map_entry_t) * map->slot_size);
  map->occupied = 0;
  map->inc = map->slot_size / 2 - 1;
  map->top = NULL;
  map->bottom = NULL;

  for (map_entry_t *entry = old_top; entry != NULL; entry = entry->next) {
    int i = map_calc_str(map, entry->key);
    map_entry_t *e;
    for (e = map->slot + i; MAP_ENTRY_IS_EXIST(e); e = map->slot + i) {
      i = MAP_NEXT(map, i);
    }
    e->key = entry->key;
    e->val = entry->val;
    if (MAP_ENTRY_IS_EMPTY(e)) {
      map->occupied++;
    }
    MAP_ENTRY_TO_EXIST(e);
    e->next = NULL;
    e->prev = map->bottom;
    if (map->bottom != NULL) {
      map->bottom->next = e;
      map->bottom = e;
    } else {
      map->top = map->bottom = e;
    }
  }
  free(old_entry);
}

void *map_get(map_t *map, char *key) {
  map_entry_t *entry = map_find(map, key);

  if (!entry) {
    return NULL;
  }

  return entry->val;
}

void map_add(map_t *map, char *key, void *val) {
  if ((double)map->occupied / map->slot_size > MAP_THRESHOLD) {
    rehash(map);
  }

  map_entry_t *entry;
  int i = map_calc_str(map, key);

  for (entry = map->slot + i; MAP_ENTRY_IS_EXIST(entry); entry = map->slot + i) {
    if (map_comp_str(key, entry->key) == 0) {
      if (map->free_val_fn) {
        (*map->free_val_fn)(entry->val);
      }
      entry->val = val;
      return;
    }
    i = MAP_NEXT(map, i);
  }
  entry->key = strdup(key);
  entry->val = val;
  map->size++;
  if (MAP_ENTRY_IS_EMPTY(entry)) {
    map->occupied++;
  }
  MAP_ENTRY_TO_EXIST(entry);
  entry->next = NULL;
  entry->prev = map->bottom;
  if (map->bottom != NULL) {
    map->bottom->next = entry;
    map->bottom = entry;
  } else {
    map->top = map->bottom = entry;
  }
}

static void map_delete_entry(map_t *map, map_entry_t *entry) {
  free(entry->key);
  if (map->free_val_fn) {
    (*map->free_val_fn)(entry->val);
  }
  MAP_ENTRY_TO_DELETE(entry);
  map->size--;
  if (entry->prev != NULL) {
    entry->prev->next = entry->next;
  }
  if (entry->next != NULL) {
    entry->next->prev = entry->prev;
  }
  if (map->top == entry) {
    map->top = entry->next;
  }
  if (map->bottom == entry) {
    map->bottom = entry->prev;
  }
}

int map_delete(map_t *map, char *key) {
  map_entry_t *entry = map_find(map, key);

  if (entry) {
    map_delete_entry(map, entry);
    return 1;
  }

  return 0;
}
