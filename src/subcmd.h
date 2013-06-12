
#ifndef _subcmd_h
#define _subcmd_h

struct subcmd {
  const char *cmd;
  int (*func)(int argc, char **argv);
};

#ifdef __cplusplus
extern "C" {
#endif

extern void subcmd_add(struct subcmd *ptr);
extern int subcmd_run(int argc, char **argv);
extern int subcmd_run_with_usage(int argc, char **argv, void (*usage)(const char *cmd));

#ifdef __cplusplus
};
#endif

#endif

