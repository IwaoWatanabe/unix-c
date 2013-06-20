/*! \file
 * \brief テキストファイルを読込む機能を提供する
 */

#ifndef _uc_text_reader
#define _uc_text_reader

#include <cstdio>
#include <cstdlib>
#include <string>

#include "uc/elog.hpp"

namespace uc {
  /// テキストを行単位で読み込む
  /**
     行の読み取りバッファの制御はこのクラスが制御する。
  */
  class Text_Source {
  protected:
    /// 読み込みバッファ
    union {
      char *c;
      wchar_t *wc;
      void *ptr;
    } buf;
    /// バッファサイズ(文字数)
    size_t buf_len;
    ELog *logger;

  private:
    /// 読み込みストリーム
    FILE *fp;
    /// 読み込み総合文字数
    int counter;
    
  protected:
    /// 読み込みストリームを設定する
    virtual void set_stream(FILE *stream) { fp = stream; counter = 0; }
    /// 読み込みソースの名称を入手する
    virtual const char *get_source_name() { return ""; }

    int get_counter() { return counter; }
    int countup() { counter++; }
    Text_Source() : fp(0), buf_len(0), logger(0), counter(0) { buf.ptr = 0; }

  public:
    /// 文字セット操作の現在のlocale設定を入手
    static const char *get_locale();
    /// 文字セット操作の現在のlocale設定
    static const char *set_locale(const char *locale);

    virtual ~Text_Source() = 0;

    /// 設定しているストリームを入手する
    virtual FILE *get_stream() { return fp; }

    /// テキストを１行読み取る
    /**
       読み込んだ行は、マルチバイトとして扱う
     */
    virtual char *read_line(size_t *len = 0);
    /// テキストを１行読み取る
    /**
       読み込んだ行は、ワイドキャラクタとして扱う
     */
    virtual wchar_t *read_wline(size_t *len = 0);

    /// 速やかに読み込みを終了する
    /**
       実装クラスのデストラクタからも呼ばれる。
       読み取られた総文字数を返す。
       既に処理が終了していた場合は特に処理せず、0を返す。
    */
    virtual long close_source();

    /// 最初から読み込み直す
    virtual void rewind();
    /// 読み込み位置を移動する
    virtual void seek(long offset, int whence);
    /// 読み込み位置を入手する
    virtual long tell();

    /// 想定するテキストのエンコーディングを指定する
    /**
       指定したエンコードを実際に読み取ることができるかどうかは実装に依存する。
     */
    virtual void set_encoding(const char *enc) = 0;
    /// 想定するテキストのエンコーディングを入手する
    virtual const char * get_encoding() = 0;
  };

  /// ファイル・システムのテキストを読み込む
  class Local_Text_Source : public Text_Source, ELog {
    std::string file_name, encoding;

    virtual const char *get_source_name() { return file_name.c_str(); }

  public:
    Local_Text_Source();
    ~Local_Text_Source();

    /// ファイルの読み込みを開始する
    virtual bool open_read_file(const char *file_name);
    virtual long close_source();

    /// 想定するテキストのエンコーディングを指定する
    virtual void set_encoding(const char *enc);

    /// 想定するテキストのエンコーディングを入手する
    virtual const char * get_encoding() { return encoding.c_str(); }
  };

  /// コマンドを動かして、その出力をテキストとして読み込む
  class Command_Text_Source : public Text_Source, ELog {
    std::string command_line, encoding;

    virtual const char *get_source_name() { return command_line.c_str(); }

  public:
    Command_Text_Source();
    ~Command_Text_Source();
    /// コマンドを呼出読み込みを開始する
    virtual bool open_pipe(const char *command_line);
    virtual long close_source();
    /// 想定するテキストのエンコーディングを指定する
    virtual void set_encoding(const char *enc);
    /// 想定するテキストのエンコーディングを入手する
    virtual const char * get_encoding() { return encoding.c_str(); }
  };

};

#include "uc/csv.hpp"

#endif


