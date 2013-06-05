
#include <cstdio>
#include "subcmd.h"

static int hello(int argc, char **argv) {
  printf("called by subcmd: %s\n", argv[0]);
  return 0;
}

static subcmd cmap[] = {
  { "hello", hello, },
  { "hello-xx", hello, },
  { 0 },
};

/// サブコマンドを使う

int main(int argc, char **argv) {
  subcmd_add(cmap);

  int rc = subcmd_run(argc, argv);
  return rc;
}

