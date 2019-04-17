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

  node_t *node = parse_file(fp);
  if (fats) {
    node_debug(node);
  } else {
    gen(node);
  }
  node_free(node);

  return 0;
}
