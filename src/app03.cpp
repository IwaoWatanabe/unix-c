
#include <cstdio>
#include "subcmd.h"

static int hello(int argc, char **argv) {
  printf("called by subcmd.\n");
  return 0;
}

static subcmd cmap[] = {
  { "hello", hello, },
  { 0 },
};

extern subcmd stl_cmap[];
extern subcmd term_cmap[];
extern subcmd temp_cmap[];

/// サブコマンドを使う

int main(int argc, char **argv) {
  subcmd_add(cmap);
  subcmd_add(stl_cmap);
  subcmd_add(term_cmap);
  subcmd_add(temp_cmap);

  int rc = subcmd_run(argc, argv);
  return rc;
}

