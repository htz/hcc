// Copyright 2019 @htz. Released under the MIT license.

#ifndef __STDIO_H
#define __STDIO_H

#define EOF (-1)

typedef struct _IO_FILE FILE;

extern struct FILE *stdin;
extern struct FILE *stdout;
extern struct FILE *stderr;

extern int fprintf(FILE *__stream, const char *__format, ...);
extern int printf(const char *__format, ...);
extern int sprintf(char *__s, const char *__format, ...);

extern FILE *fopen(const char *__filename, const char *__modes);
extern int fclose(FILE *__stream);

#endif
