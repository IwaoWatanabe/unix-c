
#include "subcmd.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>


/// コマンド数を検出
static int subcmd_len(subcmd *ptr) {
  int len = 0;
  while (ptr->cmd) { len++; ptr++; }
  return len;
}

static subcmd **cmap_ptr;
static size_t cmap_len;

/// コマンドを登録する
void subcmd_add(subcmd *ptr) {
  int len = subcmd_len(ptr);
  
  subcmd **pptr;
  cmap_ptr = pptr = (subcmd **)realloc(cmap_ptr, sizeof(*cmap_ptr) * (1 + cmap_len + len));
  if (!pptr) {
    perror("ERROR: realloc");
    abort();
  }
  pptr += cmap_len;
  cmap_len += len;
  while (ptr->cmd) { *pptr++ = ptr++; }
  *pptr = 0;
}

/// 名前で整列するための比較関数
static int subcmd_cmp(const void *a, const void *b) {
  subcmd **aa = (subcmd **)a, **bb = (subcmd **)b;
  return strcmp((*aa)->cmd, (*bb)->cmd);
}

/// 登録済みサブコマンドの確認
static void subcmd_show(subcmd **ptr, size_t len, FILE *fout) {
  subcmd **pt;
  qsort(cmap_ptr, cmap_len, sizeof cmap_ptr[0], subcmd_cmp);

  for (pt = ptr; pt < ptr + len; pt++) {
    fprintf(fout, "%s\n", (*pt)->cmd);
  }
  fprintf(stderr, "INFO: %d subcommand aviable.\n", (int)len);
}

/// サブコマンドを実行
static int subcmd_run(subcmd **ptr, size_t len, int argc, char **argv) {

  if (argc <= 1) {
    subcmd_show(ptr, len, stderr);
    return -1;
  }

  const char *cmd = argv[1];

  subcmd **pt;
  for (pt = ptr; pt < ptr + len; pt++) {
    if (strcmp((*pt)->cmd, cmd) == 0) {
      fprintf(stderr, "INFO: %s starting.\n", (*pt)->cmd);
      return (*(*pt)->func)(argc - 1, argv + 1);
    }
  }

  fprintf(stderr, "ERROR: no such sub command.\n");
  return -1;
}


int subcmd_run(int argc, char **argv, void (*usage)(const char *cmd)) {
  int rc = subcmd_run(cmap_ptr, cmap_len, argc, argv);
  if (rc < 0 && usage) (*usage)(argv[0]);
  return rc;
}

