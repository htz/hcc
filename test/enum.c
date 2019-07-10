#include "test/test.h"

static void test_basic() {
  enum {
    a = 1,
    b,
    c = 100,
    d
  };
  expect(1, a);
  expect(2, b);
  expect(100, c);
  expect(101, d);

  enum {
    e = 123
  } x = e;
  expect(e, x);
  expect(123, x);
}

enum GE {
  z = 123
};

static void test_global() {
  enum GE x = z;
  expect(124, x + 1);
}

void testmain() {
  test_basic();
}
