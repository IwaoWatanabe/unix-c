
#include "subcmd.h"
#include <cstdio>
#include <string>

using namespace std;

/// std::string の振る舞いの確認

static int string01(int argc, char **argv) {

  string aa("abc");
  string bb;

  bb += aa;
  bb += "def";

  printf("bb: %s: %d\n", bb.c_str(), bb.size());

  return 0;
}


subcmd stl_cmap[] = {
  { "str01", string01 },
  { 0 },
};


