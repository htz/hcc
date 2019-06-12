void expect(int a, int b);

void test_primitives() {
  expect(1, sizeof(void));
  expect(1, sizeof(char));
  expect(2, sizeof(short));
  expect(4, sizeof(int));
  expect(8, sizeof(long));
}

void test_pointers() {
  expect(8, sizeof(char *));
  expect(8, sizeof(short *));
  expect(8, sizeof(int *));
  expect(8, sizeof(long *));
}

void test_unsigned() {
  expect(1, sizeof(unsigned char));
  expect(2, sizeof(unsigned short));
  expect(4, sizeof(unsigned int));
  expect(8, sizeof(unsigned long));
}

void test_literals() {
  expect(4, sizeof 1);
  expect(4, sizeof('a'));
  expect(4, sizeof(1.0f));
  expect(8, sizeof 1L);
  expect(8, sizeof 1.0);
  expect(8, sizeof(1.0));
}

void test_arrays() {
  expect(1, sizeof(char[1]));
  expect(7, sizeof(char[7]));
  expect(30, sizeof(char[3][10]));
  expect(32, sizeof(int[4][2]));
}

void test_vars() {
  char a[] = { 1, 2, 3 };
  expect(3, sizeof(a));
  char b[] = "abc";
  expect(4, sizeof(b));
  expect(1, sizeof(b[0]));
  expect(1, sizeof((b[0])));
  expect(1, sizeof((b)[0]));
  char *c[5];
  expect(40, sizeof(c));
  expect(4, sizeof((int)a));
}

void test_struct() {
  expect(1, sizeof(struct { char a; }));
  expect(3, sizeof(struct { char a[3]; }));
  expect(5, sizeof(struct { char a[5]; }));
  expect(8, sizeof(struct { int a; char b; }));
  expect(12, sizeof(struct { char a; int b; char c; }));
  expect(24, sizeof(struct { char a; double b; char c; }));
  expect(24, sizeof(struct { struct { char a; double b; } x; char c; }));
  expect(24, sizeof(union { char a[17]; double b[2];}));
}

void test_constexpr() {
  char a[sizeof(char[4])];
  expect(4, sizeof(a));
}

void testmain() {
  test_primitives();
  test_pointers();
  test_unsigned();
  test_literals();
  test_arrays();
  test_vars();
  test_struct();
  test_constexpr();
}
