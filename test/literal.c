void test_char_literal() {
  expect(65, 'A');
  expect(97, 'a');
  expect(7, '\a');
  expect(8, '\b');
  expect(12, '\f');
  expect(10, '\n');
  expect(13, '\r');
  expect(9, '\t');
  expect(11, '\v');
}

void test_int_literal() {
  expect(0, 0);
  expect(0, 0u);
  expect(0, 0l);
  expect(0, 0ul);
  expect(0, 0ll);
  expect(0, 0ull);
  expect(16, 16);
  expect(22, 0x16);
  expect(23, 0X17);
  expect(15, 017);
}

void test_string_literal() {
  expect_string("abc", "abc");
  expect('a', "abc"[0]);
  expect(0, "abc"[3]);
}

void test_float_literal() {
  expect_float(1.25f, 1.25f);
  expect_double(2.25, 2.25);
  expect_double(0.123, .123);
}

void test_unsigned() {
  int s1 = 1, s2 = 2, sm1 = -1;
  unsigned int u1 = 1, u2 = 2, um1 = -1;

  expect(0, s1 == u2);
  expect(1, s1 == u1);
  expect(0, s2 == u1);
  expect(1, s1 != u2);
  expect(0, s1 != u1);
  expect(1, s2 != u1);
  expect(1, s1 < u2);
  expect(0, s1 < u1);
  expect(0, s2 < u1);
  expect(1, s1 <= u2);
  expect(1, s1 <= u1);
  expect(0, s2 <= u1);
  expect(0, sm1 < u1);
  expect(0, s1 < u1);
  expect(0, sm1 < um1);
  expect(1, s1 < um1);
  expect(0, sm1 <= u1);
  expect(1, s1 <= u1);
  expect(1, sm1 <= um1);
  expect(1, s1 <= um1);
  expect(0, s1 > u2);
  expect(0, s1 > u1);
  expect(1, s2 > u1);
  expect(0, s1 >= u2);
  expect(1, s1 >= u1);
  expect(1, s2 >= u1);
  expect(1, sm1 > u1);
  expect(0, s1 > u1);
  expect(0, sm1 > um1);
  expect(0, s1 > um1);
  expect(1, sm1 >= u1);
  expect(1, s1 >= u1);
  expect(1, sm1 >= um1);
  expect(0, s1 >= um1);
}

void test_char_limit() {
  char s1 = 0x7f, s2 = 0xff;
  expect(1, s1 >= 0);
  expect(0, s2 > 0);
  s1++;
  s2++;
  expect(0, s1 >= 0);
  expect(1, s2 == 0);

  unsigned char u1 = 0x7f, u2 = 0xff;
  expect(1, u1 >= 0);
  expect(1, u2 > 0);
  u1++;
  u2++;
  expect(1, u1 >= 0);
  expect(1, u2 == 0);
}

void test_short_limit() {
  short s1 = 0x7fff, s2 = 0xffff;
  expect(1, s1 >= 0);
  expect(0, s2 > 0);
  s1++;
  s2++;
  expect(0, s1 >= 0);
  expect(1, s2 == 0);

  unsigned short u1 = 0x7fff, u2 = 0xffff;
  expect(1, u1 >= 0);
  expect(1, u2 > 0);
  u1++;
  u2++;
  expect(1, u1 >= 0);
  expect(1, u2 == 0);
}

void test_int_limit() {
  int s1 = 0x7fffffff, s2 = 0xffffffff;
  expect(1, s1 >= 0);
  expect(0, s2 > 0);
  s1++;
  s2++;
  expect(0, s1 >= 0);
  expect(1, s2 == 0);

  unsigned int u1 = 0x7fffffff, u2 = 0xffffffff;
  expect(1, u1 >= 0);
  expect(1, u2 > 0);
  u1++;
  u2++;
  expect(1, u1 >= 0);
  expect(1, u2 == 0);
}

void test_long_limit() {
  long s1 = 0x7fffffffffffffffL, s2 = 0xffffffffffffffffL;
  expect(1, s1 >= 0);
  expect(0, s2 > 0);
  s1++;
  s2++;
  expect(0, s1 >= 0);
  expect(1, s2 == 0);

  unsigned long u1 = 0x7fffffffffffffffUL, u2 = 0xffffffffffffffffUL;
  expect(1, u1 >= 0);
  expect(1, u2 > 0);
  u1++;
  u2++;
  expect(1, u1 >= 0);
  expect(1, u2 == 0);
}

void test_llong_limit() {
  long long s1 = 0x7fffffffffffffffLL, s2 = 0xffffffffffffffffLL;
  expect(1, s1 >= 0);
  expect(0, s2 > 0);
  s1++;
  s2++;
  expect(0, s1 >= 0);
  expect(1, s2 == 0);

  unsigned long long u1 = 0x7fffffffffffffffULL, u2 = 0xffffffffffffffffULL;
  expect(1, u1 >= 0);
  expect(1, u2 > 0);
  u1++;
  u2++;
  expect(1, u1 >= 0);
  expect(1, u2 == 0);
}

void testmain() {
  test_char_literal();
  test_int_literal();
  test_string_literal();
  test_float_literal();
  test_unsigned();
  test_char_limit();
  test_short_limit();
  test_int_limit();
  test_long_limit();
  test_llong_limit();
}
