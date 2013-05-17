#include <cstdio>
#include <cstdlib>

// 終了コードを確認する

int main(int argc, char **argv) {
  int rc = 0;
  if (argc > 1) rc = atoi(argv[1]);
  if (argc > 2) {
    switch(*argv[2]) {
    case 'e': case 'E':  exit(rc);
    case 'a': case 'A':  abort();
    }
  }

  printf("rc: %d\n",rc);
  return rc;
}

