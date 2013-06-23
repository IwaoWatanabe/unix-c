/*! \file
 * \brief Key-Value-Storeを取り扱うAPI
 */

#include "uc/kvs.hpp"

#include "uc/elog.hpp"
#include "depot.h"

#include <cerrno>
#include <cstdlib>
#include <dirent.h>
#include <fcntl.h>
#include <sys/types.h>

extern "C" int ends_with(const char *target, const char *suffix);

using namespace std;

/// 実装クラスを隠す
namespace {

  // --------------------------------------------------------------------------------
  // QDBMバージョン１基本仕様書
  // http://fallabs.com/qdbm/spex-ja.html#depotapi

  /// QDB(DEPOT)を操作するKVS実装クラス
  class KVS_DEPOT_Impl : public uc::KVS, uc::ELog {
    DEPOT* db;
    string db_name;
    int db_mode, bnum;

  public:
    KVS_DEPOT_Impl();
    virtual ~KVS_DEPOT_Impl();
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

  KVS_DEPOT_Impl::KVS_DEPOT_Impl()
    : db(0), db_mode(0), bnum(-1)
  {
    init_elog("kvs-depot");
  }

  KVS_DEPOT_Impl::~KVS_DEPOT_Impl() {
    close_kvs();
  }

  bool KVS_DEPOT_Impl::set_kvs_directory(const char *dir) {
    if (db) return false;
    if (!dir) dir = "";
    db_dir_path = dir;
    return true;
  }

  void KVS_DEPOT_Impl::get_kvs_list(std::vector<std::string> &list) {
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
    string db_path(dir_path);
    if (!db_path.empty()) db_path += "/";
    db_path += dbname;
    db_path += suffix;
    return db_path;
  }

  void KVS_DEPOT_Impl::drop_kvs(const char *dbname) {
    string dbpath = qdb_name(db_dir_path, dbname);
    if (!unlink(dbpath.c_str())) {
      elog("unlink %s:(%d):%s\n", dbpath.c_str(), errno, strerror(errno));
    }
  }

  int KVS_DEPOT_Impl::open_kvs(const char *dbname, const char *mode) {

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

  bool KVS_DEPOT_Impl::fetch_value(const char *key, std::string &value) {
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

  bool KVS_DEPOT_Impl::store_value(const char *key, const char *value) {
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

  bool KVS_DEPOT_Impl::has_key(const char *key) {
    if (!db) return false;
    int size = dpvsiz(db, key, -1);
    return size > 0;
  }

  void KVS_DEPOT_Impl::begin_next_key() {
    if (!db) return;
    if (!dpiterinit(db)) {
      elog("dpiterinit %s:(%d):%s\n", db_name.c_str(), dpecode, dperrmsg(dpecode));
    }
  }

  bool KVS_DEPOT_Impl::fetch_next_key(std::string &key) {
    if (!db) { key = ""; return false; }

    char *dk = dpiternext(db, NULL);
    if (!dk) { key = ""; return false; }
    key = dk;
    free(dk);
    return true;
  }

  void KVS_DEPOT_Impl::end_next_key() {
    // nothing
  }

  void KVS_DEPOT_Impl::close_kvs() {
    if (!db) return;

    if (!dpclose(db))
      elog(W, "dpclose %s:(%d):%s\n", db_name.c_str(), dpecode, dperrmsg(dpecode));

    elog(D, "%s closed.\n", db_name.c_str());

    db = 0;
    db_name = "";
    db_mode = 0;
  }

  void KVS_DEPOT_Impl::sync_kvs() {
    if (!db) return;
    if (!dpsync(db))
      elog(F, "dpsync %s:(%d):%s\n", db_name.c_str(), dpecode, dperrmsg(dpecode));
  }

  const char *KVS_DEPOT_Impl::get_kvs_version() {
    static char vbuf[20] = { 0 };
    if (!vbuf[0]) snprintf(vbuf,sizeof vbuf,"QDBM:%s (Depot)",dpversion);
    return vbuf;
  }

};

extern uc::KVS *create_KVS_DEPOT_Impl() { return new KVS_DEPOT_Impl(); }

