/*! \file
 * \brief Key-Value-Storeを取り扱うAPI
 */

#include "uc/kvs.hpp"

#include <cerrno>
#include <cstdlib>
#include <dirent.h>
#include <fcntl.h>
#include <sys/types.h>
#include <gdbm.h>


#include "uc/elog.hpp"

extern "C" int ends_with(const char *target, const char *suffix);

using namespace std;

/// 実装クラスを隠す
namespace {

  /// GDBM ライブラリを使うKVS実装クラス
  class KVS_GDBM_Impl : public uc::KVS, uc::ELog {
    GDBM_FILE db;
    string db_name;
    int db_mode;
    datum cursor;
    int block_size;
    int store_count, fetch_count, delete_count;

  public:
    KVS_GDBM_Impl();
    virtual ~KVS_GDBM_Impl();
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

  KVS_GDBM_Impl::KVS_GDBM_Impl() : 
    db(0), db_mode(0), block_size(0), store_count(0), fetch_count(0), delete_count(0)
  {
    cursor.dptr = 0;
    init_elog("kvs-gdbm");
  }

  KVS_GDBM_Impl::~KVS_GDBM_Impl() {
    end_next_key();
    close_kvs();
  }

  bool KVS_GDBM_Impl::set_kvs_directory(const char *dir) {
    if (db) return false;
    if (!dir) dir = "";
    db_dir_path = dir;
    return true;
  }

  void KVS_GDBM_Impl::get_kvs_list(std::vector<std::string> &list) {
    DIR *dp;

    if ((dp = opendir(db_dir_path.c_str())) == NULL) {
      elog("opendir %s:(%d):%s\n", db_dir_path.c_str(), errno, strerror(errno));
      return;
    }

    struct dirent *dir;

    while ((dir = readdir(dp)) != NULL ){
      if (!ends_with(dir->d_name, ".gdbm")) continue;
      string dname(dir->d_name);
      if (dname[0] == '.') continue;
      list.push_back(dname);
    }

    closedir(dp);
  }

  static string dbm_name(const string &dir_path, const char *dbname, const char *suffix = ".gdbm") {
    string db_path(dir_path);
    if (!db_path.empty()) db_path += "/";
    db_path += dbname;
    db_path += suffix;
    return db_path;
  }

  /// データベース・ファイルを破棄する
  void KVS_GDBM_Impl::drop_kvs(const char *dbname) {

    string dbpath;

    dbpath = dbm_name(db_dir_path, dbname);
    if (!unlink(dbpath.c_str())) {
      elog("unlink %s:(%d):%s\n", dbpath.c_str(), errno, strerror(errno));
    }
  }

  /// データベース利用開始
  int KVS_GDBM_Impl::open_kvs(const char *dbname, const char *mode) {

    int flag = GDBM_WRITER;
    const char *saved_mode = mode;

    while (*mode) {
      switch(*mode) {
      case 'c':
	flag = GDBM_NEWDB;
	break;
      case 'r':
	flag = GDBM_READER;
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
    GDBM_FILE dbm = gdbm_open((char *)dbm_path.c_str(), block_size, flag, 0777, 0);
    if (gdbm_errno) {
      elog("gdbm_open %s,%d,%s(%#0x):(%d):%s\n", 
	   dbm_path.c_str(), block_size, saved_mode, flag, gdbm_errno, gdbm_strerror(gdbm_errno));
      return 0;
    }

    elog(D, "%s ,%s(%#0x) opened.\n", dbm_path.c_str(), saved_mode, flag);

    if (db) {
      // 前の接続がある場合は自動で閉じる
      gdbm_close(db);

      elog(I, "%s force closed. fetch:%d, store:%d, delete:%d\n",
	   db_name.c_str(), fetch_count, store_count, delete_count);
    }

    db = dbm;
    db_name = dbname;
    db_mode = flag;
    store_count = fetch_count = delete_count = 0;

    return 1;
  }
  
  /// 値の入手
  bool KVS_GDBM_Impl::fetch_value(const char *key, std::string &value) {
    if (!db) { value = ""; return false; }

    datum dkey;
    dkey.dptr = (char *)key;
    dkey.dsize = strlen(key);

    datum dvalue = gdbm_fetch(db, dkey);
    fetch_count++;
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
      int rc = gdbm_delete(db, dkey);
      if (rc != 0)
	elog(W, "gdbm_delete %s,%s:(%d):%s\n", db_name.c_str(), key, gdbm_errno, gdbm_strerror(gdbm_errno));
      delete_count++;
      return rc == 0;
    }
    dvalue.dptr = (char *)value;
    dvalue.dsize = strlen(value);
    store_count++;
    
    int rc = gdbm_store(db, dkey, dvalue, GDBM_REPLACE);
    if (rc)
      elog(W, "gdbm_store %s,%s:(%d):%s\n", db_name.c_str(), key, gdbm_errno, gdbm_strerror(gdbm_errno));
    return rc == 0;
  }

  /// 登録キー名の確認
  bool KVS_GDBM_Impl::has_key(const char *key) {
    if (!db) { return false; }
    datum dkey;
    dkey.dptr = (char *)key;
    dkey.dsize = strlen(key);
    int rc = gdbm_exists(db, dkey);
    return rc != 0;
  }
  
  /// 登録キー名の入手開始
  void KVS_GDBM_Impl::begin_next_key() {
    if (!db) {
      elog(W, "begin_next_key but db %s closed.\n", db_name.c_str());
      return;
    }

    cursor = gdbm_firstkey(db);
    elog(D, "cursor for %s created.\n", db_name.c_str());
  }
  
  /// 登録キー名の入手
  bool KVS_GDBM_Impl::fetch_next_key(std::string &key) {
    if (!cursor.dptr) { key = ""; return false; }

    key.assign((char *)cursor.dptr, cursor.dsize);
    datum next_key = gdbm_nextkey(db, cursor);
    free(cursor.dptr);
    cursor = next_key;

    return cursor.dptr != 0;
  }
  
  /// キーの入手の終了
  void KVS_GDBM_Impl::end_next_key() {
    free(cursor.dptr);
    cursor.dptr = 0;
  }

  /// リソースの開放
  void KVS_GDBM_Impl::close_kvs() {
    if (!db) return;

    gdbm_close(db);

    elog(D, "%s closed. fetch:%d, store:%d, delete:%d\n",
	 db_name.c_str(), fetch_count, store_count, delete_count);

    db = 0;
    db_name = "";
    db_mode = 0;
  }
  
  /// データのストレージ同期
  void KVS_GDBM_Impl::sync_kvs() {
    if (!db) return;

    gdbm_sync(db);
    /*
      メインメモリの状態をディスクの状態と同期させるまでは戻って来ない
     */
    elog(D, "%s ,(%#0x) sync called.\n", db_name.c_str());
  }

  const char *KVS_GDBM_Impl::get_kvs_version() {
    return gdbm_version;
  }

};

extern uc::KVS *create_KVS_GDBM_Impl() { return new KVS_GDBM_Impl(); }

