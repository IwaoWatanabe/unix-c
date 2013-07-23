/*! \file
 * \brief 正規表現を扱うサンプル・コード
 */

#include <string>

// --------------------------------------------------------------------------------

namespace uc {

  /// 正規表現にマッチした領域の情報を格納する構造体
  struct Matcher {
    int start_offset, ///< 条件にマッチする領域の開始位置
      end_offset; ///< 条件にマッチする領域の終了位置
    union {
      const char *text; ///< 診断対象のテキスト（マルチバイト）
      const wchar_t *wtext; ///< 診断対象のテキスト（ワイドキャラクタ）
    } ptr;

    /// 条件にマッチする領域の開始位置
    int offset() { return start_offset; }
    /// 条件にマッチする領域の大きさ
    int size() { return end_offset - start_offset; }
    /// 条件にマッチする領域のテキストを入手
    std::string str() { return std::string(ptr.text + start_offset, size()); }
    /// 条件にマッチする領域のテキストを入手
    std::wstring wstr() { return std::wstring(ptr.wtext + start_offset, size()); }
  };

  /// 正規表現を扱うクラスの共通インタフェース
  /**
     wchar_t が利用できるかどうかは実装クラスによる。
     (posix regex では未サポート)
   */
  class Regular_Expression {
  public:
    enum Option { NONE, IGNORE_CASE, EXTENDED = 1<<1, NEWLINE = 1<<2, NOSUB = 1<<3, };

    Regular_Expression(const char *reg, int option = 0);
    Regular_Expression(const wchar_t *reg, int option = 0);
    virtual ~Regular_Expression() { };

    /// 正規表現に誤りがあれば、その内容を告げるテキストが入手できる。
    virtual const char *get_error_text() = 0;
    /// 格納されている正規表現
    virtual const char *re_text() = 0;
    /// 格納されている正規表現（ワイドキャラクタ）
    virtual const wchar_t *re_wtext() = 0;
    /// Macher で返却する正規表現のグループ数を返す。
    virtual int group_count() = 0;
    /// テキストの一部が、正規表現に合致するか診断する。
    virtual bool search(const char *text, int option = 0) = 0;
    /// テキストの一部が、正規表現に合致するか診断し、結果を入手する。
    virtual bool search(const char *text, Matcher *match, size_t n_mathch, int option = 0) = 0;
    /// テキストの一部が、正規表現に合致するか診断する。
    virtual bool search(const wchar_t *text, int option = 0) = 0;
    /// テキストの一部が、正規表現に合致するか診断し、結果を入手する。
    virtual bool search(const wchar_t *text, Matcher *match, size_t n_mathch, int option = 0) = 0;
    /// テキスト全体が正規表現に合致するか診断する。
    virtual bool match(const char *text, int option = 0) = 0;
    /// テキスト全体が正規表現に合致するか診断し、結果を入手する。
    virtual bool match(const char *text, Matcher *match, size_t n_mathch, int option = 0) = 0;
    /// テキスト全体が正規表現に合致するか診断する。
    virtual bool match(const wchar_t *text, int option = 0) = 0;
    /// テキスト全体が正規表現に合致するか診断し、結果を入手する。
    virtual bool match(const wchar_t *text, Matcher *match, size_t n_mathch, int option = 0) = 0;
    /// テキストの正規表現に合致する箇所を置き換えたテキストを入手する。
    virtual std::string replace(const char *text, const char *replace, int option = 0) = 0;
    /// テキストの正規表現に合致する箇所を置き換えたテキストを入手する。
    virtual std::wstring replace(const wchar_t *text, const wchar_t *replace, int option = 0) = 0;
  };

  /// Posix regex 関数に基づいた正規表現サポートクラス
  class RegExp : public Regular_Expression {
    int n_group;
  public:
    RegExp(const char *reg, int option = 0);
    ~RegExp();
    const char *get_error_text();
    const char *re_text();
    const wchar_t *re_wtext();
    int group_count();
    bool search(const char *text, int option = 0);
    bool search(const char *text, Matcher *match, size_t n_mathch, int option = 0);
    bool search(const wchar_t *text, int option = 0);
    bool search(const wchar_t *text, Matcher *match, size_t n_mathch, int option = 0);
    bool match(const char *text, int option = 0);
    bool match(const char *text, Matcher *match, size_t n_mathch, int option = 0);
    bool match(const wchar_t *text, int option = 0);
    bool match(const wchar_t *text, Matcher *match, size_t n_mathch, int option = 0);
    std::string replace(const char *text, const char *replace, int option = 0);
    std::wstring replace(const wchar_t *text, const wchar_t *replace, int option = 0);
  };

};

// --------------------------------------------------------------------------------

#include <sys/types.h>
#include <regex.h>

using namespace std;

namespace uc {

  RegExp::RegExp(const char *reg, int option)
    : Regular_Expression(reg)
  {
  }

  RegExp::~RegExp() {
  }

  const char *RegExp::get_error_text() {
  }

  const char *RegExp::re_text() {
    return 0;
  }

  const wchar_t *RegExp::re_wtext() { return 0; }

  int RegExp::group_count() { return n_group; }

  bool RegExp::search(const char *text, int option) {
    return false;
  }

  bool RegExp::search(const char *text, Matcher *match, size_t n_mathch, int option) {
    return false;
  }

  bool RegExp::match(const char *text, int option) {
    return false;
  }

  bool RegExp::match(const char *text, Matcher *match, size_t n_mathch, int option) {
    return false;
  }

  std::string RegExp::replace(const char *text, const char *replace, int option)  {
    return text;
  }


  bool RegExp::search(const wchar_t *text, int option) { return false; }
  bool RegExp::search(const wchar_t *text, Matcher *match, size_t n_mathch, int option) { return false; }
  bool RegExp::match(const wchar_t *text, int option)  { return false; }
  bool RegExp::match(const wchar_t *text, Matcher *match, size_t n_mathch, int option)  { return false; }
  std::wstring RegExp::replace(const wchar_t *text, const wchar_t *replace, int option)  { return text; }

};

extern "C" {

  /// Posix RE を使ったgrep実装
  static int cmd_grep(int argc, char **argv) {
    return 0;
  }

};

// --------------------------------------------------------------------------------

#ifdef USE_SUBCMD
#include "subcmd.h"

subcmd index_cmap[] = {
  { "grep", cmd_grep, },
  { 0 },
};

#endif

