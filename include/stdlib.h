// Copyright 2019 @htz. Released under the MIT license.

#ifndef __STDLIB_H
#define __STDLIB_H

#include <stddef.h>

extern double strtod (const char *__nptr, char **__endptr);
extern unsigned long int strtoul (const char *__nptr, char **__endptr, int __base);
extern void *malloc(size_t __size);
extern void *calloc(size_t __nmemb, size_t __size);
extern void *realloc(void *__ptr, size_t __size);
extern void free(void *__ptr);

#endif
