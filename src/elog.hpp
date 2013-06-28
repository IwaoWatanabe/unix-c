#ifndef __elog_h
#define __elog_h

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
    /// ログ出力
    virtual int vlog(const char *format, va_list ap);
    /// ログ記録終了
    virtual void close_log();
  };

  /// ログを出力するクラス
  class ELog {
    void *mgr;
    const char *ident;
  public:
    /// ログレベル
    enum Level { F, E, W, N, I, A, D, T };
    ELog();
    ELog(const char *ident);
    virtual ~ELog();
    /// ログ初期化
    virtual void init_elog(const char *ident);
    /// ログ出力
    virtual int elog(const char *format, ...);
    /// ログ出力(ログレベルの指定)
    virtual int elog(int level, const char *format, ...);
    /// ログ出力(ログレベルの指定)
    virtual int velog(int level, const char *format, va_list ap);
  };

};

extern int elog(const char *format, ...);
extern int elog(int level, const char *format, ...);

#endif

