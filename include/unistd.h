// Copyright 2019 @htz. Released under the MIT license.

#ifndef __UNISTD_H
#define __UNISTD_H

#include <stddef.h>

extern char *getcwd(char *__buf, size_t __size);
extern int isatty(int __fd);

#endif
