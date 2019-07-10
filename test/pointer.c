#include "test/test.h"

static void test_basic() {
  int a = 61;
  int *p = &a;
  expect(61, *p);

  char *s1 = "ab";
  expect(97, *s1);

  char *s2 = "ab" + 1;
  expect(98, *s2);
}

int g = 62;
static void test_global() {
  int *p = &g;
  expect(62, *p);
}

static void test_char() {
  {
    signed char a[3] = {0xa0, 0xa1, 0xa2};
    signed char *p = a;
    expect(-96, *p++);
    expect(-95, *p++);
    expect(-94, *p++);
  }
  {
    unsigned char a[3] = {0xa0, 0xa1, 0xa2};
    unsigned char *p = a;
    expect(0xa0, *p++);
    expect(0xa1, *p++);
    expect(0xa2, *p++);
  }
}

static void test_short() {
  {
    short a[3] = {0xabc0, 0xabc1, 0xabc2};
    short *p = a;
    expect(0xabc0, *p++);
    expect(0xabc1, *p++);
    expect(0xabc2, *p++);
  }
  {
    unsigned short a[3] = {0xabc0, 0xabc1, 0xabc2};
    unsigned short *p = a;
    expect(0xabc0, *p++);
    expect(0xabc1, *p++);
    expect(0xabc2, *p++);
  }
}

static void test_int() {
  {
    int a[3] = {0xabcdef10, 0xabcdef11, 0xabcdef12};
    int *p = a;
    expect(0xabcdef10, *p++);
    expect(0xabcdef11, *p++);
    expect(0xabcdef12, *p++);
  }
  {
    unsigned int a[3] = {0xabcdef10, 0xabcdef11, 0xabcdef12};
    unsigned *p = a;
    expect(0xabcdef10, *p++);
    expect(0xabcdef11, *p++);
    expect(0xabcdef12, *p++);
  }
}

static void test_long() {
  {
    long a[3] = {0xfedcba9876543210L, 0xfedcba9876543211L, 0xfedcba9876543212L};
    long *p = a;
    expect(0xfedcba9876543210L, *p++);
    expect(0xfedcba9876543211L, *p++);
    expect(0xfedcba9876543212L, *p++);
  }
  {
    unsigned long a[3] = {0xfedcba9876543210L, 0xfedcba9876543211L, 0xfedcba9876543212L};
    unsigned long *p = a;
    expect(0xfedcba9876543210L, *p++);
    expect(0xfedcba9876543211L, *p++);
    expect(0xfedcba9876543212L, *p++);
  }
}

static void test_llong() {
  {
    long long a[3] = {0xfedcba9876543210LL, 0xfedcba9876543211LL, 0xfedcba9876543212LL};
    long long *p = a;
    expect(0xfedcba9876543210LL, *p++);
    expect(0xfedcba9876543211LL, *p++);
    expect(0xfedcba9876543212LL, *p++);
  }
  {
    unsigned long long a[3] = {0xfedcba9876543210LL, 0xfedcba9876543211LL, 0xfedcba9876543212LL};
    unsigned long long *p = a;
    expect(0xfedcba9876543210LL, *p++);
    expect(0xfedcba9876543211LL, *p++);
    expect(0xfedcba9876543212LL, *p++);
  }
}

void testmain() {
  test_basic();
  test_global();
  test_char();
}
