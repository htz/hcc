void test_type() {
  char a;
  short b;
  int c;
  long d;
  long long e;
  short int f;
  long int g;
  long long int h;
  long int long i;
  float j;
  double k;
  long double l;
}

void test_signed() {
  signed char a;
  signed short b;
  signed int c;
  signed long d;
  signed long long e;
  signed short int f;
  signed long int g;
  signed long long int h;
}

void test_unsigned() {
  unsigned char a;
  unsigned short b;
  unsigned int c;
  unsigned long d;
  unsigned long long e;
  unsigned short int f;
  unsigned long int g;
  unsigned long long int h;
}

void test_pointer() {
  int *a;
  expect(8, sizeof(a));
  int *b[5];
  expect(40, sizeof(b));
  // int (*c)[5];
  // expect(8, sizeof(c));
}

void testmain() {
  test_type();
  test_signed();
  test_unsigned();
  test_pointer();
}
