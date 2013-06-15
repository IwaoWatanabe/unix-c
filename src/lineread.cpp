/*! \file
 * \brief テキストファイルを行単位で読込むサンプル・コード
 */

#include <cstdio>
#include <cstdlib>
#include <string>

#include "elog.hpp"

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

    /// 想定するテキストのエンコーディングを指定する
    virtual void set_encoding(const char *enc);

    /// 想定するテキストのエンコーディングを入手する
    virtual const char * get_encoding() { return encoding.c_str(); }
  };

};

using namespace std;

#include <clocale>

// --------------------------------------------------------------------------------

namespace uc {
  Text_Source::~Text_Source() { }

  /// 文字セット操作の現在のlocale設定を入手
  const char *Text_Source::get_locale() {
    return setlocale(LC_CTYPE, NULL);
  }

  /// 文字セット操作の現在のlocale設定
  const char *Text_Source::set_locale(const char *locale) {
    const char *lctype = setlocale(LC_CTYPE, locale);
    if (lctype) {
      fprintf(stderr,"INFO: change loclae (LC_TYPE) to %s (codeset: %s)\n", 
	      lctype, nl_langinfo(CODESET));
    }
    else {
      lctype = setlocale(LC_CTYPE, NULL);
      fprintf(stderr,"WARNING: can not change loclae (LC_TYPE) to %s: (use %s)\n", locale, lctype);
    }
    return lctype;
  }

  wchar_t *Text_Source::read_wline(size_t *len) {
    return NULL;
  }

  char *Text_Source::read_line(size_t *len) {
    char *tbuf = buf.c;
    size_t tlen = 20;

    for(;;) {
      if (!tbuf || tbuf + tlen > buf.c + buf_len) {

	size_t pdiff = tbuf - buf.c;
	buf_len = pdiff + tlen;

	char *nbuf = (char *)realloc(buf.c, buf_len);
	if (!nbuf) {
	  if (logger)
	    logger->elog("realloc ,%u:(%d):%s\n", buf_len, errno, strerror(errno));

	  if (len) *len = pdiff ? tbuf - buf.c : 0;
	  return pdiff ? buf.c : NULL;
	}

	if (logger)
	  logger->elog(ELog::T, "realloc ,%u\n", buf_len);

	tbuf = nbuf + pdiff;
	buf.c = nbuf;
      }

      size_t rlen; // 取り込み字数
      char *tt = fgets(tbuf, tlen, fp);

      if (!tt || (rlen = strlen(tt)) == 0) {
	if (len) *len = tbuf > buf.c ? tbuf - buf.c : 0;
	return tbuf > buf.c ? buf.c : NULL;
      }

      if (tt[rlen -1] != '\n') { tbuf += rlen; continue; }
      
      if (len) *len = tbuf + rlen - buf.c;
      return buf.c;
    }
  }

  long Text_Source::close_source() {
    if (!fp) return counter;

    if (0 != fclose(fp)) {
      if (logger) {
	const char *last_source = get_source_name();
	logger->elog(ELog::W, "fclose %s:(%d):%\n",last_source,errno,strerror(errno));
      }
    }
    fp = NULL;
    return counter;
  }

  void Text_Source::rewind() {
    if (!fp) return;
    ::rewind(fp);
  }

  void Text_Source::seek(long offset, int whence) {
    if (!fp) return;
    fseek(fp,offset,whence);
  }

  long Text_Source::tell() {
    if (!fp) return -1;
    return ftell(fp);
  }

  void Local_Text_Source::set_encoding(const char *enc) {
  }

  Local_Text_Source::Local_Text_Source() {
    logger = this;
    init_elog("text-source");
  }

  Local_Text_Source::~Local_Text_Source() {
    close_source();
  }

  /// ファイルの読み込みを開始する
  /*
    クローズしないで呼び出しても動作する。
    その場合は、前のストリームを自動で閉じる。
   */
  bool Local_Text_Source::open_read_file(const char *file_name) {
    FILE *fp = fopen(file_name, "r");
    if (!fp) {
      elog("fopen %s,r:(%d):%s\n",file_name,errno,strerror(errno));
      return false;
    }

    close_source();

    this->file_name = file_name;
    set_stream(fp);
    return true;
  }

};

// --------------------------------------------------------------------------------

#include <memory>
#include <iostream>

extern "C" {

  uc::Local_Text_Source *create_Local_Text_Source() {
    return new uc::Local_Text_Source();
  }

  /// テキスト・ファイルの行読み込みのテスト
  static int cmd_text_read(int argc,char **argv) {
    int opt;
    bool verbose = false;
    bool by_wide_char = false;
    char *lang = "";

    while ((opt = getopt(argc,argv,"L:vw")) != -1) {
      switch(opt) { 
      case 'v': verbose = true; break;
      case 'w': by_wide_char = true; break;
      case 'L': lang = optarg; break;
      }
    }
    auto_ptr<uc::Local_Text_Source> ts(create_Local_Text_Source());

    ts->set_locale(lang);
    /*
      ロケールの設定。
      これを呼ばなければ ロケール"C" として処理される。
      マルチバイトで文字コードを変換しないなら、特に気にする必要はない。
     */

    for (int i = optind; i < argc; i++) {

      if (!ts->open_read_file(argv[i])) break;

      if (by_wide_char) {
	wchar_t *wline;
	size_t len;
	while((wline = ts->read_wline(&len)) != 0) {
	  /*
	    ワイドキャラクタとして読込む。
	    文字コードは C_TYPEで設定されたものを利用する。
	    len には、読込んだ文字数が返却される。

	    返却されたポインタは、次の行を読込むか
	    close_source を呼び出すまで有効。
	   */
	  wcout << wline;
	}
      }
      else {
	char *line;
	size_t len;
	while((line = ts->read_line(&len)) != 0) {
	  /*
	    マルチバイト・キャラクタとして読込む。
	    len には、読込んだ文字数が返却される。

	    返却されたポインタは、次の行を読込むか
	    close_source を呼び出すまで有効。
	   */
	  cout << line;
	}
      }

      long lines = ts->close_source();
      /*
	速やかにストリームを開放するか
	読み取った行数を把握したければ
	close_source を呼び出せばよい。

	close_source を呼び出さなくても、
	再オープンや、デストラクタの呼び出しによってもストリームは閉じられる。
       */
      cerr << "INFO: read " << argv[i] << " " << lines << " lines." << endl;
    }
  }
};

// --------------------------------------------------------------------------------

#ifdef USE_SUBCMD
#include "subcmd.h"

subcmd lineop_cmap[] = {
  { "line-read", cmd_text_read, },
  { 0 },
};

#endif

