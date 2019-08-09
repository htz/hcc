// Copyright 2019 @htz. Released under the MIT license.

#ifndef __STRING_H
#define __STRING_H

#include <stddef.h>

extern void *memcpy(void *__dest, const void *__src, size_t __n);
extern void *memset(void *__s, int __c, size_t __n);
extern char *strcpy(char *__dest, const char *__src);
extern char *strncpy(char *__dest, const char *__src, size_t __n);
extern char *strcat(char *__dest, const char *__src);
extern char *strncat(char *__dest, const char *__src, size_t __n);
extern int strcmp(const char *__s1, const char *__s2);
extern int strncmp(const char *__s1, const char *__s2, size_t __n);
extern char *strdup(const char *__s);
extern size_t strlen(const char *__s);
extern char *strerror(int __errnum);

#endif
