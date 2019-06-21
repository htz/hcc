// Copyright 2019 @htz. Released under the MIT license.

#include <stdlib.h>
#include "hcc.h"

macro_t *macro_new() {
  macro_t *m = (macro_t *)malloc(sizeof (macro_t));
  m->tokens = vector_new();
  return m;
}

void macro_free(macro_t *m) {
  vector_free(m->tokens);
  free(m);
}
