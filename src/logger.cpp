
#include <cerrno>
#include <cstdio>
#include <cstring>

namespace uc {

  /// 素朴なAPPENDログファイル操作ツール
  class Simple_logger {
    FILE *error_log;
    const char *error_logfile_name;
    long counter;

  public:
    Simple_logger() :
      error_log(0), error_logfile_name(0), counter(0) { }
    virtual ~Simple_logger() { close_log(); } 

    /// ログ初期化
    virtual bool init_log(const char *logfile_name);
    /// ログ出力
    virtual int log(const char *format, ...);
    /// ログ記録終了
    virtual void close_log();
  };
};

#include <cstdarg>
#include <cstdlib>
#include <iostream>
#include <libgen.h>

using namespace std;

namespace uc {

  /**
     ファイルにログを出力する準備をする。
     ファイルの準備ができなければ、事由をエラー出力して falseを返す。
   */
  bool
  Simple_logger::init_log(const char *logfile_name)
  {
    if (error_log) return true;

    FILE *fp = fopen(logfile_name, "a+");
    if (!fp) {
      fprintf(stderr,"ERROR: fopen %s for logging failed:(%d) %s\n",
	      logfile_name, errno, strerror(errno));
      return false;
    }
    error_log = fp;
    error_logfile_name = strdup(logfile_name);
    return true;
  }

  /// ログ出力
  /**
     printf(3)の様式したがって、メッセージをログ出力する。
     formatの末尾に \n が含まれる場合は都度、フラッシュする。
     出力バイト数を返す。
   */
  int
  Simple_logger::log(const char *format, ...)
  {
    va_list args;
    int ct = 0;
    if (!error_log) return 0;

    va_start(args, format);
    ct = vfprintf(error_log, format, args);
    va_end(args);

    int len = strlen(format);
    if (format[len - 1] == '\n') fflush(error_log);
    /*
      書式の末尾に \n が含まれる場合は強制的にフラッシュする
     */

    counter += ct;
    return ct;
  }

  /// ログ記録中止
  /**
     ログの出力を終了する。
     それまでになんらかのメッセージを出力していれば、そのバイト数をレポートする。

     このメソッドを呼び出すと以後、#log を呼び出してもファイル出力は行わなくなる。
     ただし#init_log を呼び出せば再出力できるようになる。

     このメソッドは、デストラクタから自動的に呼び出される。
   */
  void 
  Simple_logger::close_log()
  {
    if (!error_log) return;

    if (fclose(error_log) != 0) {
      fprintf(stderr,"WARNING: fclose %s failed:(%d) %s\n", 
	      error_logfile_name, errno, strerror(errno));
    }
    error_log = 0;

    if (counter > 0)
      fprintf(stderr,"INFO: logger %s output %ld bytes.\n", 
	      error_logfile_name, counter);

    free((void *)error_logfile_name);
    error_logfile_name = 0;
  }

};

namespace {

  // ロガーを使うアプリ
  class testapp : uc::Simple_logger {
  public:
    int run(int argc, char **argv);
  };

  /// 動作開始
  int testapp::run(int argc, char **argv) {
    const char *workdir = getenv("WORKDIR");
    if (!workdir) workdir = "work";

    const char *logfile_name = getenv("LOGGER");
    if (!logfile_name) {
      char cmd[strlen(argv[0]) + 1];
      string logname = workdir;
      logname += "/";
      logname += basename(strcpy(cmd,argv[0]));
      logname += ".log";
      logfile_name = strdup(logname.c_str());
    }

    init_log(logfile_name);

    if (argc == 1)
      log("logger test\n");

    for (int i = 1; i < argc; i++) {
      log("%d: %s\n",i, argv[i]);
    }

    return EXIT_SUCCESS;
  }

};

// 時間表示のテスト
static int logger_sample01(int argc, char **argv) {
  testapp aa;
  return aa.run(argc,argv);
  /*
    スコープを外れると、ログはクローズされる想定。
   */
}


#include "subcmd.h"

subcmd logger_cmap[] = {
  { "slog01", logger_sample01, },
  { 0 },
};

