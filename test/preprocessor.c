void expect(int a, int b);

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

void testmain() {
  test_basic();
  test_loop();
  test_undef();
}
