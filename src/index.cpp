/*! \file
 * \brief ドキュメント検索の基本操作を行うサンプル・コード
 */

#include <string>
#include <vector>
#include <map>

// --------------------------------------------------------------------------------

namespace uc {

  /// 検索条件を格納するDTO
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

#include <cerrno>
#include <cstdlib>
#include <dirent.h>
#include <fcntl.h>
#include <sys/types.h>

#include "uc/elog.hpp"
#include "uc/datetime.hpp"

#include "db.h"
#include "depot.h"
#include "senna/senna.h"

extern "C" int ends_with(const char *target, const char *suffix);

/// 実装クラスを隠す
namespace {

  // --------------------------------------------------------------------------------

  class Index_Manager_Impl;
  class Senna_Index_Scanner;

  // enum ELog_Level { I = uc::ELog::I, W = uc::ELog::W, };

  /// sennaインデックス・インスタンスと連動する Senna_Index_Scanner の従属クラス
  /*
    senna APIを直接的に呼び出す。
    論理的に同じドキュメントをインデックスで管理する。
    この実装は一つのsennaインデックスを操作するだけだが、
    拡張して従属情報も合わせて制御するサブクラスが作成されるかもしれない。
  */

  class Senna_Index : uc::ELog {
  protected:
    sen_index *index;
    string index_path;
    Senna_Index_Scanner *scanner;

  public:
    Senna_Index(Senna_Index_Scanner *sis)
      : scanner(sis)
    {
      init_elog("Senna_Index");
    }

    virtual ~Senna_Index() { close_index(); }

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
    /// インデックスの利用を開始する
    virtual bool open_index(const char *index_path, int flags = 0);
    /// シンボルを指定して利用を開始する
    virtual bool open_index_with_symbol(const char *index_path, sen_sym *sym, int flags = 0);
    /// インデックスの概要レポートを出力する
    virtual void show_index_info(FILE *fp);
    /// インデックスの利用を終了する
    virtual void close_index();
    /// インデックスを削除する
    virtual void remove_index(const char *index_path);
    /// インデックスを更新する
    virtual bool update_index(const char *doc_key,
		      const char *newvalue, size_t newvalue_len,
		      const char *oldvalue, size_t oldvalue_len);

    /// シンボルの利用を開始する
    virtual sen_sym *open_symbol(const char *symbol_path, int flags = 0);
    /// シンボルの利用を終了する
    virtual void close_symbol(sen_sym *sym);
    /// シンボルの概要レポートを出力する
    virtual void show_symbol_info(sen_sym *sym, FILE *fp);
  };


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
      elog(W, "sen_sym_close failur: %s\n",get_error_text(rc));
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
  bool Senna_Index::update_index(const char *doc_key, const char *newvalue, size_t newvalue_len,
				 const char *oldvalue = 0, size_t oldvalue_len = 0) {
    if (!index) {
      elog("index not opend.\n");
      return false;
    }

    // インデックスの登録・更新
    sen_rc rc =
      sen_index_upd(index, doc_key,
		    oldvalue, oldvalue_len,
		    newvalue, newvalue_len);

    if (rc == sen_success) return true;
    elog(W, "update failur: %s\n", get_error_text(rc));
  }


  /// インデックスを削除する
  void Senna_Index::remove_index(const char *index_path) {
    sen_rc rc = sen_index_remove(index_path);
    if (rc == sen_success)
      elog(I, "index removed: %s\n",index_path);
    else
      elog(W, "index remove failed: %s\n",index_path);
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
      elog(W, "sen_index_info failur: %s", get_error_text(rc));
      return;
    }

    fprintf(fout, "------- %s -------------\n",index_path.c_str());
    fprintf(fout, "key_size: %d\n"
	    "flags:%d\n"
	    "initial_n_segments:%d\n"
	    "encoding: %d\n"
	    "nrecords_keys: %u\n"
	    "file_size_keys: %u\n"
	    "nrecords_lexicon: %u\n"
	    "file_size_lexicon: %u\n"
	    "inv_seg_size: %lu\n"
	    "inv_chunk_size: %lu\n",
	    key_size, flags, initial_n_segments, encoding,
	    nrecords_keys, file_size_keys, nrecords_lexicon,
	    file_size_lexicon, inv_seg_size, inv_chunk_size);
  }


  // シンボル・ファイルの利用開始
  sen_sym *Senna_Index::open_symbol(const char *sym_path, int flags) {
    init();
    sen_sym *sym = sen_sym_open(sym_path);

    if (!sym) {
      int key_size = 0; // 0は可変長キーサイズを意味する
      if (flags == 0) flags = SEN_INDEX_NORMALIZE;
      sen_encoding enc = sen_enc_utf8;
      sym = sen_sym_create(sym_path, key_size, flags, enc);

      if (!sym) elog("symbol open/create failur: %s", sym_path);
    }
    return sym;
  }

  // シンボル・ファイルの利用終了
  void Senna_Index::close_symbol(sen_sym *sym) {
    if (sym) return;
    sen_rc rc = sen_sym_close(sym);
    if (rc == sen_success) return;
    elog("symbol close failur: %s", get_error_text(rc));
  }

  void Senna_Index::show_symbol_info(sen_sym *sym, FILE *fout) {
    int key_size;
    unsigned flags;
    sen_encoding encoding;
    unsigned nrecords, file_size;

    // シンボル・ファイルの基本情報を入手
    sen_rc rc = sen_sym_info(sym, &key_size, &flags, &encoding, &nrecords, &file_size);
    if (rc != sen_success) {
      elog(W, "sen_sym_info failur: %s", get_error_text(rc));
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

  static int senna_initialized;

  /// Sennaライブラリの利用を開始する
  void Senna_Index::init() {
    if (senna_initialized) return;
    sen_rc rc = sen_init();
    if (rc == sen_success)
      senna_initialized = 1;
    else
      fprintf(stderr, "ERROR: sen_init: %s\n", get_error_text(rc));
  }

  /// Sennaライブラリの利用を終了
  void Senna_Index::finish() {
    if (!senna_initialized) return;
    sen_rc rc = sen_fin();
    if (rc == sen_success)
      senna_initialized = 0;
    else
      fprintf(stderr, "ERROR: sen_fin: %s\n", get_error_text(rc));
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

namespace {

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


};

// --------------------------------------------------------------------------------

extern "C" {

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
  { "index", cmd_index01, },
  { "search", cmd_search01, },
  { 0 },
};

#endif
