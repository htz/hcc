void expect(int a, int b);

static void test_switch() {
  int a = 0;
  switch (1 + 2) {
  case 0: expect(1, -1);
  case 3: a = 3; break;
  case 1: expect(1, -1);
  }
  expect(a, 3);

  a = 0;
  switch (1) {
  case 0: a++;
  case 1: a++;
  case 2: a++;
  case 3: a++;
  }
  a = 3;

  a = 0;
  switch (100) {
  case 0: a++;
  default: a = 55;
  }
  expect(a, 55);

  a = 0;
  switch (100) {
  case 0: a++;
  }
  expect(a, 0);

  a = 5;
  switch (3) {
    a++;
  }
  expect(a, 5);

  a = 0;
  int count = 27;
  switch (count % 8) {
  case 0: do {  a++;
  case 7:       a++;
  case 6:       a++;
  case 5:       a++;
  case 4:       a++;
  case 3:       a++;
  case 2:       a++;
  case 1:       a++;
  } while ((count -= 8) > 0);
  }
  expect(27, a);

  switch (1)
    ;
}

void testmain() {
  test_switch();
}
