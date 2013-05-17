
/// オプション解析のサンプル

#include <cstdio>
#include <unistd.h>

int main(int argc, char **argv) {
  int opt;

  /// オプション a と c がフラグタイプ
  /// オプション b がパラメータ受け取りタイプ

  while ((opt = getopt(argc,argv,"ab:c")) != -1) {
    switch(opt) {
    case 'a':
      puts("option a");
      break;
    case 'b':
      printf("option b: %s\n",optarg); // オプション・パラメータを入手
      break;
    case 'c':
      printf("option c: %s\n",optarg); // この場合は取れない
      break;
    case '?':
      printf("unkown option: %c\n", optopt);
      return 1;
    }
  }

  // オプション以後のパラメータを入手

  for (int i = optind; i < argc; i++)
    printf("%d: %s\n", i, argv[i]);

  return 0;
}


