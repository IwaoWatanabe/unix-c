#include <cstdio>
#include <cstdlib>

class read_text {
  /// 読み込みファイル
  FILE *fp;
  /// 読み込みバッファ
  char *buf;
  /// バッファサイズ
  size_t buf_len;

public:
  void open_file(const char *file_name);
  void close_file();
};

