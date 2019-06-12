void expect(int a, int b);

void test_while() {
  int i = 0;
  while (i < 100) i++;
  expect(100, i);
}

void test_do_while() {
  int i = 0;
  do {
    i++;
  } while (i < 100);
  expect(100, i);
}

void test_for() {
  int i;
  for (i = 0; i < 100; i++) {
    i++;
  }
  expect(100, i);
}

void test_break() {
  int i = 0;
  while (i < 100) {
    i++;
    break;
  }
  expect(1, i);
}

void test_continue() {
  int i;
  int ans[10], count = 0;
  for (i = 0; i < 10; i++){
    if (i % 2 == 0) {
      continue;
    }
    ans[count++] = i;
  }
  expect(5, count);
  expect(1, ans[0]);
  expect(3, ans[1]);
  expect(5, ans[2]);
  expect(7, ans[3]);
  expect(9, ans[4]);
}

void testmain() {
  test_while();
  test_do_while();
  test_for();
  test_break();
  test_continue();
}
