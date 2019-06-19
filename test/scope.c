void expect(int a, int b);

static int a = 1;
static void test_basic() {
  expect(1, a);
  {
    expect(1, a);
    int a = 11;
    expect(11, a);
    {
      expect(11, a);
      int a = 111;
      expect(111, a);
      {
        expect(111, a);
        int a = 1111;
        expect(1111, a);
        a = 2222;
        expect(2222, a);
      }
      a = 222;
      expect(222, a);
    }
    a = 22;
    expect(22, a);
  }
  a = 2;
  expect(2, a);
}

void testmain() {
  test_basic();
}
