#ifndef __datetime_h
#define __datetime_h

#include <cstdio>
#include <ctime>
#include <string>
#include <sys/time.h>

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


  /// 簡易時間計測ツール
  class Time_recored {
    const char *report_prefix;
    struct timeval save_time;
  public:
    Time_recored(const char *prefix = "INFO: ") : 
      report_prefix(prefix) { 
      save_time.tv_sec = 0; save_time.tv_usec = 0;
    }
    
    /// 時間計測の基準時間を記録
    bool time_load();
    /// ファイルの最終更新時間を入手する
    bool file_mtime(const char *file_name);
    /// 時間のテキスト表現を入手する
    std::string time_text();

    void time_report(const char *msg, FILE *fout, long counter = -1L);
    void time_report(const char *msg, std::string &buf, long counter = -1L);
  };
};


#endif
