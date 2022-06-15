// Copyright 2019 @htz. Released under the MIT license.

#ifndef __STDARG_H
#define __STDARG_H

typedef __builtin_va_list va_list[1];

#define va_start(ap, last) __builtin_va_start(ap)
// #define va_arg(ap, t)
#define va_end(ap) 0

#endif
