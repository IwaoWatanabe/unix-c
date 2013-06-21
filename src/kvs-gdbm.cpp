/*! \file
 * \brief Key-Value-Storeを取り扱うAPI
 */

#include "uc/kvs.hpp"

#include <cerrno>
#include <cstdlib>
#include <dirent.h>
#include <fcntl.h>
#include <sys/types.h>

extern "C" {

#ifdef HAVA_NDBM_H
#include <ndbm.h>
#else

#ifndef DBM_INSERT
#define DBM_INSERT 0
#endif

#ifndef DBM_REPLACE
#define DBM_REPLACE 1
#endif

  typedef struct { char *dptr; int dsize; } datum;
  typedef void *DBM;
  extern int dbm_clearerr(DBM *db);
  extern void dbm_close(DBM *db);
  extern int dbm_delete(DBM *db, datum key);
  extern int dbm_error(DBM *db);
  extern datum dbm_fetch(DBM *db, datum key);
  extern datum dbm_firstkey(DBM *db);
  extern datum dbm_nextkey(DBM *db);
  extern DBM *dbm_open(const char *file, int open_flags, mode_t file_mode);
  extern int dbm_store(DBM *db, datum key, datum content, int store_mode);
#endif

};

#include "uc/elog.hpp"

extern "C" int ends_with(const char *target, const char *suffix);

using namespace std;

/// 実装クラスを隠す
namespace {

  /// NDBM コンパチライブラリを使って　GDBMを操作するKVS実装クラス
  class KVS_GDBM_Impl : public uc::KVS, uc::ELog {
    DBM *db;
    string db_name;
    int db_mode;
    datum cursor;

  public:
    KVS_GDBM_Impl(const char *dir);
    ~KVS_GDBM_Impl();
    void get_kvs_list(std::vector<std::string> &list);
    void drop_kvs(const char *dbname);
    int open_kvs(const char *dbname, const char *mode);
    bool fetch_value(const char *key, std::string &value);
    bool store_value(const char *key, const char *value);
    bool has_key(const char *key);
    void begin_next_key();
    bool fetch_next_key(std::string &key);
    void end_next_key();
    void close_kvs();
    void sync_kvs();
    const char *get_kvs_version();
  };

  KVS_GDBM_Impl::KVS_GDBM_Impl(const char *dir) : KVS(dir), db(0), db_mode(0) { 
    cursor.dptr = 0;
    init_elog("kvs-bdb");
  }

  KVS_GDBM_Impl::~KVS_GDBM_Impl() {
    close_kvs();
  }

  void KVS_GDBM_Impl::get_kvs_list(std::vector<std::string> &list) {
    DIR *dp;

    if ((dp = opendir(db_dir_path.c_str())) == NULL) {
      elog("opendir %s:(%d):%s\n", db_dir_path.c_str(), errno, strerror(errno));
      return;
    }

    struct dirent *dir;

    while ((dir = readdir(dp)) != NULL ){
      if (!ends_with(dir->d_name, ".dir")) continue;
      string dname(dir->d_name);
      if (dname[0] == '.') continue;
      list.push_back(dname);
    }

    closedir(dp);
  }

  static string dbm_name(const string &dir_path, const char *dbname, const char *suffix = "") {
    string bdb_path(dir_path);
    bdb_path += "/";
    bdb_path += dbname;
    bdb_path += suffix;
    return bdb_path;
  }

  /// データベース・ファイルを破棄する
  void KVS_GDBM_Impl::drop_kvs(const char *dbname) {

    string dbpath;

    dbpath = dbm_name(db_dir_path, dbname, ".dir");
    if (!unlink(dbpath.c_str())) {
      elog("unlink %s:(%d):%s\n", dbpath.c_str(), errno, strerror(errno));
    }

    dbpath = dbm_name(db_dir_path, dbname, ".pag");
    if (!unlink(dbpath.c_str())) {
      elog("unlink %s:(%d):%s\n", dbpath.c_str(), errno, strerror(errno));
    }
  }

  /// データベース利用開始
  int KVS_GDBM_Impl::open_kvs(const char *dbname, const char *mode) {

    int flag = O_RDWR;
    const char *saved_mode = mode;

    while (*mode) {
      switch(*mode) {
      case 'c':
	flag |= O_CREAT|O_TRUNC;
	break;
      case 'r':
	flag &= ~O_RDWR;
	flag |= O_RDONLY;
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

    string dbm_path = dbm_name(db_dir_path, dbname);
    DBM *dbm = dbm_open(dbm_path.c_str(), flag, 0777);
    if (!dbm) {
      elog("dbm_open %s ,%s(%#0x):(%d):%s\n", dbm_path.c_str(), saved_mode, flag, errno, strerror(errno));
      return 0;
    }

    elog(D, "%s ,%s(%#0x) opened.\n", dbm_path.c_str(), saved_mode, flag);

    if (db) {
      // 前の接続がある場合は自動で閉じる
      dbm_close(db);
      elog(I, "%s force closed.\n", db_name.c_str());
    }

    db = dbm;
    db_name = dbname;
    db_mode = flag;
    return 1;
  }
  
  /// 値の入手
  bool KVS_GDBM_Impl::fetch_value(const char *key, std::string &value) {
    if (!db) { value = ""; return false; }

    datum dkey;
    dkey.dptr = (char *)key;
    dkey.dsize = strlen(key);

    datum dvalue = dbm_fetch(db, dkey);
    if (dvalue.dptr == 0) {
      // 値が取れなければ空文字を返す
      value = "";
      return false;
    }

    value.assign((char *)dvalue.dptr, dvalue.dsize);
    return true;
  }

  /// 値の登録
  bool KVS_GDBM_Impl::store_value(const char *key, const char *value) {

    datum dkey, dvalue;
    dkey.dptr = (char *)key;
    dkey.dsize = strlen(key);

    if (!value || !*value) {
      // 値が空であればエントリを削除する
      int rc = dbm_delete(db, dkey);
      if (rc != 0)
	elog(W, "dbm_del %s,%s:(%d)\n", db_name.c_str(), key, dbm_error(db));
      return rc == 0;
    }
    dvalue.dptr = (char *)value;
    dvalue.dsize = strlen(value);
    
    int rc = dbm_store(db,dkey,dvalue, DBM_REPLACE);
    if (rc)
      elog(W, "dbm_store %s,%s:(%d)\n", db_name.c_str(), key, dbm_error(db));
    return rc == 0;
  }

  /// 登録キー名の確認
  bool KVS_GDBM_Impl::has_key(const char *key) {
    if (!db) { return false; }
    datum dkey;
    dkey.dptr = (char *)key;
    dkey.dsize = strlen(key);
    datum dvalue = dbm_fetch(db, dkey);
    return dvalue.dptr != 0;
  }
  
  /// 登録キー名の入手開始
  void KVS_GDBM_Impl::begin_next_key() {
    if (!db) {
      elog(W, "begin_next_key but db %s closed.\n", db_name.c_str());
      return;
    }

    cursor = dbm_firstkey(db);
    elog(D, "cursor for %s created.\n", db_name.c_str());
  }
  
  /// 登録キー名の入手
  bool KVS_GDBM_Impl::fetch_next_key(std::string &key) {
    if (!cursor.dptr) { key = ""; return false; }

    key.assign((char *)cursor.dptr, cursor.dsize);
    cursor = dbm_nextkey(db);

    return true;
  }
  
  /// キーの入手の終了
  void KVS_GDBM_Impl::end_next_key() {
    cursor.dptr = 0;
  }

  /// リソースの開放
  void KVS_GDBM_Impl::close_kvs() {
    if (!db) return;

    dbm_close(db);
    elog(D, "%s closed.\n", db_name.c_str());

    db = 0;
    db_name = "";
    db_mode = 0;
  }
  
  /// データのストレージ同期
  void KVS_GDBM_Impl::sync_kvs() {
    if (!db) return;

    // 同期をとるAPIが無いので、閉じて開き直す
    dbm_close(db);

    string dbm_path = dbm_name(db_dir_path, db_name.c_str());
    DBM *dbm = dbm_open(dbm_path.c_str(), db_mode, 0777);
    if (!dbm) {
      elog("dbm_open %s ,(%#0x):(%d):%s\n", dbm_path.c_str(), db_mode, errno, strerror(errno));
      db = 0;
    }
    db = dbm;

    elog(D, "%s ,(%#0x) reopened.\n", dbm_path.c_str(), db_mode);
  }

  const char *KVS_GDBM_Impl::get_kvs_version() {
    return "NDBM: 1.0";
  }

};

namespace uc {
  KVS::KVS(const char *dir) : db_dir_path(dir) { }

  void KVS::show_report(FILE *fout) {
    fprintf(fout,"kvs: %s\n", get_kvs_version());
  }

  KVS *KVS::get_kvs_instance(const char *dir_path, const char *kvs_type) {
    return new KVS_GDBM_Impl(dir_path);
  }
};

