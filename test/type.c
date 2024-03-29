#include "test/test.h"

static void test_type() {
  char a;
  short b;
  int c;
  long d;
  long long e;
  short int f;
  long int g;
  long long int h;
  long int long i;
  float j;
  double k;
  long double l;
  _Bool m;
  bool n;
}

static void test_typecode() {
  expect(0, __builtin_typecode(void));
  expect(1, __builtin_typecode(_Bool));
  expect(2, __builtin_typecode(char));
  expect(3, __builtin_typecode(short));
  expect(4, __builtin_typecode(int));
  expect(5, __builtin_typecode(long));
  expect(6, __builtin_typecode(long long));
  expect(7, __builtin_typecode(float));
  expect(8, __builtin_typecode(double));
  expect(9, __builtin_typecode(long double));
  expect(10, __builtin_typecode(void*));
  expect(11, __builtin_typecode(int[10]));
  expect(12, __builtin_typecode(struct{int a;}));
  // expect(13, __builtin_typecode(enum{a}));
  expect(14, __builtin_typecode(void ()()));
}

static void test_typecode_compare() {
  expect(1, __builtin_typecode_compare(void, void));
  expect(1, __builtin_typecode_compare(_Bool, _Bool));
  expect(1, __builtin_typecode_compare(char, char));
  expect(1, __builtin_typecode_compare(short, short));
  expect(1, __builtin_typecode_compare(int, int));
  expect(1, __builtin_typecode_compare(long, long));
  expect(1, __builtin_typecode_compare(long long, long long));
  expect(1, __builtin_typecode_compare(float, float));
  expect(1, __builtin_typecode_compare(double, double));
  expect(1, __builtin_typecode_compare(long double, long double));
  expect(1, __builtin_typecode_compare(void*, void*));
}

static void test_signed() {
  signed char a;
  signed short b;
  signed int c;
  signed long d;
  signed long long e;
  signed short int f;
  signed long int g;
  signed long long int h;
}

static void test_unsigned() {
  unsigned char a;
  unsigned short b;
  unsigned int c;
  unsigned long d;
  unsigned long long e;
  unsigned short int f;
  unsigned long int g;
  unsigned long long int h;
}

static void test_pointer() {
  int *a;
  expect(8, sizeof(a));
  int *b[5];
  expect(40, sizeof(b));
  int (*c)[5];
  expect(8, sizeof(c));
}

static void test_typedef() {
  typedef int integer;
  integer a = 5;
  expect(5, a);

  typedef int array[3];
  array b = { 1, 2, 3 };
  expect(2, b[1]);

  typedef struct tag { int x; } strtype;
  strtype c;
  c.x = 5;
  expect(5, c.x);

  typedef char x;
  {
    int x = 3;
    expect(3, x);
  }
  {
    int a = sizeof(x), x = 5, c = sizeof(x);
    expect(1, a);
    expect(5, x);
    expect(4, c);
  }
}

static int add(int x, int y) {
  return x + y;
}

static void test_func() {
  int (*calc1[1])(int,int) = {add};
  expect(3, (*calc1[0])(1, 2));

  typedef int (*func)(int,int);
  func calc2[1] = {add};
  expect(3, (*calc2[0])(1, 2));
}

extern int externvar1;
int extern externvar2;

static void test_extern() {
  expect(123, externvar1);
  expect(456, externvar2);
}

void testmain() {
  test_type();
  test_typecode();
  test_typecode_compare();
  test_signed();
  test_unsigned();
  test_pointer();
  test_typedef();
  test_func();
}
