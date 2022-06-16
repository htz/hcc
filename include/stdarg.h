// Copyright 2019 @htz. Released under the MIT license.

#ifndef __STDARG_H
#define __STDARG_H

typedef __builtin_va_list va_list[1];

#define va_start(ap, last) __builtin_va_start(ap)
#define va_arg(ap, type) \
    (*(type*)(__builtin_typecode_compare(type, double)                              \
      ? (ap->fp_offset < 128 + 48                                                   \
          ? (ap->fp_offset += 16,                                                   \
            (unsigned long long)ap->reg_save_area + ap->fp_offset - 16)             \
          : (ap->overflow_arg_area = (unsigned long long)ap->overflow_arg_area + 8, \
            (unsigned long long)ap->overflow_arg_area - 8))                         \
      : (ap->gp_offset < 48                                                         \
          ? (ap->gp_offset += 8,                                                    \
            (unsigned long long)ap->reg_save_area + ap->gp_offset - 8)              \
          : (ap->overflow_arg_area = (unsigned long long)ap->overflow_arg_area + 8, \
            (unsigned long long)ap->overflow_arg_area - 8))                         \
    ))
#define va_end(ap) 0

#endif
