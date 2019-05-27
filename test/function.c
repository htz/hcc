int t1() {
  return 77;
}

int t2(int a) {
  expect(79, a);
}

int t3(int a, int b, int c, int d, int e, int f) {
  expect(1, a);
  expect(2, b);
  expect(3, c);
  expect(4, d);
  expect(5, e);
  expect(6, f);
}

int t4a(int *p) {
  return *p;
}

int t4(void) {
  int a[] = { 98 };
  expect(98, t4a(a));
}

int t5a(int *p) {
  expect(99, *p++);
  expect(98, *p++);
  expect(97, *p++);
}

// int t5b(int p[]) {
//   expect(99, *p++);
//   expect(98, *p++);
//   expect(97, *p++);
// }

int t5() {
  int a[] = {1, 2, 3};
  int *p = a;
  *p = 99; p++;
  *p = 98; p++;
  *p = 97;
  t5a(a);
  // t5b(a);
}

int t6() {
  return 3;
}

int t7(int a, int b) {
  return a * b;
}

int t8(char *str) {
  return strcmp("test", str);
}

int t9(int i1, double d1, int i2, double d2, int i3, double d3, int i4, double d4, int i5, double d5, int i6, double d6, int i7, double d7, int i8, double d8, int i9, double d9) {
  int sumi = i1 + i2 + i3 + i4 + i5 + i6 + i7 + i8 + i9;
  double sumd = d1 + d2 + d3 + d4 + d5 + d6 + d7 + d8 + d9;
  return (int)(sumi + sumd);
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
}
