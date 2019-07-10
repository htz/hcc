#include "test/test.h"

static void test_basic_struct() {
  struct {
    signed char c;
    short s;
    int i;
    long l;
  } a1;
  a1.c = 0xa9;
  a1.s = 0xa987;
  a1.i = 0xa9876543;
  a1.l = 0xfedcba9876543210LL;
  expect(1, a1.c == 0xa9);
  expect(1, a1.s == 0xa987);
  expect(1, a1.i == 0xa9876543);
  expect(1, a1.l == 0xfedcba9876543210LL);

  struct {
    unsigned char c;
    unsigned short s;
    unsigned int i;
    unsigned long l;
  } a2;
  a1.c = 0xa9;
  a1.s = 0xa987;
  a1.i = 0xa9876543;
  a1.l = 0xfedcba9876543210LL;
  expect(1, a1.c == 0xa9);
  expect(1, a1.s == 0xa987);
  expect(1, a1.i == 0xa9876543);
  expect(1, a1.l == 0xfedcba9876543210LL);

  struct Tag1 {
    char x;
    int y;
  };
  struct Tag1 a3, a4;
  a3.x = 2;
  a3.y = 3;
  a4.x = 4;
  a4.y = 5;
  expect(2, a3.x);
  expect(3, a3.y);
  expect(4, a4.x);
  expect(5, a4.y);
}

static void test_basic_union() {
  union {
    int a;
    int b;
  } b1;
  b1.a = 6;
  expect(6, b1.b);
  b1.b = 7;
  expect(7, b1.a);

  union {
    char a[4];
    int b;
  } b2;
  b2.b = 0;
  b2.a[1] = 1;
  expect(256, b2.b);
}

static void test_nested() {
  struct {
    int a;
    struct {
      char b;
      int c;
    } y;
  } a1;
  a1.a = 1;
  a1.y.b = 2;
  a1.y.c = 3;
  expect(1, a1.a);
  expect(2, a1.y.b);
  expect(3, a1.y.c);

  struct {
    int a;
    struct {
      int b;
      struct {
        int c;
      };
    };
  } b1;
  b1.a = 4;
  b1.b = 5;
  b1.c = 6;
  expect(4, b1.a);
  expect(5, b1.b);
  expect(6, b1.c);
}

static void test_pointer() {
  struct Tag2 {
    int a;
  } x;
  struct Tag2 *p = &x;
  x.a = 1;
  expect(1, (*p).a);
  expect(1, p->a);
  (*p).a = 2;
  expect(2, x.a);
  p->a = 3;
  expect(3, x.a);
}

struct GS {
  int a;
  struct {
    char b;
    int c;
  } y;
} gs;

static void test_global_struct() {
  gs.a = 1;
  gs.y.b = 2;
  gs.y.c = 3;
  expect(1, gs.a);
  expect(2, gs.y.b);
  expect(3, gs.y.c);

  struct GS a;
  a.a = 1;
  a.y.b = 2;
  a.y.c = 3;
  expect(1, a.a);
  expect(2, a.y.b);
  expect(3, a.y.c);

  struct GS *p = &gs;
  gs.a = 4;
  expect(4, (*p).a);
  expect(4, p->a);
  (*p).a = 5;
  expect(5, gs.a);
  p->a = 6;
  expect(6, gs.a);
}

union GU {
  char a[4];
  int b;
} gu;

static void test_global_union() {
  gu.b = 0;
  gu.a[1] = 1;
  expect(256, gu.b);

  union GU a;
  a.b = 0;
  a.a[1] = 1;
  expect(256, a.b);
}

static void test_mix() {
  union {
    unsigned long long l;
    struct {
      unsigned int i[2];
    };
    struct {
      unsigned short s[4];
    };
    struct {
      unsigned char c[8];
    };
  } a;
  a.l = 0xfedcba9876543210ULL;
  expect(0xfedcba98, a.i[1]);
  expect(0x76543210, a.i[0]);
  expect(0xfedc, a.s[3]);
  expect(0xba98, a.s[2]);
  expect(0x7654, a.s[1]);
  expect(0x3210, a.s[0]);
  expect(0xfe, a.c[7]);
  expect(0xdc, a.c[6]);
  expect(0xba, a.c[5]);
  expect(0x98, a.c[4]);
  expect(0x76, a.c[3]);
  expect(0x54, a.c[2]);
  expect(0x32, a.c[1]);
  expect(0x10, a.c[0]);
}

struct Tag {
  int x;
};

static void test_scope() {
  struct Tag a;
  a.x = 0;
  struct Tag {
    int y;
  };
  struct Tag b;
  b.y = 1;
  expect(0, a.x);
  expect(1, b.y);
  {
    struct Tag c;
    c.y = 2;
    struct Tag {
      int z;
    };
    struct Tag d;
    d.z = 3;
    expect(2, c.y);
    expect(3, d.z);
  }
}

static void test_init() {
  struct {
    char c;
    short s;
    int i;
    long l;
  } a1 = {
    0xa9,
    0xa987,
    0xa9876543,
    0xfedcba9876543210LL,
  };
  expect(1, a1.c == 0xa9);
  expect(1, a1.s == 0xa987);
  expect(1, a1.i == 0xa9876543);
  expect(1, a1.l == 0xfedcba9876543210LL);

  struct {
    int a[4];
    struct {
      char x;
      long y;
    } b;
    struct {
      int z;
    };
  } a2 = {
    {1, 2, 3, 4},
    {5, 6},
    7,
  };
  expect(1, a2.a[0]);
  expect(2, a2.a[1]);
  expect(3, a2.a[2]);
  expect(4, a2.a[3]);
  expect(5, a2.b.x);
  expect(6, a2.b.y);
  expect(7, a2.z);
}

static void test_call_func(struct GS a) {
  expect(1, a.a);
  expect(2, a.y.b);
  expect(3, a.y.c);
  a.a = 0;
  a.y.b = 0;
  a.y.c = 0;
}

static void test_call() {
  struct GS a;
  a.a = 1;
  a.y.b = 2;
  a.y.c = 3;
  test_call_func(a);
  expect(1, a.a);
  expect(2, a.y.b);
  expect(3, a.y.c);

  gs.a = 1;
  gs.y.b = 2;
  gs.y.c = 3;
  test_call_func(gs);
  expect(1, gs.a);
  expect(2, gs.y.b);
  expect(3, gs.y.c);
}

static void test_assign() {
  struct GS a, b;
  a.a = 1;
  a.y.b = 2;
  a.y.c = 3;
  b = a;
  expect(1, b.a);
  expect(2, b.y.b);
  expect(3, b.y.c);

  gs = a;
  expect(1, gs.a);
  expect(2, gs.y.b);
  expect(3, gs.y.c);
}

void testmain() {
  test_basic_struct();
  test_basic_union();
  test_nested();
  test_pointer();
  test_global_struct();
  test_global_union();
  test_mix();
  test_scope();
  test_init();
  test_call();
  test_assign();
}
