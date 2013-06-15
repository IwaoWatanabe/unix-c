/*! \file
 * \brief テキストファイルを行単位で読込むサンプル・コード
 */

#include "uc/text-reader.hpp"

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

