#define MAXVAL 10000

int flags[MAXVAL + 1];
int main() {
  int i, j, f, count = 2;
  int max = 100 + 1;
  for (i = 0; i <= MAXVAL; i++) flags[i] = 0;
  f = 4;
  for (i = 5; i <= max; i += (f = 6 - f)) {
    if (!flags[i]) {
      printf("%d,", i);
      ++count;
      for (j = i * 2; j <= MAXVAL; j += i) {
        flags[j] = 1;
      }
    }
  }
  for (; i <= MAXVAL; i += (f = 6 - f))
    if (!flags[i]) {
      printf("%d,", i);
      ++count;
    }
  printf("\ncount: %d\n", count);
  return 0;
}
