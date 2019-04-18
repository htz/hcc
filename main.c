// Copyright 2019 @htz. Released under the MIT license.

#include <stdio.h>
#include <stdlib.h>
#include "hcc.h"

int main(int argc, char **argv) {
  char c;
  int fats = 0;
  FILE *fp = stdin;

  while (*++argv != NULL) {
    if (**argv == '-') {
      switch (c = *(*argv + 1)) {
      case 'a':
        fats = 1;
        break;
      default:
        errorf("unknown option %c\n", c);
      }
    } else {
      break;
    }
  }

  parse_t *parse = parse_file(fp);
  if (fats) {
    for (int i = 0; i < parse->statements->size; i++) {
      node_t *node = NULL;
      int j;
      for (j = i; j < parse->statements->size; j++) {
        node_t *n = (node_t *)parse->statements->data[j];
        if (n->kind != NODE_KIND_NOP) {
          node = n;
          break;
        }
      }
      if (node == NULL) {
        break;
      }
      if (i > 0) {
        printf(";");
      }
      i = j;
      node_debug(node);
    }
  } else {
    gen(parse);
  }
  parse_free(parse);

  return 0;
}
