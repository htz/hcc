// Copyright 2019 @htz. Released under the MIT license.

#include <stdlib.h>
#include <string.h>
#include "hcc.h"

#define VECTOR_MIN_SIZE 8

vector_t *vector_new() {
  vector_t *vector = (vector_t *)malloc(sizeof (vector_t));
  vector->size = 0;
  vector->capacity = VECTOR_MIN_SIZE;
  vector->data = malloc(sizeof (void *) * vector->capacity);
  return vector;
}

void vector_free(vector_t *vec) {
  free(vec->data);
  free(vec);
}

static int roundup(int n) {
  if (n == 0) {
    return 0;
  }
  int r = VECTOR_MIN_SIZE;
  while (n > r) {
    r <<= 1;
  }
  return r;
}

static void extend(vector_t *vec, int delta) {
  if (vec->size + delta <= vec->capacity) {
    return;
  }
  int capacity = max(roundup(vec->size + delta), VECTOR_MIN_SIZE);
  vec->data = realloc(vec->data, sizeof (void *) * capacity);
  vec->capacity = capacity;
}

void vector_push(vector_t *vec, void *d) {
  extend(vec, 1);
  vec->data[vec->size++] = d;
}

void *vector_pop(vector_t *vec) {
    return vec->data[--vec->size];
}

void *vector_dup(vector_t *vec) {
  vector_t *vector = (vector_t *)malloc(sizeof (vector_t));
  vector->size = vec->size;
  vector->capacity = vec->capacity;
  vector->data = malloc(sizeof (void *) * vec->capacity);
  memcpy(vector->data, vec->data, sizeof (void *) * vec->capacity);
  return vector;
}

bool vector_exists(vector_t *vec, void *d) {
  for (int i = 0; i < vec->size; i++) {
    if (vec->data[i] == d) {
      return true;
    }
  }
  return false;
}
