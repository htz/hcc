void expect(int a, int b);

static void test_goto() {
  int acc = 0;
  goto x;
  acc = 5;
x: expect(0, acc);

  int i = 0;
  acc = 0;
y: if (i > 10) goto z;
  acc += i++;
  goto y;
z: if (i > 11) goto a;
  expect(55, acc);
  i++;
  goto y;
a:
  ;
}

void testmain() {
  test_goto();
}
