/*! \file
 * \brief ドキュメント検索の基本操作を行うサンプル・コード
 */

#include <string>
#include <vector>
#include <map>

// --------------------------------------------------------------------------------
namespace uc {

  /// 検索条件を格納するDTO
  /**

例えば、文書Xに対して、
\verbatim
search_word：A B
and_key：C D
or_key：E F
not_key：G H
or_key2：I J
include：A1=V1|V2&A2=V3|V4
exclude：A3=V5|V6&A4=V7|V8
\endverbatim

という条件が与えられた場合の論理式は、

\verbatim
(X.contains(A) || X.contains(B) )
&& (X.contains(C) && X.contains(D) )
&& (X.contains(E) || X.contains(F) )
&& !(X.contains(G) || X.contains(H) )
&& (X.contains(I) || X.contains(J) )
&& ( (X.attributeA1==V1 || X.attributeA2 == V2 ) && ( X.attributeA2==V3 || X.attirbuteA2==V4 ) )
&& !( (X.attributeA3==V5 || X.attributeA3 == V6 ) || ( X.attributeA4==V7 || X.attirbuteA4==V8 ) )
\endverbatim

となる。

search_word, and_key, or_key, not_keyに関しては、自動的に部分一致として扱われるが、
include, excludeで指定した属性は、指定された属性名により、部分一致となるか完全一致となるか挙動が決まる。
  */

  struct Index_Condition {
    std::vector<std::string> target, search_word, and_key, not_key, or_key, or_key2, or_key3;
    std::map<std::string,std::string> includes, excludes;
  };

  /// インデックスの検索結果の入手に利用する
  class Index_Documents {
  public:
    /// 次の対象ドキュメントのドキュメントIDの入手
    virtual bool fetch_next_document(std::string doc_id) = 0;
    virtual int get_fetch_count();

    Index_Documents() {}
    virtual ~Index_Documents() {}
  };

  /// インデックスの基本情報を入手する
  /*
    特定のディレクトリを対象とるす
    インデックスの更新管理を担当する。
  */
  class Index_Scanner {
  protected:
    std::string index_name, target_path;

    Index_Scanner(const char *name) : index_name(name) { }
    virtual ~Index_Scanner();

  public:
    /// インデックス名の入手
    virtual std::string get_index_name() { return index_name; }
    /// インデックスの対象ディレクトリ名の入手
    virtual std::string get_target_path() { return target_path; }
    /// インデックスに格納されているドキュメント総数を入手
    virtual long get_document_count() = 0;
    /// インデックスの最終更新日時を入手
    virtual long get_last_update() = 0;
    /// インデックスの更新状況の確認
    virtual std::string get_scan_state() = 0;
    /// ドキュメントの走査開始
    virtual void begin_scan_document() = 0;
    /// 次の対象ドキュメントのドキュメントIDの入手
    virtual bool fetch_next_document(std::string doc_id) = 0;
    /// インデックスの走査終了
    virtual void end_scan_document() = 0;
    /// 指示するドキュメントが保持するセクション情報を入手
    virtual std::string fetch_document_sections(const char *doc_id, std::vector<std::string> &list) = 0;
    /// ドキュメントIDに対応するセクション情報を入手
    virtual std::string fetch_document(const char *doc_id, const char *section) = 0;
    /// インデックスの更新
    virtual void update_index(std::string doc_id) = 0;

    /// インデックスに対して検索処理を行う
    virtual Index_Documents *query(Index_Condition &condition) = 0;
    /// インデックスに対して検索処理をする(件数確認のみ)
    virtual long count_query(Index_Condition &condition) = 0;

  };

  /// 全文検索の基本機能を利用する
  /*
    ファイルシステム上のインデックスの配置領域と、
    インデックス化するドキュメント群の対応に名前を付けて管理する。
    具体的なインデックス操作は Index_Scanner クラスに移譲する。

    利用者は最初に、set_index_directory で
    あらかじめインデックスを配置する領域を指示しておく必要がある。

    ライブラリはその index_directory 以下にインデックスとメタ情報を配置する。
    index_directory のレイアウトはライブラリの実装に依存するが、
    create_index で指示する名称の一部は、
    サブディレクトリ名の一部として利用される。

    create_indexで生成されたディレクトリ名の一覧を入手するには
    get_index_list　を利用する。

    Sennaを利用した実装は、create_indexを呼び出さずとも、
    その構造のディレクトリにインデックスを配置することにより動作するようになる。
    インデックス管理のメタデータはできるだけファイルシステムに配置する。
    ディレクトリごと他のノードに配置するだけで利用できるようになるからである。
  */
  class Index_Manager {
  public:
    virtual ~Index_Manager();
    /// 基準ディレクトリの入手
    virtual std::string &get_index_directory() { return index_dir; }
    virtual bool set_index_directory(const char *dir) = 0;
    /// インデックの一覧を入手する
    virtual void get_index_list(std::vector<std::string> &list) = 0;
    virtual void show_index_info(FILE *out, const char *index_name) = 0;
    /// 特定のディレクトリをインデックの管理下に置く
    virtual bool crete_index(const char *name, const char *dir_path) = 0;
    /// インデックを更新する
    virtual void update_index(const char *name) = 0;
    /// インデックスを廃止する
    virtual bool drop_index(const char *name) = 0;
    /// 走査制御クラスの入手
    virtual Index_Scanner *get_scanner(const char *name) = 0;

    /// 保持しているセクション名の入手
    /*
      0:title, 1:body, 2:author, 3:year, 4:month, 5:week, 6:doctype, 7:recipients
      それ以外に何が入るかは、ドキュメント属性による
    */
    virtual void get_section_list(const char *name, std::vector<std::string> &list) = 0;
    /// インデックスに対して検索処理を行う
    virtual Index_Documents *query(const char *name, Index_Condition &condition) = 0;
    /// インデックスに対して検索処理をする(件数確認のみ)
    virtual long count_query(const char *name, Index_Condition &condition) = 0;
    /// ドキュメントIDに対応するセクション情報を入手
    virtual std::string fetch_document(const char *name, const char *doc_id, const char *section = 0) = 0;

    static Index_Manager *create_index_manager();

  protected:
    std::string index_dir;
    Index_Manager() { }
  };
};

// --------------------------------------------------------------------------------

using namespace std;

#include <cassert>
#include <cerrno>
#include <cstdlib>
#include <dirent.h>
#include <fcntl.h>
#include <limits.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "uc/elog.hpp"
#include "uc/datetime.hpp"
#include "uc/local-file.hpp"

#include "db.h"
//#include "depot.h"
#include "senna/senna.h"

extern "C" int ends_with(const char *target, const char *suffix);

namespace uc {
  extern Local_File *create_Local_File();
};

/// 実装クラスを隠す

/// 全文検索用の名前空間
namespace idx {

  // --------------------------------------------------------------------------------

  class Index_Manager_Impl;
  class Senna_Index_Scanner;

  /// sennaインデックス・インスタンスと連動する Senna_Index_Scanner の従属クラス
  /*
    senna APIを直接的に呼び出す。
    論理的に同じドキュメントをインデックスで管理する。
    この実装は一つのsennaインデックスを操作するだけだが、
    拡張して従属情報も合わせて制御するサブクラスが作成されるかもしれない。

    http://qwik.jp/senna/FrontPageJ.html
  */

  class Senna_Index : uc::ELog {

    sen_records *and_records(const vector<string>& words, sen_records *aa = 0,
			     bool partial = true , bool not_flag = false, int *weight = 0, size_t weight_len = 1);
    sen_records *or_records(const vector<string>& words, sen_records *aa = 0,
			    bool partial = true, bool not_flag = false, int *weight = 0, size_t weight_len = 1);

  protected:
    sen_index *index;
    string index_path;
    Senna_Index_Scanner *scanner; ///< 逆参照
    uc::Time_recored rec;

    /// テキストを章や節といった細かい範囲で扱うオブジェクト
    struct Value {
      sen_values *value;
      Senna_Index *ref;

      Value(sen_values *value, Senna_Index *ref = 0);
      Value(Senna_Index *ref = 0);
      Value(const char *text, unsigned weight = 1, Senna_Index *ref = 0);
      Value(const string &text, unsigned weight = 1, Senna_Index *ref = 0);
      ~Value();
      bool add(const char *text, unsigned weight = 1);
      bool add(const string &text, unsigned weight = 1);
    };

  public:
    Senna_Index(Senna_Index_Scanner *sis = 0)
      : index(0), index_path(""), scanner(sis)
    {
      init_elog("Senna_Index");
    }

    virtual ~Senna_Index() { close_index(); }

    /// ドキュメントの検索結果を操作するオブジェクト
    struct Result {
      sen_records *rec;
      Senna_Index *ref;
      Result(sen_records *rec, Senna_Index *ref);
      ~Result();
      string next(int *score = 0), current_key();
      int current_score(), nhits(), find(const char *doc_key);
      bool rewind();
    };

    /// 語彙とIDを関連付けるオブジェクト
    struct Symbol {
      sen_sym *sym;
      Senna_Index *ref;
      Symbol(sen_sym *sym, Senna_Index *ref = 0);
      ~Symbol();
      bool close();
      void show_info(FILE *fp);
      int get_symbol_size();
      sen_id store_symbol(const char *key);
      bool remove_symbol(const char *key);
      sen_id id_of(const char *key);
      string key_of(sen_id id);
      sen_id next_id_of(sen_id id);
    };

    /// Sennaライブラリの利用を開始する
    static void init();
    /// Sennaライブラリの利用を終了
    static void finish();
    /// Sennaライブラリのエラーテキストを入手する
    static const char *get_error_text(sen_rc rc);
    /// エンコード文字列を内部表現に換えて返す
    static sen_encoding get_sen_encoding(const char *t = NULL);
    /// フラグ文字列を内部表現に換えて返す
    static int get_sen_flag(char *t);
    /// インデックスの存在チェック
    virtual bool test_index(const char *index_path);
    /// インデックスの利用を開始する
    virtual bool open_index(const char *index_path, int flags = 0);
    /// シンボルを指定して利用を開始する
    virtual bool open_index_with_symbol(const char *index_path, sen_sym *sym, int flags = 0);
    /// インデックスの概要レポートを出力する
    virtual void show_index_info(FILE *fp);
    /// インデックスの利用を終了する
    virtual void close_index();
    /// インデックスの名称を変える
    virtual bool rename_index(const char *old_path, const char *new_path);
    /// インデックスを削除する
    virtual bool drop_index(const char *index_path);
    /// インデックスを更新する
    virtual bool update_index(const char *doc_key,
			      const string *new_value, const string *old_value = 0);
    /// インデックスを更新する
    virtual bool update_index(const char *doc_key, int section,
			      const Value *new_value, const Value *old_value = 0);
    /// テキストをインデックスから検索する
    virtual Result *search(const char *text);
    /// テキストをインデックスから検索する
    virtual Result *search(const uc::Index_Condition *cond);

    /// 対象ドキュメントが既に格納されているか診断する
    virtual bool contains(const char *doc_key);
    /// ドキュメントが検索に出現しないようにする（内部的には削除フラグが設定される）
    virtual bool delete_index(const char *doc_key);
    /// シンボルの利用を開始する
    virtual Symbol *open_symbol(const char *symbol_path, int flags = 0);
    /// シンボルファイルを開放する
    virtual bool drop_symbol(const char *symbol_path);
    /// テキストを先頭から切り出して返す
    virtual string snippet(const char *text, size_t snippet_length, bool tag_escape = false);
    /// 対象キーワードを切り出して返す
    virtual string snippet(const char *text, size_t snipet_length,
			   const vector<string> &words, const char *hilight_class = 0, bool tag_escape = false);
  };

  // --------------------------------------------------------------------------------

  Senna_Index::Value::Value(sen_values *_value, Senna_Index *_ref)
    : value(_value), ref(_ref) { }

  Senna_Index::Value::Value(Senna_Index *_ref)
    : value(sen_values_open()), ref(_ref) { }

  Senna_Index::Value::Value(const char *text, unsigned weight, Senna_Index *_ref)
    : value(sen_values_open()), ref(_ref)
  {
    add(text,weight);
  }

  Senna_Index::Value::Value(const string &text, unsigned weight, Senna_Index *_ref)
    : value(sen_values_open()), ref(_ref)
  {
    add(text,weight);
  }

  Senna_Index::Value::~Value() {
    if (!value) return;
    sen_rc rc = sen_values_close(value);
    if (rc != sen_success && ref) ref->elog("sen_values_close: (%d):%s\n", rc, get_error_text(rc));
  }

  bool Senna_Index::Value::add(const char *text, unsigned weight) {
    sen_rc rc = sen_values_add(value, text, strlen(text), weight);
    if (rc != sen_success && ref) ref->elog("sen_values_add: (%d):%s\n", rc, get_error_text(rc));
    return rc == sen_success;
  }

  bool Senna_Index::Value::add(const string &text, unsigned weight) {
    sen_rc rc = sen_values_add(value, text.data(), text.size(), weight);
    if (rc != sen_success && ref) ref->elog("sen_values_add: (%d):%s\n", rc, get_error_text(rc));
    return rc == sen_success;
  }

  // --------------------------------------------------------------------------------

  Senna_Index::Result::Result(sen_records *_rec, Senna_Index *_ref)
    : rec(_rec), ref(_ref) { }

  Senna_Index::Result::~Result() {
    if (!rec) return;
    sen_rc rc = sen_records_close(rec);
    if (rc != sen_success && ref) ref->elog("sen_records_close: (%d):%s\n", rc, get_error_text(rc));
  }

  /// 結果を次に進め、ドキュメントのIDとスコア値を入手する
  string Senna_Index::Result::next(int *score) {
    char tbuf[120];
    int klen = sen_records_next(rec, tbuf,sizeof tbuf, score);
    if (!klen) return "";
    if (klen < sizeof tbuf) return string(tbuf, klen);

    char kbuf[klen + 1];
    return string(kbuf, sen_records_curr_key(rec, kbuf, klen));
  }

  /// ドキュメントのIDを入手する
  string Senna_Index::Result::current_key() {
    char tbuf[120];
    int klen = sen_records_curr_key(rec, tbuf, sizeof tbuf);
    if (klen < sizeof tbuf) return string(tbuf, klen);
    char kbuf[klen + 1];
    return string(kbuf, sen_records_curr_key(rec, kbuf, klen));
  }

  /// スコア値を入手する
  int Senna_Index::Result::current_score() {
    int score = sen_records_curr_score(rec);
    return score;
  }

  /// 含まれるレコードの数を返す
  int Senna_Index::Result::nhits() {
    int nhits = sen_records_nhits(rec);
    return nhits;
  }

  /// 対応するレコードが存在すれば、そのレコードのスコアを返す。なければ 0。
  int Senna_Index::Result::find(const char *doc_key) {
    int score = sen_records_find(rec, doc_key);
    return score;
  }

  /// 結果ドキュメントを最初から入手する
  bool Senna_Index::Result::rewind() {
    sen_rc rc = sen_records_rewind(rec);
    if (rc != sen_success && ref) ref->elog("sen_records_rewind: (%d):%s\n", rc, get_error_text(rc));
    return rc == sen_success;
  }

  // --------------------------------------------------------------------------------

  /* Sennaの終了コードをテキストに変換
   */
  const char *Senna_Index::get_error_text(sen_rc rc) {
    switch(rc) {
    case sen_success: return "no error";
    case sen_memory_exhausted: return "memory error";
    case sen_file_operation_error: return "file operation error";
    case sen_invalid_format: return "invalid format";
    case sen_invalid_argument: return "invalid argument";
    case sen_external_error: return "extenal error";
    case sen_internal_error: return "internal error";
    case sen_abnormal_error: return "abnormal error";
    case sen_other_error: return "error";
    }
    return "unkown error";
  }

  sen_encoding Senna_Index::get_sen_encoding(const char *t) {
    /// エンコーディング指定コードの文字列対応テーブル
    static struct senc {
      sen_encoding enc; string label;
      senc(sen_encoding a,char *t) : enc(a), label(t) { }
    } enctbl[] = {
      senc(sen_enc_default, "default"),
      senc(sen_enc_none, "none"),
      senc(sen_enc_euc_jp, "euc"),
      senc(sen_enc_utf8, "utf8"),
      senc(sen_enc_sjis, "sjis"),
    };

    if (!t) return sen_enc_utf8;
    for (size_t i = 0; i < sizeof enctbl/sizeof enctbl[0]; i++) {
      if (enctbl[i].label == t) return enctbl[i].enc;
    }
    return sen_enc_utf8;
  }

  // フラグ文字列を内部表現に換えて返す
  int Senna_Index::get_sen_flag(char *t) {
    /// インデックス生成オプション・コードの文字列対応テーブル
    static struct sflag {
      int flag; string label;
      sflag(int a,char *t) : flag(a), label(t) { }
    } flagtbl[] = {
      sflag(SEN_INDEX_NORMALIZE, "normal"),
      sflag(SEN_INDEX_SPLIT_ALPHA, "alpha"),
      sflag(SEN_INDEX_SPLIT_DIGIT, "digit"),
      sflag(SEN_INDEX_SPLIT_SYMBOL, "symbol"),
      sflag(SEN_INDEX_NGRAM, "ngram"),
      sflag(SEN_INDEX_DELIMITED, "delimited"),
    };
    /*
      SEN_INDEX_NORMALIZE
      英文字の大文字/小文字、全角文字/半角文字を正規化してインデックスに登録する
      SEN_INDEX_SPLIT_ALPHA
      英文字列を文字要素に分割する
      SEN_INDEX_SPLIT_DIGIT
      数字文字列を文字要素に分割する
      SEN_INDEX_SPLIT_SYMBOL
      記号文字列を文字要素に分割する
      SEN_INDEX_NGRAM
      (形態素解析ではなく)n-gramを用いる
      SEN_INDEX_DELIMITED
      (形態素解析ではなく)空白区切りllで単語を区切る
    */

    if (!t) return SEN_INDEX_NORMALIZE;

    for (size_t i = 0; i < sizeof flagtbl/sizeof flagtbl[0]; i++) {
      if (flagtbl[i].label == t) return flagtbl[i].flag;
    }
    return SEN_INDEX_NORMALIZE;
  }

  /// インデックスの領域を解放する
  void Senna_Index::close_index() {
    if (!index) return;

    sen_rc rc = sen_index_close(index);
    if (rc != sen_success)
      elog(W, "sen_sym_close failur: (%d):%s\n",rc,get_error_text(rc));
  }

  /// インデックス・ファイルの存在チェック
  bool Senna_Index::test_index(const char *index_path) {
    return false;
  }

  /// インデックスの利用を開始する
  /*
    インデックスがなければデフォルト値で作成する。
    開いていれば特に処理しない
   */
  bool Senna_Index::open_index(const char *index_path, int flags) {
    init();
    sen_index *idx = sen_index_open(index_path);
    if (idx) {
      if (index) {
	close_index();
      }
      elog(I, "index %s opened\n",index_path);
      this->index = idx;
      this->index_path = index_path;
      return true;
    }

    int key_size = 0; // 0は可変長キーサイズを意味する
    int initial_n_segments = 500;
    sen_encoding enc = sen_enc_utf8;
    if (!flags) flags = SEN_INDEX_NORMALIZE;

    idx = sen_index_create(index_path, key_size, flags, initial_n_segments, enc);
    if (!idx) {
      elog("index %s create failur\n",index_path);
      return false;
    }
    this->index = idx;
    this->index_path = index_path;
    return true;
  }

  // シンボルを指定して利用を開始する
  /*
    このシンボルは、ドキュメントIDを格納したシンボルである。
    つまり、同じドキュメントIDで
    異なる本文データを保存したいときに利用できる。
   */
  bool Senna_Index::open_index_with_symbol(const char *index_path, sen_sym *sym, int flags) {
    init();
    sen_index *idx = sen_index_open_with_keys(index_path, sym);
    if (idx) {
      if (index) {
	close_index();
      }
      elog(I, "index %s opened\n",index_path);
      this->index = idx;
      this->index_path = index_path;
      return true;
    }

    int key_size = 0; // 0は可変長キーサイズを意味する
    if (flags == 0) flags = SEN_INDEX_NORMALIZE;
    int initial_n_segments = 500;
    sen_encoding enc = sen_enc_utf8;

    idx = sen_index_create_with_keys(index_path, sym, flags, initial_n_segments, enc);
    if (!idx) {
      elog("index %s create failur\n",index_path);
      return false;
    }
    this->index = idx;
    this->index_path = index_path;
    return true;
  }

  // インデックスを更新する
  /**
     新規登録の場合は oldvalue をNULLにする。
     削除の場合は newvalue をNULLにする。
     更新の場合でも古いテキストを渡す必要がある。
  */
  bool Senna_Index::update_index(const char *doc_key,
				 const string *new_value, const string *old_value) {
    if (!index) {
      elog("index not opend.\n");
      return false;
    }

    const char *oldvalue = old_value ? old_value->data() : 0;
    size_t oldvalue_len = old_value ? old_value->size() : 0;

    // インデックスの登録・更新
    sen_rc rc =
      sen_index_upd(index, doc_key, oldvalue, oldvalue_len,
		    new_value->data(), new_value->size());

    if (rc == sen_success) return true;
    elog(W, "update failur: (%d):%s\n", rc, get_error_text(rc));
    return false;
  }

  // インデックスを更新する
  /**
     新規登録の場合は oldvalue をNULLにする。
     削除の場合は newvalue をNULLにする。
     更新の場合でも古いテキストを渡す必要がある。
  */
  bool Senna_Index::update_index(const char *doc_key, int section,
				 const Value *new_value, const Value *old_value) {
    assert(new_value != NULL);

    if (!index) {
      elog("index not opend.\n");
      return false;
    }

    // インデックスの登録・更新
    sen_rc rc =
      sen_index_update(index, doc_key, section, old_value ? old_value->value : 0, new_value->value);
    if (rc == sen_success) return true;
    elog(W, "update failur: (%d):%s\n", rc, get_error_text(rc));
    return false;
  }

  static int simple_weight[] = { 1, };

  static sen_select_optarg ZERO_OPTARG;

  /**
   * aa & (words1 & words2 ...) または、aa & !(words1 & words2 ...) を検索する
   */
  sen_records *Senna_Index::and_records(const vector<string>& words, sen_records* aa,
					bool partial, bool not_flag, int *weight, size_t weight_len)
  {
    if (words.empty()) return 0;
    if (!weight) { weight = simple_weight; weight_len = 1; }

    bool first_rec = false;
    if (!aa) {
      if (not_flag) return 0;
      aa = sen_records_open(sen_rec_document, sen_rec_none, 0);
      if (!aa) return 0;
      aa->ignore_deleted_records = 1;
      first_rec = true;
    }

    sen_select_optarg opts = ZERO_OPTARG;
    opts.weight_vector = weight;
    opts.vector_size = weight_len;

    vector<string>::const_iterator it = words.begin();

    for (; it != words.end(); it++) {
        const char *word = it->c_str();
        size_t word_len = it->length();

        // " 対策
        if (word[0] == '"' && word[word_len - 1] == '"') {
            opts.mode = sen_sel_unsplit;
            word++;
            word_len -= 2;
        }
	else {
	  opts.mode = partial ? sen_sel_partial : sen_sel_exact;
	}

        sen_sel_operator op = it == words.begin() && first_rec ? sen_sel_or :
	  not_flag ? sen_sel_but : sen_sel_and;

        sen_rc rc = sen_index_select(index, word, word_len, aa, op, &opts);
        if (rc != sen_success) {
	  elog("select failed : %d\n", rc, get_error_text(rc));
	  break;
	}
    }

    return aa;
  }

  /**
   * aa & (words1 | words2 | words3 ...) の処理を行う。
   * 式展開は行わず、単純に計算する
   */
  sen_records *Senna_Index::or_records(const vector<string>& words, sen_records* aa,
				       bool partial, bool not_flag, int *weight, size_t weight_len)
  {
    if (words.empty()) return 0;
    if (!weight) { weight = simple_weight; weight_len = 1; }

    sen_records* result = sen_records_open(sen_rec_document, sen_rec_none, 0);
    if (!result) return 0;
    result->ignore_deleted_records = 1;

    sen_select_optarg opts = ZERO_OPTARG;
    opts.weight_vector = weight;
    opts.vector_size = weight_len;

    vector<string>::const_iterator it = words.begin();
    for ( ; it != words.end(); it++) {
        // " 対策
        const char* word = it->c_str();
        int word_len = it->length();

        if (word[0] == '"' && word[word_len - 1] == '"') {
            opts.mode = sen_sel_unsplit;
            word++;
            word_len -= 2;
        }
	else {
	  opts.mode = partial ? sen_sel_partial : sen_sel_exact;
	}

        sen_rc rc = sen_index_select(index, word, word_len, result, sen_sel_or, &opts);
        if (rc != sen_success) {
	  elog("select failed : %d\n", rc, get_error_text(rc));
	  break;
        }
    }

    if (aa && sen_records_nhits(aa) > 0) {
      aa = sen_records_intersect(aa, result);
    }
    else if (!aa)
      aa = result;

    return aa;
  }

  /// 単純検索
  Senna_Index::Result *Senna_Index::search(const char *text) {
    if (!index) {
      elog(W, "search called but, index not opend.\n");
      return 0;
    }
    string tbuf;

    rec.time_load();
    sen_records *res = sen_index_sel(index, text, strlen(text));

    rec.time_report("sen_index_sel ", tbuf);
    elog(T, "%s", tbuf.c_str());

    Result *res0 = new Result(res,this);
    return res0;
  }

  /// 検索
  Senna_Index::Result *Senna_Index::search(const uc::Index_Condition *cond) {
    if (!index) {
      elog(W, "search called but, index not opend.\n");
      return 0;
    }
    string tbuf;
    rec.time_load();

    // 1. まず、andKeyを処理する
    sen_records *and_res = 0;

    if (!cond->and_key.empty()) {
      and_res = and_records(cond->and_key, 0);
      if (sen_records_nhits(and_res) == 0) { return 0; }
    }

    // 2. 次にnotKeyを処理する
    sen_records *not_res = 0;

    if (!cond->not_key.empty()) {
      not_res = or_records(cond->not_key, 0);
    }

    // (a & b & c...) & !(x or y or z) を実行する
    sen_records *res = 0;
    if (and_res && not_res) {
      res = sen_records_subtract(and_res, not_res);
      if (sen_records_nhits(res) == 0) { return 0; }
      not_res = 0;
    }
    else
      res = and_res;

    // searchWordを処理する(orKeyと同じ処理をしている)

    if (!cond->search_word.empty()) {
      res = or_records(cond->search_word, res);

      if (res && not_res) {
	res = sen_records_subtract(res, not_res);
	not_res = NULL;
      }
      if (sen_records_nhits(res) == 0) { return 0; }
    }

    // orKeyを処理する
    if (!cond->or_key.empty()) {
      res = or_records(cond->or_key, res);
      if (res && not_res) {
	res = sen_records_subtract(res, not_res);
	not_res = NULL;
      }
      if (sen_records_nhits(res) == 0) return 0;
    }

    if (!cond->or_key2.empty()) {
      res = or_records(cond->or_key2, res);
      if (res && not_res) {
	res = sen_records_subtract(res, not_res);
	not_res = NULL;
      }
      if (sen_records_nhits(res) == 0) return 0;
    }

    if (!cond->or_key3.empty()) {
      res = or_records(cond->or_key3, res);
      if (res && not_res) {
	res = sen_records_subtract(res, not_res);
	not_res = NULL;
      }
      if (sen_records_nhits(res) == 0) return 0;
    }

    rec.time_report("search (with-condition) ", tbuf);
    elog(T, "%s", tbuf.c_str());

    Result *res0 = new Result(res, this);
    return res0;
  }


  bool Senna_Index::contains(const char *doc_key) {
    if (!index) return false;
    sen_id id = sen_sym_at(index->keys, doc_key);
    return id != SEN_SYM_NIL;
  }

  /// ドキュメントが検索に出現しないようにする（内部的には削除フラグが設定される）
  bool Senna_Index::delete_index(const char *doc_key) {
    sen_rc rc = sen_index_del(index, doc_key);
    if (rc != sen_success) elog(W, "index delete failed: %s\n", doc_key);
    return rc == sen_success;
  }

  /// インデックスの名称を変える
  bool Senna_Index::rename_index(const char *old_path, const char *new_path) {
    sen_rc rc = sen_index_rename(old_path, new_path);
    if (rc == sen_success)
      elog(I, "index renamed: %s to %s\n", old_path, new_path);
    else
      elog(W, "index rename failed: %s: (%d):%s\n", old_path, rc, get_error_text(rc));
    return rc == sen_success;
  }

  /// インデックスを削除する
  bool Senna_Index::drop_index(const char *index_path) {
    sen_rc rc = sen_index_remove(index_path);
    if (rc == sen_success)
      elog(I, "index droped: %s\n",index_path);
    else
      elog(W, "index drop failed: %s: (%d):%s\n",index_path, rc, get_error_text(rc));
    return rc == sen_success;
  }

  /// インデックスの基本情報を出力する
  void Senna_Index::show_index_info(FILE *fout) {
    if (!index) return;

    int key_size, flags, initial_n_segments;
    sen_encoding encoding;

    unsigned nrecords_keys, file_size_keys, nrecords_lexicon, file_size_lexicon;
    unsigned long long inv_seg_size, inv_chunk_size;

    sen_rc rc =
      sen_index_info(index, &key_size, &flags,
		     &initial_n_segments, &encoding,
		     &nrecords_keys, &file_size_keys,
		     &nrecords_lexicon, &file_size_lexicon,
		     &inv_seg_size, &inv_chunk_size);

    if (rc != sen_success) {
      elog(W, "sen_index_info failur: (%d):%s", rc, get_error_text(rc));
      return;
    }

    fprintf(fout, "Senna Index: %s.SEN* -------------\n",index_path.c_str());
    fprintf(fout, "key_size: %d  .. 0 means auto.\n"
	    "flags: %d (%#x)\n"
	    "initial_n_segments: %d\n"
	    "encoding: %d\n"
	    "nrecords_keys: %u\n"
	    "file_size_keys: %u\n"
	    "nrecords_lexicon: %u\n"
	    "file_size_lexicon: %u\n"
	    "inv_seg_size: %lu\n"
	    "inv_chunk_size: %lu\n",
	    key_size, flags, flags, initial_n_segments, encoding,
	    nrecords_keys, file_size_keys, nrecords_lexicon,
	    file_size_lexicon, inv_seg_size, inv_chunk_size);
  }


  // シンボル・ファイルの利用開始
  Senna_Index::Symbol *Senna_Index::open_symbol(const char *sym_path, int flags) {
    init();
    sen_sym *sym = sen_sym_open(sym_path);
    if (!sym) {
      int key_size = 0; // 0は可変長キーサイズを意味する
      if (flags == 0) flags = SEN_INDEX_NORMALIZE;
      sen_encoding enc = sen_enc_utf8;
      sym = sen_sym_create(sym_path, key_size, flags, enc);

      if (!sym) {
	elog("symbol open/create failur: %s", sym_path);
	return 0;
      }
    }
    Symbol *so = new Symbol(sym, this);
    return so;
  }

  bool Senna_Index::drop_symbol(const char *symbol_path) {
    return false;
  }

  // --------------------------------------------------------------------------------

  Senna_Index::Symbol::Symbol(sen_sym *_sym, Senna_Index *_ref)
    : sym(_sym), ref(_ref) { }

  Senna_Index::Symbol::~Symbol() { }

  // シンボル・ファイルの利用終了
  bool Senna_Index::Symbol::close() {
    if (!sym) return false;
    sen_rc rc = sen_sym_close(sym);
    sym = 0;
    if (rc == sen_success) return true;
    if (ref) ref->elog("symbol close failur: (%d):%s", rc, get_error_text(rc));
    return false;
  }

  /// シンボルの概要レポートを出力する
  void Senna_Index::Symbol::show_info(FILE *fout) {
    int key_size;
    unsigned flags;
    sen_encoding encoding;
    unsigned nrecords, file_size;

    // シンボル・ファイルの基本情報を入手
    sen_rc rc = sen_sym_info(sym, &key_size, &flags, &encoding, &nrecords, &file_size);
    if (rc != sen_success) {
      if (ref) ref->elog(W, "sen_sym_info failur: (%d):%s", rc, get_error_text(rc));
      return;
    }

    fprintf(fout, "key_size: %d\n"
	    "flags:%d\n"
	    "encoding: %d\n"
	    "nrecords: %u\n"
	    "file_size: %u\n"
	    "sym size: %u\n",
	    key_size, flags, encoding,
	    nrecords, file_size,sen_sym_size(sym));
  }

  /// シンボルファイルに登録されているキーの数を返す。
  int Senna_Index::Symbol::get_symbol_size() {
    return sen_sym_size(sym);
  }

  /// シンボルを登録する
  sen_id Senna_Index::Symbol::store_symbol(const char *key) {
    return sen_sym_get(sym, (const unsigned char *)key);
  }

  /// シンボルを削除する
  bool Senna_Index::Symbol::remove_symbol(const char *key) {
    sen_rc rc = sen_sym_del(sym, (const unsigned char *)key);
    if (rc != sen_success && ref)
      ref->elog("symbol %s remove failur: (%d):%s", key, rc, get_error_text(rc));
    return rc == sen_success;
  }

  /// シンボルIDを入手する
  // 未登録であった場合は SEN_SYM_NIL を返します。
  sen_id Senna_Index::Symbol::id_of(const char *key) {
    return sen_sym_at(sym, (const unsigned char *)key);
  }

  /// シンボル・テキストを入手する
  string Senna_Index::Symbol::key_of(sen_id id) {
    unsigned char tbuf[120];
    int klen = sen_sym_key(sym, id, tbuf, sizeof tbuf -1);
    if (klen == 0) return "";
    if (klen < sizeof tbuf) return string((char *)tbuf, klen);

    unsigned char kbuf[klen + 1];
    return string((char *)kbuf, sen_sym_key(sym, id, kbuf, klen));
  }

  /// 次のシンボルIDを得る
  sen_id Senna_Index::Symbol::next_id_of(sen_id id) {
    return sen_sym_next(sym, id);
  }

  // --------------------------------------------------------------------------------

  static int senna_initialized;

  /// Sennaライブラリの利用を開始する
  void Senna_Index::init() {
    if (senna_initialized) return;
    /*
      何度呼び出してもガードするようになっている
     */
    sen_rc rc = sen_init();
    if (rc == sen_success)
      senna_initialized = 1;
    else
      ::elog("sen_init: (%d):%s\n", rc, get_error_text(rc));
  }

  /// Sennaライブラリの利用を終了
  void Senna_Index::finish() {
    if (!senna_initialized) return;
    sen_rc rc = sen_fin();
    if (rc == sen_success)
      senna_initialized = 0;
    else
      ::elog("sen_fin: (%d):%s\n", rc, get_error_text(rc));
  }

  // XMLタグのエスケープ処理
  static string xml_escape(const char *str) {
    string buf;

    for (const char* pt = str; *pt; pt++) {
      // XMLとして不正な文字を無視する
      if ((0x00 < *pt && *pt < 0x09) ||
	  (0x0a < *pt && *pt < 0x0d) ||
	  (0x0d < *pt && *pt < 0x20)) continue;

      if (*pt == '&') buf += "&amp;";
      else if (*pt == '<') buf += "&lt;";
      else if (*pt == '>') buf += "&gt;";
      else if (*pt == '"') buf += "&quot;";
      else if (*pt == '\'') buf += "&apos;";
      else buf += *pt;
    }
    return buf;
  }

  // Array of skip-bytes-per-initial character.
  static const char utf8_skip_data[256] = {
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,   // 00-0F
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,   // 10-1F
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,   // 20-2F
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,   // 30-3F
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,   // 40-4F
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,   // 50-5F
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,   // 60-6F
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,   // 70-7F
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,   // 80-8F
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,   // 90-9F
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,   // A0-AF
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,   // B0-BF
    2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,   // C0-CF
    2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,   // D0-DF
    3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,   // E0-EF
    4,4,4,4,4,4,4,4,5,5,5,5,6,6,1,1    // F0-FF
  };

  /// テキストの先頭から 指定桁数だけテキストを切り出す
  string Senna_Index::snippet(const char *text, size_t snippet_length, bool tag_escape) {
    if(snippet_length == 0) {
      // 長さ無制限であるため、そのまま返す
      if (!tag_escape) return text;
      return xml_escape(text);
    }

    int len, nn;
    const char *pp;
    for(len = 0, pp = text; len < snippet_length && *pp != 0; ) {
      nn = utf8_skip_data[*(char *)(pp) & 0xFF];
      len += nn;
      pp += nn;
    }

    string sbuf(text, pp - text);
    if (sbuf.length() != strlen(text)) sbuf += "...";
    if (!tag_escape) return sbuf;

    return xml_escape(sbuf.c_str());
  }

  /// 要素中で一番長いアイテムを返す
  static const string &longest_item(const vector<string>& words) {
    vector<string>::const_iterator it = words.begin(), max = it;
    for (; it != words.end(); it++) {
      if ( (*it).length() > (*max).length()) max = it;
    }
    return *max;
  }

  /// 対象キーワードを切り出して返す
  string Senna_Index::snippet(const char *text, size_t snippet_length,
			   const vector<string> &words, const char *hilight_class, bool tag_escape)
  {
    if (words.empty()) {
      // 対象語がないと、snippetもhighlightもできない。
      // よって、文章の先頭からsnippetLength切り出す処理だけ行う
      return snippet(text, tag_escape, snippet_length);
    }

    string openTag, closeTag;

    if (hilight_class) {
      openTag = "<span class=\"";
      openTag.append(hilight_class).append("\">");
      closeTag = "</span>";
    }

    // 対象語よりもsnippet幅が小さい場合を考慮して補正する
    // 最低でも対象語の長さの2倍を保障する
    // snippetLengthが0、つまり、highlight処理だけを行う場合は、最大長に設定する。

    unsigned int snippet_width =
      std::max(snippet_length == 0 ? UINT_MAX : size_t(snippet_length),
	       longest_item(words).length()) * 2;

    int word_num = words.size() > 16 ? 16 : words.size();

    sen_snip* snip =
      sen_snip_open(sen_enc_utf8, SEN_SNIP_NORMALIZE, snippet_width, word_num,
		    openTag.c_str(), openTag.length(), closeTag.c_str(), closeTag.length(),
		    tag_escape ? (sen_snip_mapping*)-1 : NULL);
    if (!snip) {
      elog("sen_snip_open failur\n");
      return "";
    }

    int n = 0;
    vector<string>::const_iterator it = words.begin();

    for ( ; it != words.end() && n < word_num; it++, n++) {
      sen_rc rc = sen_snip_add_cond(snip, it->c_str(), it->length(), NULL, 0, NULL, 0);
      if (rc != sen_success) {
	elog("sen_snip_add_cond %s failed!:(%d):%s\n", it->c_str(), rc, get_error_text(rc));
	sen_snip_close(snip);
	return "";
      }
    }

    unsigned int nresults;
    unsigned int max_tagged_len;
    sen_rc rc = sen_snip_exec(snip, text, strlen(text), &nresults, &max_tagged_len);

    if (rc != sen_success) {
      elog("sen_snip_exec failed!:(%d):%s\n", rc, get_error_text(rc));
      sen_snip_close(snip);
      return "";
    }

    // snippetの生成に失敗した場合は、
    // 自前で長さを切る処理を行う(対象語が見つからなかった場合等)
    if (nresults == 0) {
      sen_snip_close(snip);
      return snippet(text, snippet_length, tag_escape);
    }

    string result;

    for (unsigned int i = 0; i < nresults; i++) {
      char *buf = new char[max_tagged_len];
      unsigned int result_len;

      rc = sen_snip_get_result(snip, i, buf, &result_len);
      if (rc != sen_success) {
	elog("sen_snip_get_result failed:(%d):%s\n", rc, get_error_text(rc));
	delete [] buf;
      }

      result.append(buf);
      delete [] buf;
    }

    sen_snip_close(snip);

    return result;
  }

  // senna を使った全文検の実装
  /*
    senna APIを直接的に呼び出すのはこのクラスとその従属クラスになる。
    この実装はインデックセット名に、時系列（処理開始日時）を付けた
    インデックス・ファイル名を作成する。

    複数のsennaファイルを切り替えながら扱う。

    インデックスの更新が完了したら、最新の時系列のファイルが利用されることになるが
    過去の状態を検索したい場合は、古いものが対照となる。
    期限切れの処理により、古いインデックスを削除対象となる。

    メタデータは同フォルダの、BDBに格納する。
    ドキュメントIDは、基準ディレクトリからのファイル相対パスとする。
    全文検索の対象となるテキスト・データは、あらかじめ
    更新処理フォルダに集約され、インデックスが作成されたタイミングでzipでアーカイブする。
    ディレクトリ構造はそのまま維持された状態でzipされる。
    前のデータは前の世代からそのまま転記される。
    最初はzipデータに格納できない容量でもってツールの限界とする。
    ※次の世代で分割zipを検討する。

    最初の実装はいわゆるテキスト・ファイルだけを対象とする。
    収集のタイミングで文字コードは統一する。
    処理できなかったファイルは、BDBに記録してユーザに確認を求める。

   */
  class Senna_Index_Scanner : public uc::Index_Scanner, uc::ELog {
    Index_Manager_Impl *manager;

  public:
    Senna_Index_Scanner(const char *name, Index_Manager_Impl *manager);
    virtual ~Senna_Index_Scanner();

    /// インデックスに格納されているドキュメント総数を入手
    virtual long get_document_count();
    /// インデックスの最終更新日時を入手
    virtual long get_last_update();
    /// インデックスの更新状況の確認
    virtual string get_scan_state();
    /// ドキュメントの走査開始
    virtual void begin_scan_document();
    /// 次の対象ドキュメントのドキュメントIDの入手
    virtual bool fetch_next_document(string doc_id);
    /// インデックスの走査終了
    virtual void end_scan_document();
    /// 指示するドキュメントが保持するセクション情報を入手
    virtual string fetch_document_sections(const char *doc_id, vector<string> &list);
    /// ドキュメントIDに対応するセクション情報を入手
    virtual string fetch_document(const char *doc_id, const char *section);
    /// インデックスの更新
    virtual void update_index(string doc_id);

    /// インデックスに対して検索処理を行う
    virtual uc::Index_Documents *query(uc::Index_Condition &condition);
    /// インデックスに対して検索処理をする(件数確認のみ)
    virtual long count_query(uc::Index_Condition &condition);
  };

  class Index_Manager_Impl : public uc::Index_Manager, uc::ELog {
  public:
    Index_Manager_Impl();
    virtual ~Index_Manager_Impl() {}
    virtual bool set_index_directory(const char *dir);
    /// インデックの一覧を入手する
    virtual void get_index_list(vector<string> &list);
    /// インデックの基本情報を確認する
    virtual void show_index_info(FILE *out, const char *index_name);
    /// 特定のディレクトリをインデックの管理下に置く
    virtual bool crete_index(const char *name, const char *dir_path);
    /// インデックを更新する
    virtual void update_index(const char *name);
    /// インデックスを廃止する
    virtual bool drop_index(const char *name);
    /// 走査制御クラスの入手
    virtual uc::Index_Scanner *get_scanner(const char *name);

    /// 保持しているセクション名の入手
    /*
      0:title, 1:body, 2:author, 3:year, 4:month, 5:week, 6:doctype, 7:recipients
      それ以外に何が入るかは、ドキュメント属性による
    */
    virtual void get_section_list(const char *name, vector<string> &list);
    /// インデックスに対して検索処理を行う
    virtual uc::Index_Documents *query(const char *name, uc::Index_Condition &condition);
    /// インデックスに対して検索処理をする(件数確認のみ)
    virtual long count_query(const char *name, uc::Index_Condition &condition);
    /// ドキュメントIDに対応するセクション情報を入手
    virtual string fetch_document(const char *name, const char *doc_id, const char *section = 0);
  };

  // --------------------------------------------------------------------------------

  Senna_Index_Scanner::Senna_Index_Scanner(const char *name, Index_Manager_Impl *mgr)
    : Index_Scanner(name), manager(mgr) { }

  Senna_Index_Scanner::~Senna_Index_Scanner() { }

  /// インデックスに格納されているドキュメント総数を入手
  long Senna_Index_Scanner::get_document_count() {
    return 0;
  }

  /// インデックスの最終更新日時を入手
  long Senna_Index_Scanner::get_last_update() {
    return 0;
  }

  /// インデックスの更新状況の確認
  string Senna_Index_Scanner::get_scan_state() {
    return "";
  }

  /// ドキュメントの走査開始
  void Senna_Index_Scanner::begin_scan_document() {
  }

  /// 次の対象ドキュメントのドキュメントIDの入手
  bool Senna_Index_Scanner::fetch_next_document(string doc_id) {
    return false;
  }

  /// インデックスの走査終了
  void Senna_Index_Scanner::end_scan_document() {
  }

  /// 指示するドキュメントが保持するセクション情報を入手
  string Senna_Index_Scanner::fetch_document_sections(const char *doc_id, vector<string> &list) {
    return "";
  }

  /// ドキュメントIDに対応するセクション情報を入手
  string Senna_Index_Scanner::fetch_document(const char *doc_id, const char *section) {
    return "";
  }

  /// インデックスの更新
  void Senna_Index_Scanner::update_index(string doc_id) {
  }

  /// インデックスに対して検索処理を行う
  uc::Index_Documents *Senna_Index_Scanner::query(uc::Index_Condition &condition) {
    return 0;
  }

  /// インデックスに対して検索処理をする(件数確認のみ)
  long Senna_Index_Scanner::count_query(uc::Index_Condition &condition) {
    return 0;
  }

  // --------------------------------------------------------------------------------

  Index_Manager_Impl::Index_Manager_Impl() {}

  bool Index_Manager_Impl::set_index_directory(const char *dir) {
    return false;
  }

  /// インデックの一覧を入手する
  void Index_Manager_Impl::get_index_list(vector<string> &list) {
  }

  void Index_Manager_Impl::show_index_info(FILE *out, const char *index_name) {
  }

  /// 特定のディレクトリをインデックの管理下に置く
  bool Index_Manager_Impl::crete_index(const char *name, const char *dir_path) {
  }

  /// インデックを更新する
  void Index_Manager_Impl::update_index(const char *name) {
  }

  /// インデックスを廃止する
  bool Index_Manager_Impl::drop_index(const char *name) {
  }

  /// 走査制御クラスの入手
  uc::Index_Scanner *Index_Manager_Impl::get_scanner(const char *name) {
    return 0;
  }

  /// 保持しているセクション名の入手
  /*
    0:title, 1:body, 2:author, 3:year, 4:month, 5:week, 6:doctype, 7:recipients
    それ以外に何が入るかは、ドキュメント属性による
  */
  void Index_Manager_Impl::get_section_list(const char *name, vector<string> &list) {
  }

  /// インデックスに対して検索処理を行う
  uc::Index_Documents *Index_Manager_Impl::query(const char *name, uc::Index_Condition &condition) {
    return 0;
  }

  /// インデックスに対して検索処理をする(件数確認のみ)
  long Index_Manager_Impl::count_query(const char *name, uc::Index_Condition &condition) {
    return 0;
  }

  /// ドキュメントIDに対応するセクション情報を入手
  string Index_Manager_Impl::fetch_document(const char *name, const char *doc_id, const char *section) {
    return "";
  }

};

// --------------------------------------------------------------------------------

using namespace idx;

namespace uc {

  Index_Scanner::~Index_Scanner() { }

  Index_Manager::~Index_Manager() {}

  Index_Manager *Index_Manager::create_index_manager() {
    return new Index_Manager_Impl();
  }

};

// --------------------------------------------------------------------------------

#include <iostream>
#include <memory>

#include "uc/all.hpp"


extern "C" uc::Local_Text_Source *create_Local_Text_Source();
extern "C" char *trim(char *t);

namespace idx {

  /// Index_Managerの基本機能を確認するツール
  /*
    検索対象の登録と、検索を行う基本機能を定義する。
    主にカレント・インデックスに対して処理するものと、
    複合インデックス（検索グループ）について処理するものに分かれる。
    検索は後者に対して行う。
   */
  class Index_tool {
  public:
    uc::Index_Manager *im;

    Index_tool() : im(uc::Index_Manager::create_index_manager()) { }
    ~Index_tool() { delete im; }

    /// 登録済みインデックの確認
    int show_index_list();
    /// カレント・インデックの設定と確認
    int change_current_index(const char *index_name);
    /// インデック対象のフォルダを登録
    int register_index(const char *index_name, const char *path, const char *description);
    /// インデックと、関連する検索グループの破棄（復元不能）
    int drop_index(const char *index_name);
    /// カレント・インデックの即時更新を行う
    int update_index();
    /// カレント・インデックに格納されているドキュメント一覧を入手
    int show_documents_list(int offset, int limit);
    /// カレント・インデックから除くドキュメントを指定
    int ignore_documents(const char *did);
    /// 登録済み検索グループを確認する
    int show_search_groups();
    /// 検索グループを変更する
    int change_search_groups(const char *group_name);
    /// 検索グループを定義する
    int create_search_group(const char *group_name, const char *description);
    /// 検索グループに対象インデックスを追加する
    int add_search_index(const char *index_name);
    /// 検索グループを破棄する
    int drop_search_group(const char *group_name);
    /// カレント検索グループに対して検索を行う
    int search(int argc, char **argv);
    /// 検索履歴を確認
    int show_search_history();
    /// カレントグループで再検索を行う
    int history_search(int id);
  };


  /// 登録済みインデックの確認
  int Index_tool::show_index_list() {
    return 0;
  }

  /// カレント・インデックの設定と確認
  int Index_tool::change_current_index(const char *index_name) {
    return 0;
  }

  /// インデック対象のフォルダを登録
  int Index_tool::register_index(const char *index_name, const char *path, const char *description) {
    return 0;
  }

  /// インデックと、関連する検索グループの破棄（復元不能）
  int Index_tool::drop_index(const char *index_name) {
    return 0;
  }

  /// カレント・インデックの即時更新を行う
  int Index_tool::update_index() {
    return 0;
  }

  /// カレント・インデックに格納されているドキュメント一覧を入手
  int Index_tool::show_documents_list(int offset, int limit) {
    return 0;
  }

  /// カレント・インデックから除くドキュメントを指定
  int Index_tool::ignore_documents(const char *did) {
    return 0;
  }

  /// 登録済み検索グループを確認する
  int Index_tool::show_search_groups() {
    return 0;
  }

  /// 検索グループを変更する
  int Index_tool::change_search_groups(const char *group_name) {
    return 0;
  }

  /// 検索グループを定義する
  int Index_tool::create_search_group(const char *group_name, const char *description) {
    return 0;
  }

  /// 検索グループに対象インデックスを追加する
  int Index_tool::add_search_index(const char *index_name) {
    return 0;
  }

  /// 検索グループを破棄する
  int Index_tool::drop_search_group(const char *group_name) {
    return 0;
  }

  /// カレント検索グループに対して検索を行う
  int Index_tool::search(int argc, char **argv) {
    return 0;
  }

  /// 検索履歴を確認
  int Index_tool::show_search_history() {
    return 0;
  }

  /// カレントグループで再検索を行う
  int Index_tool::history_search(int id) {
    return 0;
  }

  /// Senna_Index の実装を確認するために直接操作するクラス

  class Senna_Tool : uc::ELog {
    Senna_Index *index; ///< そう他対象インスタンス（一つだけ）
    Senna_Index::Result *last_result;

    map<string, Senna_Index::Symbol *> syms;
    uc::Local_File *lf;
    char *text_buffer;

  protected:
    Senna_Index::Symbol *lookup_symbol(const char *name);

    /// 次の読み込みを行うまで有効
    const char *read_text_file(const char *file_path, size_t *len_return = 0);

  public:
    Senna_Tool();
    ~Senna_Tool();
    bool open(const char *index_path);
    void close();
    void show_info(FILE *fout);
    bool rename(const char *old_path, const char *new_path);
    bool drop(const char *index_path);

    bool add_document(const char *file_path);
    bool simple_search(const char *text);
    string next_path(int *score);
    int result_count();

    string open_symbol(const char *symbol_path);

    void show_symbol_info(const char *name, FILE *fout);
    bool close_symbol(const char *name);
    void close_all_symbols();

    bool store_symbols(const char *name, const char * const *symbols, size_t len, map<string, sen_id> *syms);
    int fetch_symbols(const char *name, sen_id *ids, size_t id_len, map<sen_id,string> &syms);
  };

  /// シンボル情報を登録する
  bool Senna_Tool::store_symbols(const char *name, const char * const *symbols, size_t len, map<string, sen_id> *syms) {
    Senna_Index::Symbol *sym = lookup_symbol(name);
    if (!sym) return false;

    for (const char * const *key = symbols; key < symbols + len; key++) {
      sen_id id = sym->store_symbol(*key);
      if (syms) syms->insert(map<string, sen_id>::value_type(*key, id));
    }
    return true;
  }

  /// シンボル情報を読み取る
  int Senna_Tool::fetch_symbols(const char *name, sen_id *ids, size_t id_len, map<sen_id,string> &syms) {
    Senna_Index::Symbol *sym = lookup_symbol(name);
    if (!sym) return false;

    for (sen_id *id = ids; id < ids + id_len; id++) {
      syms.insert(map<sen_id, string>::value_type(*id, sym->key_of(*id)));
    }

  }

  Senna_Tool::Senna_Tool()
    : index(new Senna_Index()), last_result(0),
      lf(uc::create_Local_File()), text_buffer(0)
  {
    init_elog("Senna_Tool");
  }

  Senna_Tool::~Senna_Tool() {
    delete last_result;
    free(text_buffer);
    close();
    close_all_symbols();
  }

  const char *Senna_Tool::read_text_file(const char *file_path, size_t *len_return) {
    struct stat sbuf;

    int res = stat(file_path, &sbuf);
    if (res) {
      elog("stat %s faild:(%d):%s\n", file_path, errno, strerror(errno));
      return 0;
    }

    if (!S_ISREG(sbuf.st_mode)) {
      elog("%s not reguler file:(%d):%s\n", file_path, errno, strerror(errno));
      return 0;
    }

    FILE *fin;
    if ((fin = fopen(file_path, "r")) == NULL) {
      elog("fopen %s faild:(%d):%s\n", file_path, errno, strerror(errno));
      return 0;
    }

    int len = sbuf.st_size, n;
    char *buf = (char *)malloc(len + 10), *ptr = buf;

    long file_len = 0;

    while(len > 0 && (n = fread(ptr, 1, len, fin)) > 0) {
      ptr += n;
      len -= n;
      file_len += n;
    }

    if (fclose(fin) != 0) {
      elog(W, "fclose %s faild:(%d):%s\n",file_path,errno,strerror(errno));
    }

    if (len) {
      elog("cannot read all data:%s:%ld of %ld", file_path, file_len, sbuf.st_size);
      free(buf);
      buf = 0;
    }

    if (buf && len_return)
      *len_return = file_len;

    elog(I, "%s load %ld bytes.\n", file_path, file_len);

    return buf;
  }

  /// パスに基づいてドキュメントを登録
  /*
    この実装は登録済みのドキュメントはパスする
   */
  bool Senna_Tool::add_document(const char *file_path) {
    if (index->contains(file_path)) return false;
    const char *text = read_text_file(file_path);
    if (!text) return false;

    string vtext(text); free((char *)text);
    bool rc = index->update_index(file_path, &vtext);
    return rc;
  }

  /// パスに基づいてドキュメントを登録
  bool Senna_Tool::simple_search(const char *text) {
    if (last_result) { delete last_result; last_result = 0; }
    last_result = index->search(text);
    return last_result != 0;
  }

  /// 検索結果の件数を得る
  int Senna_Tool::result_count() {
    return last_result ? last_result->nhits() : 0;
  }

  /// 検索結果の対象パスを得る
  string Senna_Tool::next_path(int *score) {
    if (!last_result) return "";
    string res = last_result->next(score);
    if (res.empty()) { delete last_result; last_result = 0; }
    return res;
  }

  Senna_Index::Symbol *Senna_Tool::lookup_symbol(const char *name) {
    map<string, Senna_Index::Symbol *>::iterator it = syms.find(name);
    if (it != syms.end()) return it->second;
    elog("symbol %s not loaded.\n", name);
    return 0;
  }

  /// シンボル情報を出力する
  void Senna_Tool::show_symbol_info(const char *symbol_name, FILE *fout) {
    Senna_Index::Symbol *sym = lookup_symbol(symbol_name);
    if (!sym) return;

    fprintf(fout, "Symbol: %s :%p ------\n", symbol_name, sym);
    sym->show_info(fout);
  }

  /// シンボル情報の利用を開始する
  string Senna_Tool::open_symbol(const char *symbol_path) {
    Senna_Index::Symbol *sym = index->open_symbol(symbol_path);
    if (!sym) return "";

    string name = lf->basename(symbol_path);

    map<string, Senna_Index::Symbol *>::iterator it = syms.find(name);
    if (it != syms.end()) {
      /*
	以前の同名のシンボルを開放する
       */
      it->second->close();
      delete it->second;
      syms.erase(it);
    }
    syms.insert(map<string, Senna_Index::Symbol *>::value_type(name, sym));
    return name;
  }

  /// シンボル情報の利用を終了する
  bool Senna_Tool::close_symbol(const char *symbol) {
    if (!symbol || !*symbol) return false;

    string name = symbol;

    map<string, Senna_Index::Symbol *>::iterator it = syms.find(name);
    if (it == syms.end()) return false;

    int rc = it->second->close();
    delete it->second;
    syms.erase(it);
    return rc;
  }

  /// 全てのシンボル情報の利用を終了する
  void Senna_Tool::close_all_symbols() {
    map<string, Senna_Index::Symbol *>::iterator it = syms.begin();
    int ct = 0, err = 0;

    while (it != syms.end()) {
      if (!it->second->close()) err++;
      delete it->second;
      syms.erase(it++);
      ct++;
    }
    if (ct > 0) elog(I, "close %d symbol files. (error:%d)\n", ct, err);
  }

  /// 指定するSENNAインデックスの名称を変える。
  /**
     パスの移動はサポートしない。
     new_name は basenmae が利用されるのみ。
   */
  bool Senna_Tool::rename(const char *old_path, const char *new_name) {
    return index->rename_index(old_path, new_name);
  }

  /// 指定するSENNAインデックスを破棄する
  /**
     これで破棄したインデックスを復元することはできないので注意すること。
   */
  bool Senna_Tool::drop(const char *index_path) {
    return index->drop_index(index_path);
  }

  /// インデックスの利用を開始する
  /**
     指定するSENNAインデックスの利用を開始する。
     前回の閉じていないインデックス操作は開放する。
   */
  bool Senna_Tool::open(const char *index_path) {
    close();
    return index->open_index(index_path);
  }

  /// 開いているSENNAインデックスの基本情報を出力する
  void Senna_Tool::show_info(FILE *fout) {
    index->show_index_info(fout);
  }

  /// インデックスの利用を終了する
  void Senna_Tool::close() {
    index->close_index();
  }

};

// --------------------------------------------------------------------------------

extern "C" {

  enum ELog_Level { I = uc::ELog::I, W = uc::ELog::W, };

  /// インデックスを検索する
  /**
    事前にインデックスを作成しておく必要がある。
  */
  int senna_search(int argc, char **argv) {

    if (argc <= optind + 1) {
      fprintf(stderr,"usage: %s <index-file> [key]\n", argv[0]);
      return 1;
    }

    const char *idx_file = argv[optind];
    auto_ptr<Senna_Tool> st(new Senna_Tool());

    if (!st->open(idx_file)) return 2;
    st->show_info(stdout);

    int rc = 0;

    const char *key = argv[optind + 1];

    elog(I, "senna search: %s\n", key);
    if (!st->simple_search(key))
      rc = 2;
    else {
      string path;
      int score;
      elog(I, "%d recoreds matches.\n", st->result_count());

      while (!(path = st->next_path(&score)).empty()) {
	fprintf(stderr, "%s: %d\n", path.c_str(), score);
      }
    }
    return rc;
  }

  /// インデックスを作成する
  /// まだ登録していないファイルであれば追加する
  int senna_create_index(int argc, char **argv) {

    if (argc <= optind + 1) {
      fprintf(stderr,"usage: %s <index-file> [target-file] ..\n", argv[0]);
      return 1;
    }

    const char *idx_file = argv[optind];
    auto_ptr<Senna_Tool> st(new Senna_Tool());

    if (!st->open(idx_file)) return 2;

    int rc = 0;

    for (int i = optind + 1; i < argc; i++) {
      const char *arg = argv[i];
      if (!st->add_document(arg)) rc = 3;
    }

    st->show_info(stdout);

    return rc;
  }

  /// IDからシンボル・テキストを確認する
  int senna_sym_fetch(int argc, char **argv) {
    auto_ptr<Senna_Tool> st(new Senna_Tool());

    if (argc <= optind + 2) {
      fprintf(stderr,"usage: %s <sym-file> word ..\n", argv[0]);
      return 1;
    }

    const char *sym_file = argv[optind];
    string sym = st->open_symbol(sym_file);
    if (sym.empty()) return 1;

    int id_len = argc - optind - 1;
    sen_id *ids = (sen_id *)malloc(id_len * sizeof *ids), *id = ids;
    int idx = optind + 1;

    while (id < ids + id_len) {
      *id++ = atol(argv[idx++]);
    }

    map<sen_id, string> syms;
    st->fetch_symbols(sym.c_str(), ids, id_len, syms);

    map<sen_id, string>::const_iterator it = syms.begin();
    for (; it != syms.end(); it++) {
      fprintf(stderr,"%d: %s\n", (int)it->first, it->second.c_str());
    }

    free(ids);

    return 0;
  }

  int senna_sym_store(int argc, char **argv) {
    auto_ptr<Senna_Tool> st(new Senna_Tool());
    int rc = 0;

    if (argc <= optind + 2) {
      fprintf(stderr,"usage: %s <sym-file> word ..\n", argv[0]);
      return 1;
    }

    const char *sym_file = argv[optind];
    string sym = st->open_symbol(sym_file);
    if (sym.empty()) return 1;

    map<string, sen_id> syms;
    st->store_symbols(sym.c_str(), argv + optind + 1, argc - optind - 1, &syms);

    map<string, sen_id>::const_iterator it = syms.begin();
    for (; it != syms.end(); it++) {
      fprintf(stderr,"%d: %s\n", (int)it->second, it->first.c_str());
    }

    return 0;
  }

  int senna_sym_info(int argc, char **argv) {
    auto_ptr<Senna_Tool> st(new Senna_Tool());
    int rc = 0;
    for (int i = optind; i < argc; i++) {
      const char *arg = argv[i];
      string name = st->open_symbol(arg);
      if (name.empty()) { rc = 1; continue; }
      st->show_symbol_info(name.c_str(), stdout);
    }
    return rc;
  }

  int senna_info(int argc, char **argv) {
    auto_ptr<Senna_Tool> st(new Senna_Tool());
    int rc = 0;
    for (int i = optind; i < argc; i++) {
      const char *arg = argv[i];
      if (!st->open(arg)) { rc = 1; continue; }
      st->show_info(stdout);
    }
    return rc;
  }

  int senna_drop(int argc, char **argv) {
    auto_ptr<Senna_Tool> st(new Senna_Tool());
    int rc = 0;
    for (int i = optind; i < argc; i++) {
      const char *arg = argv[i];
      if (!st->drop(arg)) { rc = 1; continue; }
    }
    return rc;
  }

  int senna_rename(int argc, char **argv) {
    auto_ptr<Senna_Tool> st(new Senna_Tool());
    if (optind +2 < argc) {
      fprintf(stderr, "usage: usage %s <old> <new>\n", argv[0]);
      return 1;
    }
    return st->rename(argv[optind],argv[optind + 1]) ? 0 : 1;
  }

  /// Sennaのインデックスを構築する
  /*
   */
  int cmd_index01(int argc, char **argv) {
    auto_ptr<Index_tool> tool(new Index_tool);
    return 0;
  }

  /// Sennaのインデックスを検索する
  int cmd_search01(int argc, char **argv) {
    return 0;
  }

  
};

// --------------------------------------------------------------------------------


#ifdef USE_SUBCMD
#include "subcmd.h"

subcmd index_cmap[] = {
  { "senna", senna_search, },
  { "senna-create", senna_create_index, },
  { "senna-info", senna_info, },
  { "senna-rename", senna_rename, },
  { "senna-drop", senna_drop, },
  { "senna-sym-info", senna_sym_info, },
  { "senna-sym-store", senna_sym_store, },
  { "senna-sym-fetch", senna_sym_fetch, },

  { "index", cmd_index01, },
  { "search", cmd_search01, },
  { 0 },
};

#endif
