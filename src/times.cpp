/*! \file
 * \brief 時間操作関連のサンプルコード
 */

#include "datetime.hpp"

#include <cstdio>
#include <cstring>
#include <cerrno>
#include <iostream>
#include <string>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

using namespace std;

namespace uc {

  /// 時間計測の基準時間を記録
  bool Time_recored::time_load() {
    struct timeval now;
    
    if (gettimeofday(&now, 0) == -1) {
      /* gettimeofday() は紀元 (1970年1月1日00:00:00 UTC) からの経過時間を秒単位で返す。
	 もし t が NULL でなかったら返り値は t の指しているメモリにも格納される。
	 エラーの場合は (-1) を返し、errno を設定する。
      */
      perror("gettimeofday");
      return false;
    }
    save_time = now;
    return true;
  }

  bool
  Time_recored::file_mtime(const char *file_name) {
    struct stat sbuf;
    int rc = stat(file_name, &sbuf);
    if (rc == 0)
      save_time.tv_sec = sbuf.st_mtime;
    else
      fprintf(stderr,"stat %s: %s\n",file_name,strerror(errno));
    return rc == 0;
  }

  string 
  Time_recored::time_text() {
    struct tm tbuf;
    char sbuf[40];

    if (localtime_r(&save_time.tv_sec, &tbuf) == NULL) {
      /* localtime_r()は、time_t によって表現されれる時刻をローカル時刻情報に変換する。
       */
      fputs("WARRNING: cannot localtime convert.\n", stderr);
      return "";
    }
  
    sprintf(sbuf, "%4d-%02d-%02d %02d:%02d:%02d", 
	    tbuf.tm_year + 1900, tbuf.tm_mon + 1, tbuf.tm_mday + 1, 
	    tbuf.tm_hour, tbuf.tm_min, tbuf.tm_sec);
    return sbuf;
  }

  /// 基準時間からの経過時間を出力
  void Time_recored::time_report(const char *msg, FILE *fout, long counter) {
    struct timeval now;
    if (gettimeofday(&now, 0) == -1) {
      perror("gettimeofday");
      return;
    }
    double begin_time = save_time.tv_usec * 1e-6 + save_time.tv_sec,
      sec = now.tv_usec * 1e-6 + now.tv_sec - begin_time;

    if (counter < 0) {
      fprintf(fout,"%s%s in %.2f sec\n", 
	      report_prefix ? report_prefix : "", msg, sec);
      return;
    }
    double rps = counter/sec;
    fprintf(fout,"%s%ld %s in %.2f sec (%.0f /sec)\n", 
	    report_prefix ? report_prefix : "", counter, msg, sec, rps);
  }


  /// 基準時間からの経過時間を文字列に追加
  void Time_recored::time_report(const char *msg, string &buf, long counter) {
    struct timeval now;
    if (gettimeofday(&now, 0) == -1) {
      perror("gettimeofday");
      return;
    }
    double begin_time = save_time.tv_usec * 1e-6 + save_time.tv_sec,
      sec = now.tv_usec * 1e-6 + now.tv_sec - begin_time;

    char tbuf[100];

    if (report_prefix) buf += report_prefix;
    if (counter < 0) {
      buf += msg;
      sprintf(tbuf," in %.2f sec\n", sec);
      buf += tbuf;
      return;
    }

    double rps = counter/sec;
    sprintf(tbuf,"%ld ",counter);
    buf += tbuf;
    buf += msg;
    sprintf(tbuf," in %.2f sec (%.0f /sec)\n", sec, rps);
    buf += tbuf;
  }
};


// 時間表示のテスト
static int time_sample01(int argc, char **argv) {

  uc::Time_recored tr, fm;
  tr.time_load();

  if (argc == 1) {
    fm.file_mtime(__FILE__);
    cerr << __FILE__ << ": mtime=" << fm.time_text() << endl;
  }

  for (int i = 1; i < argc; i++) {
    fm.file_mtime(argv[i]);
    cerr << argv[i] << ": mtime=" << fm.time_text() << endl;
  }

  usleep(800 * 1000);
  tr.time_report("time fetch.", stderr);

  return 0;
}


#include "subcmd.h"

subcmd time_cmap[] = {
  { "times", time_sample01, },
  { 0 },
};

