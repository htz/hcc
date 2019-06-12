void expect(int a, int b);

void test_basic() {
  expect(1, 1 == (int)1.1);
  expect(1, 1.0 == (double)1);
  expect(1, 1.0f == (float)1);
  expect(1, 1.0f == (float)1.0);
  expect(1, 1.0 == (double)1.0f);
}

void testmain() {
  test_basic();
}
