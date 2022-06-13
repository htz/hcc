#include <stdarg.h>
#include <stdio.h>

void my_printf(char *fmt, int a1, ...) {
  va_list ap;
  va_start(ap, fmt);
  printf("&fmt = %p\n", &fmt);
  printf("&a1 = %p\n", &a1);
  printf("ap.gp_offset = %d\n", ap->gp_offset);
  printf("ap.fp_offset = %d\n", ap->fp_offset);
  printf("ap.overflow_arg_area = %p\n", ap->overflow_arg_area);
  printf("ap.reg_save_area = %p\n", ap->reg_save_area);
  // vprintf(fmt, ap);
  printf("*ap->reg_save_area = %d\n", *((int *)ap->reg_save_area));
  printf("*ap->reg_save_area = %d\n", *((int *)ap->reg_save_area + 1));
  printf("*ap->reg_save_area = %d\n", *((int *)ap->reg_save_area + 2));
  printf("*ap->reg_save_area = %d\n", *((int *)ap->reg_save_area + 3));
  printf("*ap->reg_save_area = %d\n", *((int *)ap->reg_save_area + 4));
  printf("*ap->reg_save_area = %d\n", *((int *)ap->reg_save_area + 5));
  printf("*ap->reg_save_area = %d\n", *((int *)ap->reg_save_area + 6));
}

int main() {
  my_printf("test = %d\n", 123, 456, 789);
  return 0;
}
