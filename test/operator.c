#include "test/test.h"

static void test_basic() {
  expect(3, 1 + 2);
  expect(3, 1 - -2);
  expect(10, 1 + 2 + 3 + 4);
  expect(11, 1 + 2 * 3 + 4);
  expect(14, 1 * 2 + 3 * 4);
  expect(4, 4 / 2 + 6 / 3);
  expect(3, 24 / 2 / 4);
  expect(13, (1 + 2) * 3 + 4);
  expect(1, (1 + 2 + 3 + 4) % 3);
  expect(64, 127 ^ 63);
  expect(4, 1 << 2);
  expect(31, 127 >> 2);
  expect(3, 1 | 2);
  expect(1, 1 & 3);
  expect(1, +1);
  expect(-2, -2);
}

static void test_inc_dec() {
  int a = 15;
  expect(15, a++);
  expect(16, a);
  expect(16, a--);
  expect(15, a);
}

static void test_logical() {
  expect(0, !1);
  expect(1 ,!0);
  expect(-11 ,~10);
  expect(1, 55 && 2);
  expect(0, 55 && 0);
  expect(0, 0 && 55);
  expect(0, 0 && 0);
  expect(1, 55 || 2);
  expect(1, 55 || 0);
  expect(1, 0 || 55);
  expect(0, 0 || 0);
}

static void test_ternary() {
  expect(51, (1 + 2) ? 51 : 52);
  expect(52, (1 - 1) ? 51 : 52);
  expect(26, (1 - 1) ? 51 : 52 / 2);
  expect(17, (1 - 0) ? 51 / 3 : 52);
}

static void test_comp() {
  expect(0, 1 == 2);
  expect(1, 1 == 1);
  expect(0, 2 == 1);
  expect(1, 1 != 2);
  expect(0, 1 != 1);
  expect(1, 2 != 1);
  expect(1, 1 < 2);
  expect(0, 1 < 1);
  expect(0, 2 < 1);
  expect(1, 1 <= 2);
  expect(1, 1 <= 1);
  expect(0, 2 <= 1);
  expect(1, -1 < 1);
  expect(0, -1 < -1);
  expect(0, 1 < -1);
  expect(1,-1 <= 1);
  expect(1,-1 <= 1);
  expect(0,1 <= -1);
  expect(0, 1 > 2);
  expect(0, 1 > 1);
  expect(1, 2 > 1);
  expect(0, 1 >= 2);
  expect(1, 1 >= 1);
  expect(1, 2 >= 1);
  expect(0, -1 > 1);
  expect(0, -1 > -1);
  expect(1, 1 > -1);
  expect(0, -1 >= 1);
  expect(1, -1 >= -1);
  expect(1, 1 >= -1);
}

static void test_assign() {
  int a1 = 1;
  a1 += 10;
  expect(11, a1);

  int a2 = 1;
  a2 -= 10;
  expect(-9, a2);

  int a3 = 1;
  a3 *= 10;
  expect(10, a3);

  int a4 = 11;
  a4 /= 5;
  expect(2, a4);

  int a5 = 11;
  a5 %= 3;
  expect(2, a5);

  int a6 = 127;
  a6 ^= 63;
  expect(64, a6);
}

void testmain() {
  test_basic();
  test_inc_dec();
  test_logical();
  test_ternary();
  test_comp();
  test_assign();
}
