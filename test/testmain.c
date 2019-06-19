#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern void testmain(void);

// For test/type.c
int externvar1 = 123;
int externvar2 = 456;

void expect(int a, int b) {
  if (a == b) {
    return;
  }
  printf("%d expected, but got %d\n", a, b);
  exit(1);
}

void expect_string(char *a, char *b) {
  if (strcmp(a, b) == 0) {
    return;
  }
  printf("\"%s\" expected, but got \"%s\"\n", a, b);
  exit(1);
}

void expect_float(float a, float b) {
  if (a == b) {
    return;
  }
  printf("%f expected, but got %f\n", a, b);
  exit(1);
}

void expect_double(double a, double b) {
  if (a == b) {
    return;
  }
  printf("%lf expected, but got %lf\n", a, b);
  exit(1);
}

int main() {
  testmain();
  printf("All tests passed\n");
  return 0;
}
