void test_basic() {
  int a1[1] = {55};
  int *p1 = a1;
  expect(55, *p1);

  int a2[2] = {55, 67};
  int *p2 = a2 + 1;
  expect(67, *p2);

  int a3[3] = {20, 30, 40};
  int *p3 = a3 + 1;
  expect(30, *p3);

  int a4[3];
  *a4 = 10;
  *(a4 + 1) = 20;
  *(a4 + 2) = 30;
  expect(60, *a4 + *(a4 + 1) + *(a4 + 2));
}

void test_char_array() {
  char s[4] = "abc";
  char *c = s + 2;
  expect(99, *c);
}

void test_load() {
  int a[5] = {1, 2, 3, 4, 5};
  expect(15, a[0] + a[1] + a[2] + a[3] + a[4]);
}

void test_load_two_dimensional_array() {
  int a[2][3] = {{0, 1, 2}, {3, 4, 5}};
  expect(15, a[0][0] + a[0][1] + a[0][2] + a[1][0] + a[1][1] + a[1][2]);
}

void test_save_two_dimensional_array() {
  int a[2][3];
  a[0][1] = 1;
  a[1][1] = 2;
  int *p = a;
  expect(1, p[1]);
  expect(2, p[4]);
}

int g1[3];
int g2[3] = {1, 2, 3};

void test_global() {
  g1[0] = 1;
  g1[1] = 2;
  g1[2] = 3;
  expect(1, g1[0]);
  expect(2, g1[1]);
  expect(3, g1[2]);
  expect(1, g2[0]);
  expect(2, g2[1]);
  expect(3, g2[2]);
}

void testmain() {
  test_basic();
  test_char_array();
  test_load();
  test_load_two_dimensional_array();
  test_save_two_dimensional_array();
  test_global();
}
