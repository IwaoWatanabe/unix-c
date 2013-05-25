
#include "subcmd.h"

static int temp01(int argc, char **argv) {
  return 0;
}

subcmd temp_cmap[] = {
  { "temp", temp01,  },
  { 0 },
};

