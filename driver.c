// Copyright 2019 @htz. Released under the MIT license.

#include <stdio.h>

extern int mymain(void);

int sum2(int a, int b) {
  return a + b;
}

int sum10(int a, int b, int c, int d, int e, int f, int g, int h, int i, int j) {
  return a + b + c + d + e + f + g + h + i + j;
}

int main(int argc, char **argv) {
  printf("%d\n", mymain());
  return 0;
}
