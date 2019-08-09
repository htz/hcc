// Copyright 2019 @htz. Released under the MIT license.

#ifndef __STDIO_H
#define __STDIO_H

#include <stddef.h>
#include <stdarg.h>

#define EOF (-1)

typedef struct _IO_FILE FILE;

extern struct FILE *stdin;
extern struct FILE *stdout;
extern struct FILE *stderr;

extern FILE *fopen(const char *__filename, const char *__modes);
extern int fclose(FILE *__stream);
extern int fprintf(FILE *__stream, const char *__format, ...);
extern int printf(const char *__format, ...);
extern int sprintf(char *__s, const char *__format, ...);
extern int vfprintf(FILE *__s, const char *__format, va_list __arg);
extern int vprintf(const char *__format, va_list __arg);
extern int vsprintf(char *__s, const char *__format, va_list __arg);
extern int vsnprintf(char *__s, size_t __maxlen, const char *__format, va_list __arg);
extern int fgetc(FILE *__stream);
extern int getc(FILE *__stream);
extern int getchar(void);
extern int fputc(int __c, FILE *__stream);
extern int putc(int __c, FILE *__stream);
extern int putchar(int __c);
extern char *gets(char *__s);
extern char *fgets(char *__s, int __n, FILE *__stream);
extern int fputs(const char *__s, FILE *__stream);
extern int puts(const char *__s);
extern int ungetc(int __c, FILE *__stream);
extern int fileno(FILE *__stream);

#endif
