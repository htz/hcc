void expect(int a, int b);
void expect_string(char *a, char *b);
void expect_double(double a, double b);

#define ZERO 0
#define ONE 1
#define TWO ONE + ONE
#define THREE TWO + ONE
#define FOUR TWO + TWO
#define FIVE TWO + THREE
#define SIX THREE + THREE
#define SEVEN FOUR + THREE
#define EIGHT FIVE + THREE
#define NINE SEVEN + TWO
#define TEN SEVEN + TWO + ONE
#define AAA do {         \
  while (i < 100) {i++;} \
} while (0)

static void test_basic() {
  expect(1, ONE);
  expect(2, TWO);
  expect(3, THREE);
  expect(4, FOUR);
  expect(5, FIVE);
  expect(6, SIX);
  expect(7, SEVEN);
  expect(8, EIGHT);
  expect(9, NINE);
  expect(10, TEN);
  int i = 0;
  AAA;
  expect(100, i);
}

#define VAR1 VAR2
#define VAR2 VAR1

static void test_loop() {
  int VAR1 = 1;
  int VAR2 = 2;
  expect(1, VAR1);
  expect(2, VAR2);
}

static void test_undef() {
  int a = 3;
#define a 10
  expect(10, a);
#undef a
  expect(3, a);
#define a 16
  expect(16, a);
#undef a
}

static void test_function_like() {
#define MAX(a, b) ((a) > (b) ? (a) : (b))
  expect(10, MAX(10, 0));
  expect(10, MAX(0, 10));
  expect(10, MAX(1, MAX(2, MAX(3, MAX(4, MAX(5, MAX(6, MAX(7, MAX(8, MAX(9, 10))))))))));
}

static int plus(int a, int b) {
  return a + b;
}

static int minus(int a, int b) {
  return a - b;
}

static void test_funclike() {
#define stringify(x) #x
  expect_string("5", stringify(5));
  expect_string("x", stringify(x));
  expect_string("x y", stringify(x y));
  expect_string("x y", stringify( x y ));
  expect_string("x + y", stringify( x + y ));
  expect_string("x + y", stringify(/**/x/**/+/**//**/ /**/y/**/));
  expect_string("x+y", stringify( x+y ));
  expect_string("'a'", stringify('a'));
  expect_string("'\\''", stringify('\''));
  expect_string("\"abc\"", stringify("abc"));
  expect_string("ONE", stringify(ONE));
  expect_string("1 2", stringify(1
2));

#define m1(x) x
  expect(5, m1(5));
  expect(7, m1((5 + 2)));
  expect(8, m1(plus(5, 3)));
  expect(10, m1() 10);
  expect(14, m1(2 + 2 +) 10);

#define m2(x) x + x
  expect(10, m2(5));

#define m3(x, y) x * y
  expect(50, m3(5, 10));
  expect(11, m3(2 + 2, 3 + 3));

#define m4(x, y) x + y + TWO
  expect(17, m4(5, 10));

#define m6(x, ...) x + __VA_ARGS__
  expect(20, m6(2, 18));
  expect(25, plus(m6(2, 18, 5)));

#define plus(x, y) x * y + plus(x, y)
  expect(11, plus(2, 3));
#undef plus

#define plus(x, y)  minus(x, y)
#define minus(x, y) plus(x, y)
  expect(31, plus(30, 1));
  expect(29, minus(30, 1));

#define m7 (0) + 1
  expect(1, m7);

#define m8(x, y) x ## y
  expect(2, m8(TW, O));
  expect(0, m8(ZERO,));
  expect(8, 1 m8(<, <) 3);
  expect_double(.123, m8(., 123));

#define m9(x, y, z) x y + z
  expect(8, m9(1,, 7));

#define m10(x) x ## x
  expect_string("a", "a" m10());

#define hash_hash # ## #
#define mkstr(a) # a
#define in_between(a) mkstr(a)
#define join(c, d) in_between(c hash_hash d)
  expect_string("x ## y", join(x, y));

  int m14 = 67;
#define m14(x) x
  expect(67, m14);
  expect(67, m14(m14));

  int a = 68;
#define glue(x, y) x ## y
  glue(a+, +);
  expect(69, a);

#define identity(x) stringify(x)
  expect_string("aa A B aa C", identity(m10(a) A B m10(a) C));

#define identity2(x) stringify(z ## x)
  expect_string("zA m10(a) A B m10(a) C", identity2(A m10(a) A B m10(a) C));

#define m15(x) x x
  expect_string("a a", identity(m15(a)));

#define m16(x) (x,x)
  expect_string("(a,a)", identity(m16(a)));

#define m17(x) stringify(.x . x)
  expect_string(".3 . 3", m17(3));
}

static void test_empty() {
#define EMPTY
  expect(1, 1 EMPTY);
#define EMPTY2(x)
  expect(2, 2 EMPTY2(foo));
  expect(2, 2 EMPTY2(foo bar));
  expect(2, 2 EMPTY2(((()))));
}

static void test_noarg() {
#define NOARG() 55
  expect(55, NOARG());
}

static void test_null() {
  #
}

static void test_cond_incl() {
  int a = 1;
#if 0
  a = 2;
#endif
  expect(1, a);

#if 0
  fail("if 0");
xyz    /*
#else
abc    */
  fail("if 0");
#endif

/*
*/#if 0
  fail("if 0");
xyz "\"/*" '\'/*'
#else
  a = 5;
#endif
  expect(a, 5);

#if 0
#elif 1
  a = 2;
#endif
  expect(2, a);

#if 1
  a = 3;
#elif 1
  a = 4;
#endif
  expect(3, a);

#if 1
  a = 5;
#endif
  expect(5, a);

#if 1
  a = 10;
#else
  a = 12;
#endif
  expect(10, a);

#if 0
  a = 11;
#else
  a = 12;
#endif
  expect(12, a);

#if 0
# if 1
# endif
#else
  a = 150;
#endif
  expect(150, a);
}

static void test_defined() {
  int a = 0;
#if defined ZERO
  a = 1;
#endif
  expect(1, a);
#if defined(ZERO)
  a = 2;
#endif
  expect(2, a);
#if defined(NO_SUCH_MACRO)
  a = 3;
#else
  a = 4;
#endif
  expect(4, a);
}

static void test_ifdef() {
  int a = 0;
#ifdef ONE
  a = 1;
#else
  a = 2;
// #
// #1234
#endif
  expect(a, 1);

#ifdef NO_SUCH_MACRO
  a = 3;
#else
  a = 4;
#endif
  expect(a, 4);

#ifndef ONE
  a = 5;
#else
  a = 6;
#endif
  expect(a, 6);

#ifndef NO_SUCH_MACRO
  a = 7;
#else
  a = 8;
#endif
  expect(a, 7);
}

void testmain() {
  test_basic();
  test_loop();
  test_undef();
  test_function_like();
  test_funclike();
  test_empty();
  test_noarg();
  test_null();
  test_cond_incl();
  test_defined();
  test_ifdef();
}
