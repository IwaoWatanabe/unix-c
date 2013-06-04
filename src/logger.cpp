/*! \file
 * \brief 簡易ロギングのAPI
 */

#include <cstdarg>
#include <cstdlib>
#include <iostream>
#include <libgen.h>

#include "elog.h"

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
    va_start(args, format);
    int ct = vlog(format, args);
    va_end(args);
    return ct;
  }

  /// ログ出力
  /**
     printf(3)の様式したがって、メッセージをログ出力する。
     formatの末尾に \n が含まれる場合は都度、フラッシュする。
     出力バイト数を返す。
   */
  int
  Simple_logger::vlog(const char *format, va_list ap)
  {
    va_list args;
    int ct = 0;
    if (!error_log) return 0;

    ct = vfprintf(error_log, format, ap);

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

// --------------------------------------------------------------------------------
namespace uc {

  /// ログを振り分ける操作を担当するクラス
  /*
    この実装は、コンソールとファイルにログ出力する。
    スレッドの排他制御を入れていない。
   */
  class ELog_Manager {
    Simple_logger app_log, auth_log;
    std::string ident, dir;
  public:
    ELog_Manager(const char *ident, const char *log_dir);
    virtual ~ELog_Manager() {}
    /// ログをログ出力の実装クラスに転送する
    virtual int vlog(int level, const char *format, va_list ap);
    /// ログ出力インスタンスを再初期化する
    virtual void reopen();
  };

  void
  ELog_Manager::reopen() {
    app_log.close_log();
    auth_log.close_log();

    string logfile(dir);
    logfile += "/";
    logfile += ident;
    logfile += ".log";
    app_log.init_log(logfile.c_str());

    logfile = dir;
    logfile += "/";
    logfile += ident;
    logfile += "-auth.log";
    auth_log.init_log(logfile.c_str());
  }

  ELog_Manager::ELog_Manager(const char *ident, const char *log_dir) {
    char ibuf[strlen(ident) + 1];
    char *log_prefix = basename(strcpy(ibuf, ident));
    this->ident = log_prefix;
    this->dir = log_dir;
    reopen();
  }

  int ELog_Manager::vlog(int level, const char *format, va_list ap) {
    string buf;

    switch(level) {
    case ELog::E:
      buf += "ERROR: ";
      break;
    case ELog::W:
      buf += "WARN: ";
      break;
    case ELog::N:
      buf += "NOTICE: ";
      break;
    case ELog::F:
      buf += "FATAL: ";
      break;
    case ELog::I:
      buf += "INFO: ";
      break;
    case ELog::A:
      buf += "AUTH: ";
      break;
    case ELog::D:
      buf += "DEBUG: ";
      break;
    case ELog::T:
      buf += "TRACE: ";
      break;
    }

    buf += format;

    if (level == ELog::A) {
      // 監査ログは、別に出す
      va_list ap2;
      va_copy(ap2,ap);
      auth_log.vlog(buf.c_str(), ap2);
      va_end(ap2);
    }

    {
      // コンソールにも出力する
      va_list ap3;
      va_copy(ap3,ap);
      vfprintf(stderr, buf.c_str(), ap3);
      va_end(ap3);
    }

    int ct = app_log.vlog(buf.c_str(), ap);
    return ct;
  }

  ELog::ELog() : mgr(0) { }
  ELog::~ELog() { if (mgr) mgr->reopen(); }

  void
  ELog::init_elog(const char *ident) {
    if (mgr) return;
    const char *work_dir = getenv("WORKDIR");
    if (!work_dir) work_dir = "work";
    mgr = new ELog_Manager(ident, work_dir);

    elog(T, "elog initializing: %s: %s\n", ident, work_dir);
  }
  
  int
  ELog::elog(const char *format, ...) {
    va_list args;
    va_start(args, format);
    int ct = mgr ? mgr->vlog(E, format, args) : 
      vfprintf(stderr,format, args);
    va_end(args);
    return ct;
  }

  int
  ELog::elog(int level, const char *format, ...) {
    if (!mgr) return 0;
    va_list args;
    va_start(args, format);
    int ct = mgr ? mgr->vlog(level, format, args) : 
      vfprintf(stderr,format, args);
    va_end(args);
    return ct;
  }

  int
  ELog::velog(int level, const char *format, va_list args) {
    if (!mgr) return 0;
    int ct = mgr ? mgr->vlog(level, format, args) : 
      vfprintf(stderr,format, args);
    return ct;
  }

};

// --------------------------------------------------------------------------------

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

    const char *logfile_name = getenv("LOGFILE");
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


  // ロガーを使うアプリ
  class testapp2 : uc::ELog {
  public:
    int run(int argc, char **argv);
  };

  /// 動作開始
  int testapp2::run(int argc, char **argv) {

    init_elog(argv[0]);

    if (argc == 1) {
      elog("logger test\n");
      elog(F, "logger test\n");
      elog(E, "logger test\n");
      elog(W, "logger test\n");
      elog(N, "logger test\n");
      elog(I, "logger test\n");
      elog(A, "logger test\n");
      elog(D, "logger test\n");
      elog(T, "logger test\n");
    }

    for (int i = 1; i < argc; i++) {
      elog(I, "%d: %s\n",i, argv[i]);
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

// ELogのテスト
static int logger_sample02(int argc, char **argv) {
  testapp2 aa;
  return aa.run(argc,argv);
  /*
    スコープを外れると、ログはクローズされる想定。
   */
}

#include "subcmd.h"

subcmd logger_cmap[] = {
  { "slog01", logger_sample01, },
  { "slog02", logger_sample02, },
  { 0 },
};

