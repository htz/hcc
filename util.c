// Copyright 2019 @htz. Released under the MIT license.

#include "hcc.h"

int max(int a, int b) {
  return a > b ? a : b;
}

void align(int *np, int a) {
  int rem = *np % a;
  *np = (rem == 0) ? *np : *np - rem + a;
}
