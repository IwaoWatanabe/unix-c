/*! \file
 * \brief ドキュメント検索の基本操作を行うサンプル・コード
 */

#include <string>
#include <vector>
#include <map>

namespace uc {

  /// Key-Value Store の基本機能を利用するインタフェース
  class DBA {
  protected:
    DBA(const char *dir);
    std::string db_dir_path;

  public:
    /// 操作用インスタンスの入手
    static DBA *get_dba_instance(const char *dir_path, const char *dba_type = "");
    virtual ~DBA() {}

    /// 基準ディレクトリの入手
    virtual std::string &get_dba_directory() { return db_dir_path; }

    /// データベース・ファイルの一覧を入手する
    virtual void get_dba_list(std::vector<std::string> &list) = 0;
    /// データベース・ファイルを破棄する
    virtual void drop_dba(const char *dbname) = 0;

    /// データベース利用開始
    virtual int open_dba(const char *dbname, const char *mode) = 0;
    /// 値の入手
    virtual bool fetch_value(const char *key, std::string &value) = 0;
    /// 値の登録
    virtual bool put_value(const char *key, const char *value) = 0;
    /// 登録キー名の確認
    virtual bool has_key(const char *key) = 0;

    /// 登録キー名の入手開始
    virtual void begin_next_key() = 0;
    /// 登録キー名の入手
    virtual bool fetch_next_key(std::string &key) = 0;
    /// キーの入手の終了
    virtual void end_next_key() = 0;

    /// リソースの開放
    virtual void close_dba() = 0;
    /// データのストレージ同期
    virtual void sync_dba() = 0;
    /// ストアの実装名とバージョン文字列を返す
    virtual const char *get_dba_version() { return "DBA: 0.0"; }
    /// ストアの状態をレポートする
    virtual void show_report(FILE *fout);
  };
};

// --------------------------------------------------------------------------------

namespace uc {

  /// 検索条件を格納するDTO
  struct Index_Condition {
    std::vector<std::string> target, search_word, and_key, not_key, or_key, or_key2, or_key3;
    std::map<std::string,std::string> includes, excludes;
  };

  /// インデックスの検索結果の入手
  class Index_Result_Documents {
  public:
    /// 次の対象ドキュメントのドキュメントIDの入手
    virtual bool fetch_next_document(std::string doc_id) = 0;
    virtual int get_fetch_count();

    Index_Result_Documents() {}
    virtual ~Index_Result_Documents() {}
  };

  /// インデックスの基本情報の入手
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
    virtual Index_Result_Documents *query(Index_Condition &condition) = 0;
    /// インデックスに対して検索処理をする(件数確認のみ)
    virtual long count_query(Index_Condition &condition) = 0;

  };

  /// 全文検索の基本機能を利用するインタフェース
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
    virtual Index_Result_Documents *query(const char *name, Index_Condition &condition) = 0;
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

  /// Berkeley DBを操作するDBA実装クラス
  class DBA_BDB_Impl : public uc::DBA, uc::ELog {
    DB *db;
    DBC *cursor;
    string db_name;
    int db_mode;

  public:
    DBA_BDB_Impl(const char *dir);
    ~DBA_BDB_Impl();
    void get_dba_list(std::vector<std::string> &list);
    void drop_dba(const char *dbname);
    int open_dba(const char *dbname, const char *mode);
    bool fetch_value(const char *key, std::string &value);
    bool put_value(const char *key, const char *value);
    bool has_key(const char *key);
    void begin_next_key();
    bool fetch_next_key(std::string &key);
    void end_next_key();
    void close_dba();
    void sync_dba();
    const char *get_dba_version();
  };

  DBA_BDB_Impl::DBA_BDB_Impl(const char *dir) : DBA(dir), db(0), cursor(0), db_mode(0) {
    init_elog("dba-bdb");
  }

  DBA_BDB_Impl::~DBA_BDB_Impl() {
    close_dba();
  }

  void DBA_BDB_Impl::get_dba_list(std::vector<std::string> &list) {
    DIR *dp;

    if ((dp = opendir(db_dir_path.c_str())) == NULL) {
      elog("opendir %s:(%d):%s\n", db_dir_path.c_str(), errno, strerror(errno));
      return;
    }

    struct dirent *dir;

    while ((dir = readdir(dp)) != NULL ){
      if (!ends_with(dir->d_name, ".bdb")) continue;
      string dname(dir->d_name);
      if (dname[0] == '.') continue;
      list.push_back(dname);
    }

    closedir(dp);
  }

  static string bdb_name(const string &dir_path, const char *dbname, const char *suffix = ".bdb") {
    string bdb_path(dir_path);
    bdb_path += "/";
    bdb_path += dbname;
    bdb_path += suffix;
    return bdb_path;
  }

  // ゼロクリア用
  static DBT DBT_ZERO;

  /// データベース・ファイルを破棄する
  void DBA_BDB_Impl::drop_dba(const char *dbname) {

    string dbpath = bdb_name(db_dir_path, dbname);
    if (!unlink(dbpath.c_str())) {
      elog("unlink %s:(%d):%s\n", dbpath.c_str(), errno, strerror(errno));
    }
  }

  /// データベース利用開始
  int DBA_BDB_Impl::open_dba(const char *dbname, const char *mode) {

    int flag = 0;
    const char *saved_mode = mode;

    while (*mode) {
      switch(*mode) {
      case 'c':
	flag |= DB_CREATE;
	break;
      case 'r':
	flag |= DB_RDONLY;
	break;
      }
      mode++;
    }

    if (db) {
      if (db_name == dbname && db_mode == flag) {
	// 前回の接続と同じであれば、開きなおさないで成功させる
	elog(W, "%s already opened.\n", dbname);
	return 1;
      }
    }

    DB *bdb;

    int rc = db_create(&bdb, NULL, 0);
    if (rc) {
      elog("db_create failed:(%d):%s\n", rc, db_strerror(rc));
      return 0;
    }

    string bdb_path = bdb_name(db_dir_path, dbname);

    rc = bdb->open(bdb, NULL, bdb_path.c_str(), NULL, DB_BTREE, flag, 0666);
    if (rc) {
      elog("bdb->open %s ,%s(%#0x):(%d):%s\n", bdb_path.c_str(), saved_mode, flag, rc, db_strerror(rc));
      return 0;
    }
    elog(D, "%s ,%s(%#0x) opened.\n", bdb_path.c_str(), saved_mode, flag);

    if (db) {
      // 前の接続がある場合は自動で閉じる
      rc = db->close(db, 0);
      if (rc)
	elog(W, "bdb->close %s:(%d):%s\n", db_name.c_str(), rc, db_strerror(rc));
      else
	elog(I, "%s force closed.\n", db_name.c_str());
    }

    db = bdb;
    db_name = dbname;
    db_mode = flag;

    return 1;
  }
  
  /// 値の入手
  bool DBA_BDB_Impl::fetch_value(const char *key, std::string &value) {
    if (!db) {
      value = "";
      return false;
    }

    DBT dbkey = DBT_ZERO, dbdata = DBT_ZERO;
    dbkey.data = (char *)key;
    dbkey.size = strlen(key);

    int rc = db->get(db,NULL,&dbkey,&dbdata,0);
    if (rc != 0) {
      elog(W, "bdb->get %s,%s:(%d):%s\n", db_name.c_str(), key, rc, db_strerror(rc));
      // 値が取れなければ空文字を返す
      value = "";
      return false;
    }

    value.assign((char *)dbdata.data, dbdata.size);
    return true;
  }

  /// 値の登録
  bool DBA_BDB_Impl::put_value(const char *key, const char *value) {
    DBT dbkey = DBT_ZERO, dbdata = DBT_ZERO;
    dbkey.data = (char *)key;
    dbkey.size = strlen(key);
    
    if (!value || !*value) {
      // 値が空であればエントリを削除する
      int rc = db->del(db,NULL,&dbkey,0);
      if (rc)
	elog(W, "bdb->del %s,%s:(%d):%s\n", db_name.c_str(), key, rc, db_strerror(rc));
      return rc == 0;
    }
    dbdata.data = (char *)value;
    dbdata.size = strlen(value);

    int rc = db->put(db,NULL,&dbkey,&dbdata, 0);
    if (rc)
      elog(W, "bdb->put %s,%s:(%d):%s\n", db_name.c_str(), key, rc, db_strerror(rc));
    return rc == 0;
  }
  
  /// 登録キー名の確認
  bool DBA_BDB_Impl::has_key(const char *key) {
    DBT dbkey = DBT_ZERO, dbdata = DBT_ZERO;
    dbkey.data = (char *)key;
    dbkey.size = strlen(key);
    int rc = db->get(db,NULL,&dbkey,&dbdata,0);
    return rc == 0;
  }
  
  /// 登録キー名の入手開始
  void DBA_BDB_Impl::begin_next_key() {
    if (cursor) {
      elog(W, "cursor for %s alreay created.\n", db_name.c_str());
      return;
    }

    int rc = db->cursor(db, NULL, &cursor, 0);
    if (rc != 0) {
      elog(W, "cursor for %s:(%d):%s\n", db_name.c_str(), rc, db_strerror(rc));
      cursor = 0;
      return;
    }

    elog(D, "cursor for %s created.\n", db_name.c_str());
  }
  
  /// 登録キー名の入手
  bool DBA_BDB_Impl::fetch_next_key(std::string &key) {
    if (!cursor) return false;

    DBT dbkey = DBT_ZERO, dbdata = DBT_ZERO;

    int rc = cursor->c_get(cursor, &dbkey, &dbdata, DB_NEXT);
    if (rc == 0) {
      key.assign((char *)dbkey.data, dbkey.size);
      return true;
    }

    if (rc != DB_NOTFOUND)
      elog(W, "cursor next for %s: (%d):%s\n", db_name.c_str(), rc, db_strerror(rc));
    return false;
  }
  
  /// キーの入手の終了
  void DBA_BDB_Impl::end_next_key() {
    if (!cursor) { return; }

    int rc = cursor->c_close(cursor);
    if (rc == 0) { cursor = NULL; return; }

    elog(W, "cursor close %s:(%d):%s\n", db_name.c_str(), rc, db_strerror(rc));
  }

  /// リソースの開放
  void DBA_BDB_Impl::close_dba() {
    if (!db) return;

    int rc = db->close(db, 0);
    if (rc) elog(W, "bdb->close %s:(%d):%s\n", db_name.c_str(), rc, db_strerror(rc));

    elog(D, "%s closed.\n", db_name.c_str());

    db = 0;
    db_name = "";
    db_mode = 0;
  }
  
  /// データのストレージ同期
  void DBA_BDB_Impl::sync_dba() {
    if (!db) return;
    int rc = db->sync(db,0);
    if (rc)
      elog(W, "bdb->sync %s:(%d):%s\n", db_name.c_str(), rc, db_strerror(rc));
  }

  const char *DBA_BDB_Impl::get_dba_version() {
    int major, minor, patch;
    char *ver = db_version(&major, &minor, &patch);
    return ver;
  }
  
  // --------------------------------------------------------------------------------
  // QDBMバージョン１基本仕様書
  // http://fallabs.com/qdbm/spex-ja.html#depotapi

  /// QDB(DEPOT)を操作するDBA実装クラス
  class DBA_DEPOT_Impl : public uc::DBA, uc::ELog {
    DEPOT* db;
    string db_name;
    int db_mode, bnum;

  public:
    DBA_DEPOT_Impl(const char *dir);
    ~DBA_DEPOT_Impl();
    void get_dba_list(std::vector<std::string> &list);
    void drop_dba(const char *dbname);
    int open_dba(const char *dbname, const char *mode);
    bool fetch_value(const char *key, std::string &value);
    bool put_value(const char *key, const char *value);
    bool has_key(const char *key);
    void begin_next_key();
    bool fetch_next_key(std::string &key);
    void end_next_key();
    void close_dba();
    void sync_dba();
    const char *get_dba_version();
  };

  DBA_DEPOT_Impl::DBA_DEPOT_Impl(const char *dir)
    : DBA(dir), db(0), db_mode(0), bnum(-1)
  {
    init_elog("dba-depot");
  }

  DBA_DEPOT_Impl::~DBA_DEPOT_Impl() {
    close_dba();
  }

  void DBA_DEPOT_Impl::get_dba_list(std::vector<std::string> &list) {
    DIR *dp;

    if ((dp = opendir(db_dir_path.c_str())) == NULL) {
      elog("opendir %s:(%d):%s\n", db_dir_path.c_str(), errno, strerror(errno));
      return;
    }

    struct dirent *dir;

    while ((dir = readdir(dp)) != NULL ){
      if (!ends_with(dir->d_name, ".qdb")) continue;
      string dname(dir->d_name);
      if (dname[0] == '.') continue;
      list.push_back(dname);
    }

    closedir(dp);
  }

  static string qdb_name(string &dir_path, const char *dbname, const char *suffix = ".qdb") {
    return bdb_name(dir_path, dbname, suffix);
  }

  void DBA_DEPOT_Impl::drop_dba(const char *dbname) {
    string dbpath = qdb_name(db_dir_path, dbname);
    if (!unlink(dbpath.c_str())) {
      elog("unlink %s:(%d):%s\n", dbpath.c_str(), errno, strerror(errno));
    }
  }

  int DBA_DEPOT_Impl::open_dba(const char *dbname, const char *mode) {

    // DP_OWRITER: 読み書き
    // DP_OCREAT: ファイルが無い場合に新規作成する
    // DP_OTRUNC: ファイルが存在しても作り直す
    // DP_OSPARSE: ファイルをスパースにする

    int flag = DP_OWRITER;
    const char *saved_mode = mode;

    while (*mode) {
      switch(*mode) {
      case 'c':
	flag |= DP_OCREAT|DP_OTRUNC;
	break;
      case 'r':
	flag &= ~DP_OWRITER;
	break;
      }
      mode++;
    }

    /*
      DP_OWRITER: （読み書き両用モード）でデータベースファイルを開く際には
      そのファイルに対して排他ロックがかけられ、
      DP_OREADER（読み込み専用モード）で開く際には共有ロックがかけられる。
      その際には該当のロックがかけられるまで制御がブロックする。
      DP_ONOLCK を指示する場合はこのロックを回避できるか、アプリケーションが排他制御の責任を負う。
    */

    if (db) {
      if (db_name == dbname && db_mode == flag) {
	// 前回の接続と同じであれば、開きなおさないで成功させる
	elog(W, "%s already opened.\n", dbname);
	return 0;
      }
    }

    string qdb_path = qdb_name(db_dir_path, dbname);
    DEPOT* qdb = dpopen(qdb_path.c_str(), flag, bnum);
    if (!qdb) {
      elog("dpopen %s ,%s(%#0x),%d:(%d):%s\n", qdb_path.c_str(), saved_mode, flag, bnum, dpecode, dperrmsg(dpecode));
      return 0;
    }

    elog(D, "%s ,%s(%#0x) opened.\n", qdb_path.c_str(), saved_mode, flag);

    if (db) {
      // 前の接続がある場合は自動で閉じる
      if (!dpclose(db))
	elog(W, "dpclose %s:(%d):%s\n", db_name.c_str(), dpecode, dperrmsg(dpecode));
      else
	elog(I, "%s force closed.\n", db_name.c_str());
    }
    db = qdb;
    db_name = dbname;
    db_mode = flag;
    return 1;
  }

  bool DBA_DEPOT_Impl::fetch_value(const char *key, std::string &value) {
    if (!db) { value = ""; return false; }

    int offset = 0, size = -1;
    char *vstr = dpget(db, key, -1, offset, size, NULL);
    if (!vstr) {
      // 該当するレコードがない
      value = "";
      return false;
    }
    value = vstr;
    free(vstr);
    return true;
  }

  bool DBA_DEPOT_Impl::put_value(const char *key, const char *value) {
    if (!db) return false;

    if (!value || !*value) {
      // 値が空であればエントリを削除する
      int rc = dpout(db, key, -1);
      if (!rc)
	elog(W, "dpout %s,%s:(%d):%s\n", db_name.c_str(), key, dpecode, dperrmsg(dpecode));
      return rc != 0;
    }

    if (!dpput(db, key, -1, value, -1, DP_DOVER)) {
      // DP_DOVER: 既存のレコードの値を上書き
      // DP_DKEEP: 既存のレコードを残してエラー
      // DP_DCAT: 指定された値を既存の値の末尾に加える
      elog("dpput %s,%s:(%d):%s\n", db_name.c_str(), key, dpecode, dperrmsg(dpecode));
      return false;
    }
    return true;
  }

  bool DBA_DEPOT_Impl::has_key(const char *key) {
    if (!db) return false;
    int size = dpvsiz(db, key, -1);
    return size > 0;
  }

  void DBA_DEPOT_Impl::begin_next_key() {
    if (!db) return;
    if (!dpiterinit(db)) {
      elog("dpiterinit %s:(%d):%s\n", db_name.c_str(), dpecode, dperrmsg(dpecode));
    }
  }

  bool DBA_DEPOT_Impl::fetch_next_key(std::string &key) {
    if (!db) { key = ""; return false; }

    char *dk = dpiternext(db, NULL);
    if (!dk) { key = ""; return false; }
    key = dk;
    free(dk);
    return true;
  }

  void DBA_DEPOT_Impl::end_next_key() {
    // nothing
  }

  void DBA_DEPOT_Impl::close_dba() {
    if (!db) return;

    if (!dpclose(db))
      elog(W, "dpclose %s:(%d):%s\n", db_name.c_str(), dpecode, dperrmsg(dpecode));

    elog(D, "%s closed.\n", db_name.c_str());

    db = 0;
    db_name = "";
    db_mode = 0;
  }

  void DBA_DEPOT_Impl::sync_dba() {
    if (!db) return;
    if (!dpsync(db))
      elog(F, "dpsync %s:(%d):%s\n", db_name.c_str(), dpecode, dperrmsg(dpecode));
  }

  const char *DBA_DEPOT_Impl::get_dba_version() {
    static char vbuf[20] = { 0 };
    if (!vbuf[0]) snprintf(vbuf,sizeof vbuf,"QDBM:%s (Depot)",dpversion);
    return vbuf;
  }

  // --------------------------------------------------------------------------------

  class Index_Manager_Impl;
  class Senna_Index_Scanner;

  enum ELog_Level { I = uc::ELog::I, W = uc::ELog::W, };

  /// sennaインデックス・インスタンスと連動する Senna_Index_Scanner の従属クラス
  /*
    senna APIを直接的に呼び出す。
    この実装は一つのsennaインデックスを操作するだけだが、
    従属情報も合わせて制御するサブクラスが作成されるかもしれない。
  */

  class Senna_Index {
  protected:
    sen_index *index;
    string index_path;
    uc::ELog *logger;
    Senna_Index_Scanner *scanner;

  public:
    Senna_Index(Senna_Index_Scanner *sis, uc::ELog *elog) :
      scanner(sis),logger(elog) { }
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
      logger->elog(W, "sen_sym_close failur: %s\n",get_error_text(rc));
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
      logger->elog(I, "index %s opened\n",index_path);
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
      logger->elog("index %s create failur\n",index_path);
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
      logger->elog(I, "index %s opened\n",index_path);
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
      logger->elog("index %s create failur\n",index_path);
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
      logger->elog("index not opend.\n");
      return false;
    }

    // インデックスの登録・更新
    sen_rc rc =
      sen_index_upd(index, doc_key,
		    oldvalue, oldvalue_len,
		    newvalue, newvalue_len);

    if (rc == sen_success) return true;
    logger->elog(W, "update failur: %s\n", get_error_text(rc));
  }


  /// インデックスを削除する
  void Senna_Index::remove_index(const char *index_path) {
    sen_rc rc = sen_index_remove(index_path);
    if (rc == sen_success)
      logger->elog(I, "index removed: %s\n",index_path);
    else
      logger->elog(W, "index remove failed: %s\n",index_path);
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
      logger->elog(W, "WARNING: sen_index_info failur: %s", get_error_text(rc));
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

      if (!sym) logger->elog("symbol open/create failur: %s", sym_path);
    }
    return sym;
  }

  // シンボル・ファイルの利用終了
  void Senna_Index::close_symbol(sen_sym *sym) {
    if (sym) return;
    sen_rc rc = sen_sym_close(sym);
    if (rc == sen_success) return;
    logger->elog("symbol close failur: %s", get_error_text(rc));
  }

  void Senna_Index::show_symbol_info(sen_sym *sym, FILE *fout) {
    int key_size;
    unsigned flags;
    sen_encoding encoding;
    unsigned nrecords, file_size;

    // シンボル・ファイルの基本情報を入手
    sen_rc rc = sen_sym_info(sym, &key_size, &flags, &encoding, &nrecords, &file_size);
    if (rc != sen_success) {
      logger->elog(W, "WARNING: sen_sym_info failur: %s", get_error_text(rc));
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
    virtual uc::Index_Result_Documents *query(uc::Index_Condition &condition);
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
    virtual uc::Index_Result_Documents *query(const char *name, uc::Index_Condition &condition);
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
  uc::Index_Result_Documents *Senna_Index_Scanner::query(uc::Index_Condition &condition) {
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
  uc::Index_Result_Documents *Index_Manager_Impl::query(const char *name, uc::Index_Condition &condition) {
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

  DBA::DBA(const char *dir) : db_dir_path(dir) { }

  void DBA::show_report(FILE *fout) {
    fprintf(fout,"dba: %s\n", get_dba_version());
  }

  DBA *DBA::get_dba_instance(const char *dir_path, const char *dba_type) {
    if (strcasecmp(dba_type,"bdb") == 0)
      return new DBA_BDB_Impl(dir_path);

    if (strcasecmp(dba_type,"qdb") == 0 || strcasecmp(dba_type,"depot") == 0)
      return new DBA_DEPOT_Impl(dir_path);

    return new DBA_BDB_Impl(dir_path);
  }

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

  /// DBAの基本機能を確認するツール
  class DBA_tool {
    uc::Local_Text_Source *ts;

  public:
    DBA_tool(const char *path, const char *type) :
      db(uc::DBA::get_dba_instance(path, type)), ts(create_Local_Text_Source()) { }
    ~DBA_tool() { delete db; delete ts; }

    uc::DBA *db;
    int show_key_list();
    int dump_key_values();
    int import_key_values(const char *input_file);
  };

  /// 保持しているキー名を出力する
  int DBA_tool::show_key_list() {
    db->begin_next_key();
    string key;
    int ct = 0;
    while (db->fetch_next_key(key)) {
      cout << key << endl;
      ct++;
    }
    db->end_next_key();
    return ct;
  }

  /// 保持しているキー名とデータを出力する
  int DBA_tool::dump_key_values() {
    string key, value;
    db->begin_next_key();
    int ct = 0;
    while (db->fetch_next_key(key)) {
      if (db->fetch_value(key.c_str(), value)) {
	cout << key << endl << value << endl << endl;
	ct++;
      }
    }
    db->end_next_key();
    return ct;
  }

  /// ファイルからKVデータを読込み、DBAに登録する
  int DBA_tool::import_key_values(const char *input_file) {

    if (!ts->open_read_file(input_file)) return 1;

    char *line;
    int rc = 0;

    // # で始まる行はコメント扱いとする
    // key,value を交互の行を入力とする
    // value の空行は削除を意味する

    while((line = ts->read_line()) != 0) {
      line = trim(line);
      if (*line == '#' || !*line) continue;
      /*
	key の空行はスキップする。
	つまり空行を数件入れると、
	その次の空行でない行は必ずkeyとして扱われる
      */

      // cerr << "key read: >>" << line << "<<" << endl;
      string keybuf(line);
      /*
	次の read_line の呼び出しで無効になるため複製している。
      */

      while ((line = ts->read_line()) != 0) {
	line = trim(line);
	if (*line != '#') break;
      }

      if (!line) break;

      // cerr << "value read: >>" << line << "<<" << endl;

      if (!db->put_value(keybuf.c_str(), line)) {
	fprintf(stderr,"Break Loop!\n");
	rc = 1; break;
      }
      /*
	登録に失敗したら速やかに中断する
      */
    }

    return rc;
  }

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

  // awk -F: '{print $1;print $0}' < /etc/passwd > work/user-by-loginname.dump
  // awk -F: '{print $3;print $0}' < /etc/passwd > work/user-by-pid.dump


  /// DBAオブジェクトを操作してみる
  int cmd_dba01(int argc, char **argv) {

    const char *dir_path = "work";
    const char *dba_type = "bdb";

    int opt;
    const char *mode = "r";
    const char *lang = "";
    bool show_key_list = false;

    while ((opt = getopt(argc,argv,"cD:kL:T:uv")) != -1) {
      switch(opt) { 
      case 'c': mode = "c"; break;
      case 'u': mode = "w"; break;
      case 'k': show_key_list = true; break;
      case 'L': lang = optarg; break;
      case 'D': dir_path = optarg; break;
      case 'T': dba_type = optarg; break;
      case '?': return 1;
      }
    }

    auto_ptr<DBA_tool> tool(new DBA_tool(dir_path, dba_type));

    uc::Text_Source::set_locale(lang);

    if (argc - optind < 1) {
      cerr << "usage: " << argv[0] << " [-c][-k] <dbname> [source-file]" << endl;
      /*
	データベースが指定されていないため、
	定義済みデータベースの一覧を出力する。
       */
      vector<string> list;
      
      tool->db->get_dba_list(list);
      vector<string>::iterator it = list.begin();
      for (;it != list.end(); it++) {
	cout << *it << endl;
      }
      cout << list.size() << " databases found." << endl; 
      tool->db->show_report(stderr);

      return EXIT_FAILURE;
    }

    if (argc - optind <= 1) {
      const char *dbname = argv[optind];
      if (!tool->db->open_dba(dbname, "r")) return EXIT_FAILURE;

      if (show_key_list) {
	cerr << tool->show_key_list() << " keys registerd." << endl;
	return EXIT_SUCCESS;
      }
      
      cerr << tool->dump_key_values() << " entries dumped." << endl;
      /*
	保持しているキー名とデータを出力する
       */
      return EXIT_SUCCESS;
    }

    // 以下、登録処理

    const char *dbname = argv[optind];
    const char *input_file = argv[optind + 1];
    if (*mode == 'r') mode = "w";
    if (!tool->db->open_dba(dbname, mode)) return 1;

    return tool->import_key_values(input_file);
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
  { "bda", cmd_dba01, },
  { "index", cmd_index01, },
  { "search", cmd_search01, },
  { 0 },
};

#endif
