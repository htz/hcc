int testif1() { if (1) { return 'a';} return 0; }
int testif2() { if (0) { return 0;} return 'b'; }
int testif3() { if (1) { return 'c';} else { return 0; } return 0; }
int testif4() { if (0) { return 0;} else { return 'd'; } return 0; }
int testif5() { if (1) return 'e'; return 0; }
int testif6() { if (0) return 0; return 'f'; }
int testif7() { if (1) return 'g'; else return 0; return 0; }
int testif8() { if (0) return 0; else return 'h'; return 0; }
int testif9() { if (0+1) return 'i'; return 0; }
int testif10() { if (1-1) return 0; return 'j'; }

void test_basic() {
  expect('a', testif1());
  expect('b', testif2());
  expect('c', testif3());
  expect('d', testif4());
  expect('e', testif5());
  expect('f', testif6());
  expect('g', testif7());
  expect('h', testif8());
  expect('i', testif9());
  expect('j', testif10());
}

void testmain() {
  test_basic();
}
