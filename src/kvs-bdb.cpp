/*! \file
 * \brief Key-Value-Storeを取り扱うAPI (Berkeley DB)
 */

#include "uc/kvs.hpp"

#include "uc/elog.hpp"
#include "db.h"

#include <cerrno>
#include <cstdlib>
#include <dirent.h>
#include <fcntl.h>
#include <sys/types.h>

extern "C" int ends_with(const char *target, const char *suffix);

using namespace std;

/// 実装クラスを隠す
namespace {

  /// Berkeley DBを操作するKVS実装クラス
  class KVS_BDB_Impl : public uc::KVS, uc::ELog {
    DB *db;
    DBC *cursor;
    string db_name;
    int db_mode;

  public:
    KVS_BDB_Impl();
    virtual ~KVS_BDB_Impl();
    virtual bool set_kvs_directory(const char *dir);
    virtual void get_kvs_list(std::vector<std::string> &list);
    virtual void drop_kvs(const char *dbname);
    virtual int open_kvs(const char *dbname, const char *mode);
    virtual bool fetch_value(const char *key, std::string &value);
    virtual bool store_value(const char *key, const char *value);
    virtual bool has_key(const char *key);
    virtual void begin_next_key();
    virtual bool fetch_next_key(std::string &key);
    virtual void end_next_key();
    virtual void close_kvs();
    virtual void sync_kvs();
    virtual const char *get_kvs_version();
  };

  KVS_BDB_Impl::KVS_BDB_Impl()
    : db(0), cursor(0), db_mode(0)
  {
    init_elog("kvs-bdb");
  }

  KVS_BDB_Impl::~KVS_BDB_Impl() {
    end_next_key();
    close_kvs();
  }

  bool KVS_BDB_Impl::set_kvs_directory(const char *dir) {
    if (db) return false;
    if (!dir) dir = "";
    db_dir_path = dir;
    return true;
  }

  void KVS_BDB_Impl::get_kvs_list(std::vector<std::string> &list) {
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
    string db_path(dir_path);
    if (!db_path.empty()) db_path += "/";
    db_path += dbname;
    db_path += suffix;
    return db_path;
  }

  // ゼロクリア用
  static DBT DBT_ZERO;

  /// データベース・ファイルを破棄する
  void KVS_BDB_Impl::drop_kvs(const char *dbname) {

    string dbpath = bdb_name(db_dir_path, dbname);
    if (!unlink(dbpath.c_str())) {
      elog("unlink %s:(%d):%s\n", dbpath.c_str(), errno, strerror(errno));
    }
  }

  /// データベース利用開始
  int KVS_BDB_Impl::open_kvs(const char *dbname, const char *mode) {

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
  bool KVS_BDB_Impl::fetch_value(const char *key, std::string &value) {
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
  bool KVS_BDB_Impl::store_value(const char *key, const char *value) {
    if (!db) {
      elog(W, "bdb %s not opened when sotre %s\n", db_name.c_str(), key);
      return false;
    }

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
  bool KVS_BDB_Impl::has_key(const char *key) {
    DBT dbkey = DBT_ZERO, dbdata = DBT_ZERO;
    dbkey.data = (char *)key;
    dbkey.size = strlen(key);
    int rc = db->get(db,NULL,&dbkey,&dbdata,0);
    return rc == 0;
  }
  
  /// 登録キー名の入手開始
  void KVS_BDB_Impl::begin_next_key() {
    if (cursor) {
      elog(W, "cursor for %s alreay created.\n", db_name.c_str());
      return;
    }
    if (!db) {
      elog(W, "not bdb opened.\n");
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
  bool KVS_BDB_Impl::fetch_next_key(std::string &key) {
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
  void KVS_BDB_Impl::end_next_key() {
    if (!cursor) { return; }

    int rc = cursor->c_close(cursor);
    if (rc == 0) { cursor = NULL; return; }

    elog(W, "cursor close %s:(%d):%s\n", db_name.c_str(), rc, db_strerror(rc));
  }

  /// リソースの開放
  void KVS_BDB_Impl::close_kvs() {
    if (!db) return;

    int rc = db->close(db, 0);
    if (rc) elog(W, "bdb->close %s:(%d):%s\n", db_name.c_str(), rc, db_strerror(rc));

    elog(D, "%s closed.\n", db_name.c_str());

    db = 0;
    db_name = "";
    db_mode = 0;
  }
  
  /// データのストレージ同期
  void KVS_BDB_Impl::sync_kvs() {
    if (!db) return;
    int rc = db->sync(db,0);
    if (rc)
      elog(W, "bdb->sync %s:(%d):%s\n", db_name.c_str(), rc, db_strerror(rc));
  }

  const char *KVS_BDB_Impl::get_kvs_version() {
    int major, minor, patch;
    char *ver = db_version(&major, &minor, &patch);
    return ver;
  }

};

extern uc::KVS *create_KVS_BDB_Impl() { return new KVS_BDB_Impl(); }

