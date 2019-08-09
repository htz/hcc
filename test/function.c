#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "test/test.h"

static int t1() {
  return 77;
}

static int t2(int a) {
  expect(79, a);
}

static int t3(int a, int b, int c, int d, int e, int f) {
  expect(1, a);
  expect(2, b);
  expect(3, c);
  expect(4, d);
  expect(5, e);
  expect(6, f);
}

static int t4a(int *p) {
  return *p;
}

static int t4(void) {
  int a[] = { 98 };
  expect(98, t4a(a));
}

static int t5a(int *p) {
  expect(99, *p++);
  expect(98, *p++);
  expect(97, *p++);
}

// static int t5b(int p[]) {
//   expect(99, *p++);
//   expect(98, *p++);
//   expect(97, *p++);
// }

static int t5() {
  int a[] = {1, 2, 3};
  int *p = a;
  *p = 99; p++;
  *p = 98; p++;
  *p = 97;
  t5a(a);
  // t5b(a);
}

static int t6() {
  return 3;
}

static int t7(int a, int b) {
  return a * b;
}

static int t8(char *str) {
  return strcmp("test", str);
}

static int t9(int i1, double d1, int i2, double d2, int i3, double d3, int i4, double d4, int i5, double d5, int i6, double d6, int i7, double d7, int i8, double d8, int i9, double d9) {
  int sumi = i1 + i2 + i3 + i4 + i5 + i6 + i7 + i8 + i9;
  double sumd = d1 + d2 + d3 + d4 + d5 + d6 + d7 + d8 + d9;
  return (int)(sumi + sumd);
}

static int t10(int, int, int);
static int t10(int a, int, int c) {
  expect(1, a);
  expect(3, c);
}

static void test_int(int a, ...) {
  va_list ap;
  va_start(ap, a);
  expect(1, a);
  expect(2, va_arg(ap, int));
  expect(3, va_arg(ap, int));
  expect(5, va_arg(ap, int));
  expect(8, va_arg(ap, int));
  va_end(ap);
}

static void test_float(float a, ...) {
  va_list ap;
  va_start(ap, a);
  expect_float(1.0, a);
  expect_double(2.0, va_arg(ap, double));
  expect_double(4.0, va_arg(ap, double));
  expect_double(8.0, va_arg(ap, double));
  va_end(ap);
}

static void test_mix(char *p, ...) {
  va_list ap;
  va_start(ap, p);
  expect_string("abc", p);
  expect_double(2.0, va_arg(ap, double));
  expect(4, va_arg(ap, int));
  expect_string("d", va_arg(ap, char *));
  expect(5, va_arg(ap, int));
  va_end(ap);
}

char *fmt(char *fmt, ...) {
  static char buf[100];
  va_list ap;
  va_start(ap, fmt);
  vsprintf(buf, fmt, ap);
  va_end(ap);
  return buf;
}

static void test_va_list() {
  expect_string("", fmt(""));
  expect_string("1,2,3,4", fmt("%d,%d,%d,%d", 1, 2, 3, 4));
  // expect_string("3,1.0,6,2.0,abc", fmt("%d,%.1f,%d,%.1f,%s", 3, 1.0, 6, 2.0, "abc"));
}

void testmain() {
  expect(77, t1());
  t2(79);
  t3(1, 2, 3, 4, 5, 6);
  t4();
  t5();
  expect(3, t6());
  expect(12, t7(3, 4));
  expect(0, t8("test"));
  expect(94, t9(1, 1.23, 2, 2.34, 3, 3.45, 4, 4.56, 5, 5.67, 6, 6.78, 7, 7.89, 8, 8.90, 9, 9.01));
  t10(1, 2, 3);

  test_int(1, 2, 3, 5, 8);
  test_float(1.0, 2.0, 4.0, 8.0);
  test_mix("abc", 2.0, 4, "d", 5);
  test_va_list();
}
