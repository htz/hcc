// Copyright 2019 @htz. Released under the MIT license.

#include <errno.h>
#include <limits.h>
#include <string.h>
#include <unistd.h>
#include "hcc.h"

int min(int a, int b) {
  return a < b ? a : b;
}

int max(int a, int b) {
  return a > b ? a : b;
}

void align(int *np, int a) {
  int rem = *np % a;
  *np = (rem == 0) ? *np : *np - rem + a;
}

static void normalize_path(string_t *path) {
  assert(path->buf[0] == '/');
  char *p = path->buf, *q = p + 1;
  for (;;) {
    if (*p == '/') {
      p++;
    } else if (strncmp("./", p, 2) == 0) {
      p += 2;
    } else if (strncmp("../", p, 3) == 0) {
      p += 3;
      if (q >path->buf + 1) {
        for (q--; q[-1] != '/'; q--);
      }
    } else {
      while (*p != '/' && *p != '\0') {
        *q++ = *p++;
      }
      if (*p == '\0') {
        break;
      }
      *q++ = *p++;
    }
  }
  *q = '\0';
  path->size = q - path->buf;
}

string_t *fullpath(char *path) {
  string_t *ret = string_new();
  if (path[0] == '/') {
    string_append(ret, path);
  } else {
    char cwd[PATH_MAX];
    if (!getcwd(cwd, PATH_MAX)) {
      errorf("getcwd failed: %s", strerror(errno));
    }
    string_appendf(ret, "%s/%s", cwd, path);
  }
  normalize_path(ret);
  return ret;
}
