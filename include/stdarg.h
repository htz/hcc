// Copyright 2019 @htz. Released under the MIT license.

#ifndef __STDARG_H
#define __STDARG_H

typedef __builtin_va_list va_list[1];
typedef va_list __gnuc_va_list;

#define va_start(ap, _) __builtin_va_start(ap)
#define va_end(ap)
#define __align(ptr, n) (ptr = (void *)(((unsigned long long)(ptr) + (n - 1)) & ~(n - 1)))
#define va_arg(ap, type) ( \
  *( \
    (type *)( \
      __typecode(type) == 12 ? \
        (__align(ap->overflow_arg_area, 8), ap->overflow_arg_area = (void *)((unsigned long long)ap->overflow_arg_area + sizeof(type)), (unsigned long long)ap->overflow_arg_area - sizeof(type)) : \
        (__typecode(type) == 7 || __typecode(type) == 8 || __typecode(type) == 9) ? \
          (ap->fp_offset == 112 ? \
            (__align(ap->overflow_arg_area, 8), ap->overflow_arg_area = (void *)((unsigned long long)ap->overflow_arg_area + 8), (unsigned long long)ap->overflow_arg_area - 8) : \
            (ap->fp_offset += 8, (unsigned long long)ap->reg_save_area + ap->fp_offset - 8) \
          ) : \
          (ap->gp_offset == 48 ? \
            (__align(ap->overflow_arg_area, 8), ap->overflow_arg_area = (void *)((unsigned long long)ap->overflow_arg_area + 8), (unsigned long long)ap->overflow_arg_area - 8) : \
            (ap->gp_offset += 8, (unsigned long long)ap->reg_save_area + ap->gp_offset - 8) \
          ) \
    ) \
  ) \
)

#endif
