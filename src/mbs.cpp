/*! \File
 * \brief マルチバイト・テキストを操作する機能を提供します
 */

#include <cerrno>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <clocale>
#include <iconv.h>
#include <iostream>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>

namespace {

  using namespace std;

  /// マルチバイト文字コードセットを変換をサポートする
  class mbsconv {
    std::string internalEncode, convertEncode, src, target;
    iconv_t convertEngine;
    
    mbsconv();
    bool open(const std::string &src, const std::string &tocode);
    void close();
    
    const char *lookup(const char *enc);
    
  public:
    virtual ~mbsconv();

    /// 変換コンバータを入手する(現在のlocale相対)
    static mbsconv *createConvertHelper(const std::string &encode);

    std::string convert(const char *target, const std::string &tocode, const std::string &fromcode);

    /// 内部コードを指定している外部コードに変換する
    std::string encode(const std::string &txt);
    std::string encode(const char *txt);

    /// 指定している外部コードを内部コードに変換する
    std::string decode(const std::string &txt);
    std::string decode(const char *txt);

    /// 内部コードのエンコードを入手する
    std::string getInternalEncoding();

    /// 変換エンコードを入手する
    std::string getConvertEncoding();
  };

  // ロケールとiconvのエンコードの対応(日本語系のみ)
  static struct locale_iconv_tbl {
    char *locale, *iconv;
    locale_iconv_tbl(char *tl,char *ti) : locale(tl), iconv(ti) { }
  } litbl[] = {
    locale_iconv_tbl("ja_JP","EUC-JP-MS"),
    locale_iconv_tbl("ja_JP.eucjp","EUC-JP-MS"),
    locale_iconv_tbl("ja_JP.ujis","EUC-JP-MS"),
    locale_iconv_tbl("ja_JP.utf8","UTF-8"),
    locale_iconv_tbl("ja_JP.UTF-8","UTF-8"),
    locale_iconv_tbl("ja_JP.sjis","SJIS-WIN"),
    locale_iconv_tbl("japanese","EUC-JP-MS"),
    locale_iconv_tbl("japanese.euc","EUC-JP-MS"),
    locale_iconv_tbl("C","ISO_8859-1"),
  };

  /// ロケールとiconvのエンコードを対応を引く
  const char *mbsconv::lookup(const char *ctype) {
    char *iconv = NULL;
    for (int i = 0; i < sizeof litbl/sizeof litbl[0]; i++) {
      if (strcasecmp(litbl[i].locale, ctype) == 0)
	iconv = litbl[i].iconv;
    }
    return iconv;
  }

  mbsconv::mbsconv() : convertEngine(0) {
    // ロケールの値からiconvのエンコーディング名を決定する
    const char *ctype = setlocale(LC_CTYPE, NULL);
    const char *iconv = lookup(ctype);
    if (iconv == NULL) iconv = "ISO_8859-1";
    internalEncode.assign(iconv);
  }

  mbsconv::~mbsconv() { close(); }

  /// エンジンのリソースの解放
  void mbsconv::close() {
    if (!convertEngine) return;

    if (!::iconv_close(convertEngine)) {
      if (errno)
	cerr << "WARNING: iconv_close: " << strerror(errno) << endl;
    }
    convertEngine = 0;
  }

  /// 新しい変換エンジンを作成する
  bool mbsconv::open(const string &tcode, const string &fromcode) {

    iconv_t cd = ::iconv_open(tcode.c_str(), fromcode.c_str());
    if (cd < 0) {
      cerr << "ERROR: iconv_open: " << strerror(errno) << endl;
      return false;
    }

    cerr << "TRACE: iconv_open: " << tcode << ":" << fromcode << endl;
    close();
    convertEngine = cd;
    src = fromcode;
    target = tcode;
    return true;
  }

  string mbsconv::getConvertEncoding() {
    return convertEncode;
  }

  string mbsconv::getInternalEncoding() {
    return internalEncode;
  }

  mbsconv* mbsconv::createConvertHelper(const string &encode) {
    mbsconv *ret = new mbsconv();
    ret->convertEncode = encode;
    return ret;
  }

  string mbsconv::encode(const char *txt) {
    return convert(txt, internalEncode, convertEncode);
  }

  string mbsconv::encode(const string &txt) {
    return convert(txt.c_str(), internalEncode, convertEncode);
  }

  string mbsconv::decode(const char *txt) {
    return convert(txt, convertEncode, internalEncode);
  }

  string mbsconv::decode(const string &txt) {
    return convert(txt.c_str(), convertEncode, internalEncode);
  }

  string mbsconv::convert(const char *text, const string &fromcode, const string &tocode) {
    if (src.empty() || target.empty() || tocode != target || src != fromcode) {
      if (!open(tocode, fromcode)) return "";
    }

    string ret;
    char* psrc = const_cast<char*>(text);
    size_t srclen = strlen(text);

    char* dst = new char[srclen + 1];
    char* pdst = dst;
    size_t dstlen = srclen;
    size_t dstmax = srclen;

    while (srclen > 0) {
      char* psrc_org = psrc;

      size_t n = ::iconv(convertEngine, &psrc, &srclen, &pdst, &dstlen);
      if ((n != (size_t)-1 && srclen == 0) || (errno == EINVAL)) {
	srclen = 0;
	ret.append(dst, 0, dstmax-dstlen);
      }
      else {
	switch (errno) {
	case E2BIG:
	  ret.append(dst, 0, dstmax-dstlen);
	  pdst = dst;
	  dstlen = dstmax;
	  break;
	case EILSEQ:
	  ret.append(dst, 0, dstmax-dstlen);
	  ret.append(psrc, 0, 1);
	  psrc++;  srclen--;
	  pdst = dst;
	  dstlen = dstmax;
	  break;
	default:
	  ret.append(psrc_org);
	  srclen = 0;
	  break;
	}
	errno = 0;
      }
    }
    pdst = dst;
    dstlen = dstmax;

    if (::iconv(convertEngine, NULL, NULL, &pdst, &dstlen) != (size_t)-1) {
      ret.append(dst, 0, dstmax-dstlen);
    }

    delete [] dst;
    return ret;
  }

};

/// iconvを使った変換モジュールの振る舞いの確認

static int testIconv(int argc,char **argv) {

  setlocale(LC_ALL, "");

  auto_ptr<mbsconv> conv(mbsconv::createConvertHelper("EUC-JP-MS"));

  cerr << "Locale(ctype): " << setlocale(LC_CTYPE, NULL) << endl;
  cerr << "Internal Encoding: " << conv->getInternalEncoding() << endl;
  cerr << "Target Encoding: " << conv->getConvertEncoding() << endl;
  cerr << endl;

  // cout << conv->convert("ABCあいうえお123","UTF-8","EUC-JP") << endl;

  string tt = conv->encode("ABCあいうえお123");

  cout << conv->decode(tt) << endl;
  
  return 0;
}

// --------------------------------------------------------------------------------

/// テキストをワイド文字としてメモリに読み込む
/**
  wchar_t としてファイルから行単位でテキストを読み込む
  変換に失敗した行は読み捨てられる。

  @param path 対象ファイル
  @param lastmod 最終更新時間
  @param encoding 想定するテキスト・エンコーディング
  @param buf_len 読み込みバッファ・サイズ
 */
wstring load_wtext(const char *path, const char *encoding = "UTF-8", time_t *lastmod = 0, size_t buf_len = 4096) {
  auto_ptr<mbsconv> conv(mbsconv::createConvertHelper(encoding));

  FILE *fp;
  wstring wres;

  if (lastmod) {
	struct stat sbuf;
	int res = stat(path, &sbuf);
	if (res) {
	  cerr << "ERROR: stat faild:" << strerror(errno) << ":" << path << endl;
	err_return:
	  return L"";
	}
    *lastmod = sbuf.st_mtime;
  }

  if ((fp = fopen(path, "r")) == NULL) {
    cerr << "ERROR: fopen faild:" << strerror(errno) << ":" << path << endl;
    return wres;
  }

  char buf[buf_len];
  wchar_t wbuf[buf_len];

  int lineno = 0;
  while(fgets(buf,buf_len,fp)) {
    bool has_newline =  strrchr(buf, '\n') != NULL;
    if (has_newline) lineno++;

	string tt = conv->decode(buf);

    size_t len = mbstowcs(wbuf, tt.c_str(), buf_len - 1);
    if (len == (size_t)-1) {
      // 想定するエンコーディングでないため変換に失敗した。
      cerr << "ERROR: invalid encoding text: " << lineno << endl;
      continue;
    }
    wbuf[len] = 0;
    wres.append(wbuf);
  }

  if (fclose(fp) != 0) {
    cerr << "WARNING: fclose faild:" << strerror(errno) << ":" << path << endl;
  }

  return wres;
}

/// マルチバイトテキストをワイド文字で読み込むテスト
static int test_load_wtext(int argc,char **argv) {
  
  setlocale(LC_CTYPE, "");

  for (int i = 1; i < argc; i++) {
    wstring text = load_wtext(argv[i]);
    wcout << text;
  }

  return 0;
}

// --------------------------------------------------------------------------------

/// ワイド文字列からマルチバイト文字列
/// 変換できなかった場合はfalse
bool narrow(string &dst, const wstring &src) {
#if 0
  // こちらはベタな変換
  char *mbs = new char[src.length() * MB_CUR_MAX + 1];
  size_t len = wcstombs(mbs, src.c_str(), src.length() * MB_CUR_MAX + 1);
  dst = mbs;
  delete [] mbs;
  return len != (size_t)-1;
#else
  // こちらはstringのメモリ管理を利用した書き方
  size_t slen = src.length() * MB_CUR_MAX + 1;
  dst.resize(slen);
  size_t len = wcstombs((char *)dst.data(), src.c_str(), slen);
  bool result = len != (size_t)-1;
  if (result) dst.resize(len);
  return result;
#endif
}

/// マルチバイト文字列からワイド文字列
/// 変換できなかった場合はfalse
bool widen(wstring &dst, const string &src) {
#if 0
  // こちらはベタな変換
  wchar_t *wcs = new wchar_t[src.length() + 1];
  size_t len = mbstowcs(wcs, src.c_str(), src.length() + 1);
  dst = wcs;
  delete [] wcs;
  return len != (size_t)-1;
#else
  // こちらはwstringのメモリ管理を利用した書き方
  size_t slen = src.length() + 1;
  dst.resize(slen);
  size_t len = mbstowcs((wchar_t *)dst.data(), src.c_str(), slen);
  bool result = len != (size_t)-1;
  if (result) dst.resize(len);
  return result;
#endif
}

/// ワイドキャラクタの操作試験
static int test_wcs(int argc, char **argv) {
  char *lang = "", *last_lang;
  last_lang = setlocale(LC_ALL, lang);
  /// ワイドキャラクタを操作する前に setlocaleを呼び出す必要がある

  string mbs;
  wstring wcs;

  if (widen(wcs, "本日は晴天なり") && narrow(mbs, wcs)) {
    cerr << mbs << endl;
    wcout << wcs << endl;

    fprintf(stderr,"wcs:%ls\n"
	    "mbs:%s\n",wcs.c_str(), mbs.c_str());

    // %ls を使うと、fprintf内部でマルチバイトに変換して表示してくれる。
  }

  return 0;
}

// --------------------------------------------------------------------------------

#ifdef USE_SUBCMD
#include "subcmd.h"

subcmd mbs_cmap[] = {
  { "iconv", testIconv,  },
  { "wtext", test_load_wtext,  },
  { "wcs", test_wcs,  },
  { 0 },
};

#endif

