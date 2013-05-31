
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

/// サブコマンドを使う

int main(int argc, char **argv) {
  subcmd_add(cmap);

  extern subcmd stl_cmap[];
  subcmd_add(stl_cmap);

  extern subcmd term_cmap[];
  subcmd_add(term_cmap);

  extern subcmd temp_cmap[];
  subcmd_add(temp_cmap);

  extern subcmd csv_cmap[];
  subcmd_add(csv_cmap);

  extern subcmd xwin_cmap[];
  subcmd_add(xwin_cmap);

  extern subcmd awt_cmap[];
  subcmd_add(awt_cmap);

  extern subcmd motif_cmap[];
  subcmd_add(motif_cmap);

  extern subcmd mbs_cmap[];
  subcmd_add(mbs_cmap);

  extern subcmd time_cmap[];
  subcmd_add(time_cmap);

  extern subcmd logger_cmap[];
  subcmd_add(logger_cmap);

  extern subcmd ini_cmap[];
  subcmd_add(ini_cmap);

  int rc = subcmd_run(argc, argv);
  return rc;
}

