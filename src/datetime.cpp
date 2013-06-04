/*! \file
 * \brief 日時操作関連のサンプルコード
 */

#include <cerrno>
#include <clocale>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <string>
#include <time.h>

namespace uc {

  /// 秒制度の日時操作を行うクラス
  /**
    注意：生成されるテキストはロケールの設定に従う
   */
  class Date {
    static struct tm zero_tbuf;
    struct tm tbuf;
  protected:
    time_t sec;
  public:
    Date() { tbuf = zero_tbuf; now(); }
    Date(Date &t) : sec(t.sec), tbuf(t.tbuf) { }
    Date(time_t t) { set_utime(t); }
    Date(int y,int m, int d, bool hold_time_part = true);
    Date(int y,int m, int d, int h, int m, int s);

    /// 日付表現の現在のlocale設定を入手
    static const char *get_locale();
    /// 日付表現の現在のlocale設定
    static const char *set_locale(const char *locale);

    /// 日付の設定値
    /*
      hold_time_part で true を渡すと、時刻について0リセットしない。
      不正な日付を渡すと false が返る
     */
    bool set_date(int y,int m, int d, bool hold_time_part = true);

    /// 日時の設定値
    bool set_date_time(int y, int m, int d, int h, int m, int s);

    bool set_utime(time_t sec);
    time_t get_utime() { return sec; }

    /// 日付書式を指定してテキストを入手する
    std::string get_date_text(const char *format);

    enum Style { NORMAL, SHORT, LONG, ISO, ISO_DATE, };

    /// 標準的な日付テキストを入手する
    std::string get_date_text(int style = NORMAL);

    // 現在日時を取得する
    /** 自身のリファレンスを返す */
    Date &now();

    /// 時刻の差（秒数）を入手する
    double diff(Date &d);

    int year();
    int month();
    int day();
    int hour();
    int minute();
    int second();
    /// 暦週の日（曜日）を返す。0-6, 0は日曜日
    int cwday();
    /// 暦週を返す。1-53
    int cweek();

    Date &tomorrow();
    Date &yesterday();
    Date &next_week();
    Date &last_week();
    Date &add(int hour,int minute,int second);

    /// 閏年判定
    bool is_leap();

  };
};

// --------------------------------------------------------------------------------

using namespace std;

namespace uc {

  struct tm Date::zero_tbuf;

  const char *
  Date::get_locale() {
    return ::setlocale(LC_TIME,NULL);
  }

  const char *
  Date::set_locale(const char *locale) {
    return ::setlocale(LC_TIME,locale);
  }

  Date &Date::now() {
    if (time(&sec) == (time_t)-1) perror("time");
    else if (!::localtime_r(&sec, &tbuf)) perror("localtime_r");
    return *this;
  }

  bool Date::set_utime(time_t sec) {
    if (::localtime_r(&sec, &tbuf)) {
      this->sec = sec;
      return true;
    }
    return false;
  }

  Date::Date(int y,int m, int d, bool hold_time_part) {
    if (hold_time_part) {
      now();
      tbuf.tm_year = y - 1900;
      tbuf.tm_mon = m - 1;
      tbuf.tm_mday = d;
    }
    else {
      tbuf = zero_tbuf;
      tbuf.tm_year = y - 1900;
      tbuf.tm_mon = m - 1;
      tbuf.tm_mday = d;
    }
    if ((sec = mktime(&tbuf)) == (time_t)-1) perror("mktime");
    else if (!::localtime_r(&sec, &tbuf)) perror("localtime_r");
  }

  bool Date::set_date(int y,int m, int d, bool hold_time_part) {
    struct tm ttbuf;
    if (hold_time_part) {
      now();
      ttbuf = tbuf;
      ttbuf.tm_year = y - 1900;
      ttbuf.tm_mon = m - 1;
      ttbuf.tm_mday = d;
    }
    else {
      ttbuf = zero_tbuf;
      ttbuf.tm_year = y - 1900;
      ttbuf.tm_mon = m - 1;
      ttbuf.tm_mday = d;
    }
    time_t dd;
    if ((dd = mktime(&ttbuf)) == (time_t)-1) return false;
    if (!::localtime_r(&dd, &tbuf)) return false;
    sec = dd;
    return true;
  }

  bool Date::set_date_time(int y, int m, int d, int hh, int mm, int ss) {
    struct tm ttbuf = zero_tbuf;
    ttbuf.tm_year = y - 1900;
    ttbuf.tm_mon = m - 1;
    ttbuf.tm_mday = d;
    ttbuf.tm_hour = hh;
    ttbuf.tm_min = mm;
    ttbuf.tm_sec = ss;
    time_t dd;
    if ((dd = mktime(&ttbuf)) == (time_t)-1) return false;
    if (!::localtime_r(&dd, &tbuf)) return false;
    sec = dd;
    return true;
  }

  int Date::year() { return tbuf.tm_year + 1900; }
  int Date::month() { return tbuf.tm_mon + 1;}
  int Date::day() { return tbuf.tm_mday;}
  int Date::hour() { return tbuf.tm_hour;}
  int Date::minute() { return tbuf.tm_min;}
  int Date::second()  { return tbuf.tm_sec;}
  int Date::cwday()  { return tbuf.tm_wday;}
  int Date::cweek()  { return tbuf.tm_yday / 7 + 1;}

  double Date::diff(Date &d) { return difftime(sec, d.sec); }

  string 
  Date::get_date_text(const char *format) {
    size_t len = 100;
    char *sbuf = (char *)malloc(len);
    int retry = 5;
    while (retry-- >= 0) {
      size_t n = strftime(sbuf, len, format, &tbuf);
      if (n > 0 && n < len) break;
      char *tt = (char *)realloc(sbuf, len *= 2);
      if (!tt) break;
      sbuf = tt;
    }
    string res(sbuf);
    free(sbuf);
    return res;
  }

  string 
  Date::get_date_text(int style) {
    char *iso = "%Y-%m-%d %H:%M:%S";
    return get_date_text(iso);
  }
  
};


// --------------------------------------------------------------------------------

namespace {
  // 日時情報を使うアプリ
  class testapp1 : uc::Date {
  public:
    int run(int argc, char **argv);
  };

  /// 動作開始
  int testapp1::run(int argc, char **argv) {
    char *format = "%Y-%m-%d %H:%M:%S";
    printf("Locale: %s\n", get_locale());
    printf("Now: %s - %s\n", get_date_text().c_str(), get_date_text(format).c_str());

    printf("part: %04d-%02d-%02d %02d:%02d:%02d\n", 
	   year(), month(), day(), hour(), minute(), second());

    printf("part: cweek:%d  cwday:%d\n", cweek(), cwday());

    set_date_time(2014, 10, 5, 20, 45, 30);
    printf("set: %s - %s\n", get_date_text().c_str(), get_date_text(format).c_str());

    printf("utime: %llu\n", (unsigned long long)get_utime());

    return 0;
  }
};

// --------------------------------------------------------------------------------

static int date_sample01(int argc, char **argv) {
  testapp1 aa;
  return aa.run(argc,argv);
}

#ifdef USE_SUBCMD

#include "subcmd.h"

subcmd date_cmap[] = {
  { "date01", date_sample01, },
  { 0 },
};

#endif

