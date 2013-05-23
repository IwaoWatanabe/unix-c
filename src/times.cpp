
#include <cstdio>
#include <cerrno>
#include <time.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

struct timebuf {
  time_t epoch_interval;
 public:
  /// システム時間を入手する
  int load_time();

  /// ファイルの最終更新時間を入手する
  bool file_mtime(const char *file_name);

  /// 時間情報を出力する
  void show_time(FILE *fp);
};

int
timebuf::load_time() {
  time_t now;
  
  if (time(&now) == (time_t)-1) {
    /* time() は紀元 (1970年1月1日00:00:00 UTC) からの経過時間を秒単位で返す。
       もし t が NULL でなかったら返り値は t の指しているメモリにも格納される。
       エラーの場合は ((time_t)-1) を返し、errno を設定する。
    */
    perror("time");
    return 0;
  }
  epoch_interval = now;
  return 1;
}

bool
timebuf::file_ntime(const char *file_name) {
  struct stat sbuf;
  int rc = stat(file_name, &sbuf);
  if (rc == 0)
    epoch_interval = sbuf.st_mtime;
  else
    fprintf(stderr,"stat %s: %s\n",file_name,strerror(errno));
  return rc == 0;
}

//static const char *week_name = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", };

void
timebuf::show_time(FILE *fp) {
  struct tm tbuf;

  if (localtime_r(&epoch_interval, &tbuf) == NULL) {
    /* localtime_r()は、time_t によって表現されれる時刻をローカル時刻情報に変換する。
     */
    fputs("localtime convert error\n", stderr);
    return;
  }
  
  fprintf(fp, "%4d-%02d-%02d %02d:%02d:%02d", 
	  tbuf.tm_year + 1900, tbuf.tm_mon + 1, tbuf.tm_mday + 1, 
	  tbuf.tm_hour, tbuf.tm_min, tbuf.tm_sec);
}

int main(int argc, char **argv) {

  timebuf tb;
  tb.load_time();
  tb.show_time(stdout);
  putchar('\n');

  tb.file_ntime(__FILE__);


  return 0;
}

