
#ifndef _subcmd_h
#define _subcmd_h

#include <cstdlib>

struct subcmd {
  const char *cmd;
  int (*func)(int argc, char **argv);
};

extern void subcmd_add(subcmd *ptr);
extern int subcmd_run(int argc, char **argv, void (*usage)(const char *cmd) = 0);

#endif

