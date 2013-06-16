/*! \file
 * \brief ドキュメント検索の基本操作を行うサンプル・コード
 */

#include <string>
#include <vector>

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
  };

};

// --------------------------------------------------------------------------------

using namespace std;

#include <cerrno>
#include <cstdlib>
#include <db.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/types.h>

#include "uc/elog.hpp"
#include "uc/datetime.hpp"

extern "C" int ends_with(const char *target, const char *suffix);

/// 実装クラスを隠す
namespace {

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

  static string bdb_name(string &dir_path, const char *dbname, const char *suffix = ".bdb") {
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
      elog("bdb->open %s,%0x:(%d):%s\n", bdb_path.c_str(), flag, rc, db_strerror(rc));
      return 0;
    }

    if (db) {
      rc = db->close(db, 0);
      if (rc) elog(W, "bdb->close %s:(%d):%s\n", db_name.c_str(), rc, db_strerror(rc));
      elog(W, "%s force closed.\n", db_name.c_str());
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


};

// --------------------------------------------------------------------------------

namespace uc {

  DBA::DBA(const char *dir) : db_dir_path(dir) { }

  DBA *DBA::get_dba_instance(const char *dir_path, const char *dba_type) {
    return new DBA_BDB_Impl(dir_path);
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

      if (!db->put_value(keybuf.c_str(), line)) { rc = 1; break; }
      /*
	登録に失敗したら速やかに中断する
      */
    }

    return rc;
  }

};

// --------------------------------------------------------------------------------

extern "C" {

  // awk -F: '{print $1;print $0}' < /etc/passwd > work/user-by-loginname.dump
  // awk -F: '{print $3;print $0}' < /etc/passwd > work/user-by-pid.dump


  /// Berkeley DBを操作してみる
  int cmd_bdb01(int argc, char **argv) {

    const char *dir_path = "work";
    const char *dba_type = "bdb";

    int opt;
    const char *mode = "r";
    const char *lang = "";
    bool show_key_list = false;

    while ((opt = getopt(argc,argv,"cD:kL:uv")) != -1) {
      switch(opt) { 
      case 'c': mode = "c"; break;
      case 'u': mode = "w"; break;
      case 'k': show_key_list = true; break;
      case 'L': lang = optarg; break;
      case 'D': dir_path = optarg; break;
      }
    }

    auto_ptr<DBA_tool> tool(new DBA_tool(dir_path, dba_type));

    uc::Text_Source::set_locale(lang);

    if (argc - optind < 1) {
      cerr << "usage: " << argv[0] << " [-c] <dbname> [source-file]" << endl;
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
  int cmd_index01(int argc, char **argv) {
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
  { "bdb", cmd_bdb01, },
  { "index", cmd_index01, },
  { "search", cmd_search01, },
  { 0 },
};

#endif
