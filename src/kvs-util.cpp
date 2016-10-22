/*! \file
 * \brief Key-Value-Storeを取り扱うAPI
 */

#include "uc/kvs.hpp"
#include "uc/local-file.hpp"

#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <cxxabi.h>
#include <iostream>
#include <typeinfo>
#include <vector>
#include <memory>

#include <unistd.h>

using namespace std;
extern int vsprintf(string &buf, const char *format, va_list ap);

extern uc::KVS *create_KVS_BDB_Impl();
extern uc::KVS *create_KVS_GDBM_Impl();
extern uc::KVS *create_KVS_NDBM_Impl();
extern uc::KVS *create_KVS_DEPOT_Impl();

extern "C" {

  extern uc::Local_Text_Source *create_Local_Text_Source();
  extern char *trim(char *t);


  extern char *demangle(const char *demangle) {
    int status;
    return abi::__cxa_demangle(demangle, 0, 0, &status);
  }

};

namespace uc {

  KVS::~KVS() { }

  void KVS::show_report(FILE *fout) {
    fprintf(fout,"KVS-Driver: %s\n"
	    "KVS-Version: %s\n", demangle(typeid(*this).name()), get_kvs_version());
  }

  bool KVS::set_kvs_fdirectory(const char *format, ...) {
    string buf;
    va_list ap;
    va_start(ap, format);
    int n = vsprintf(buf, format, ap);
    va_end(ap);
    if (n == 0) return false;
    return set_kvs_directory(buf.c_str());
  }

  int KVS::open_fkvs(const char *db_format, const char *mode, ...) {
    string buf;
    va_list ap;
    va_start(ap, mode);
    int n = vsprintf(buf, db_format, ap);
    va_end(ap);
    if (n == 0) return 0;
    return open_kvs(buf.c_str(), mode);
  }

  bool KVS::fetch_fvalue(const char *key_format, std::string &value, ...) {
    string buf;
    va_list ap;
    va_start(ap, value);
    int n = vsprintf(buf, key_format, ap);
    va_end(ap);
    if (n == 0) return false;
    return fetch_value(buf.c_str(), value);
  }

  bool KVS::store_fvalue(const char *key_format, const char *value, ...) {
    string buf;
    va_list ap;
    va_start(ap, value);
    int n = vsprintf(buf, key_format, ap);
    va_end(ap);
    if (n == 0) return false;
    return store_value(buf.c_str(), value);
  }

  bool KVS::has_fkey(const char *key_format, ...) {
    string buf;
    va_list ap;
    va_start(ap, key_format);
    int n = vsprintf(buf, key_format, ap);
    va_end(ap);
    if (n == 0) return false;
    return has_key(buf.c_str());
  }

  KVS *KVS::get_kvs_instance(const char *dir_path, const char *kvs_type) {
    KVS *kvs;

    if (strcasecmp(kvs_type, "bdb") == 0)
      kvs = create_KVS_BDB_Impl();
#ifndef IGNORE_QDBM
    else if (strcasecmp(kvs_type, "qdbm") == 0 || strcasecmp(kvs_type, "depot") == 0)
      kvs = create_KVS_DEPOT_Impl();
#endif
#ifndef IGNORE_NDBM
    else if (strcasecmp(kvs_type, "ndbm") == 0)
      kvs = create_KVS_NDBM_Impl();
#endif
#ifndef IGNORE_GDBM
    else if (strcasecmp(kvs_type, "gdbm") == 0)
      kvs = create_KVS_GDBM_Impl();
#endif
    else
      kvs = create_KVS_BDB_Impl();

    kvs->set_kvs_directory(dir_path);

    return kvs;
  }
};

namespace {

  /// KVSの基本機能を確認するツール
  class KVS_tool : uc::ELog {
    uc::Local_Text_Source *ts;

  public:
    KVS_tool(const char *path, const char *type) :
      db(uc::KVS::get_kvs_instance(path, type)),
      ts(create_Local_Text_Source())
    {
      init_elog("KVS_tool");
    }

    ~KVS_tool() { delete db; delete ts; }

    uc::KVS *db;
    int show_key_list();
    int dump_key_values();
    int import_key_values(const char *input_file);
  };

  /// 保持しているキー名を出力する
  int KVS_tool::show_key_list() {
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
  int KVS_tool::dump_key_values() {
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

  /// ファイルからKVデータを読込み、KVSに登録する
  int KVS_tool::import_key_values(const char *input_file) {

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

      if (!db->store_value(keybuf.c_str(), line)) {
	fprintf(stderr,"Break Loop!\n");
	rc = 1; break;
      }
      /*
	登録に失敗したら速やかに中断する
      */
    }

    return rc;
  }

};

extern "C" {

  // awk -F: '{print $1;print $0}' < /etc/passwd > work/user-by-loginname.dump
  // awk -F: '{print $3;print $0}' < /etc/passwd > work/user-by-pid.dump


  /// KVSオブジェクトを操作してみる
  static int cmd_kvs00(int argc, char **argv, const char *kvs_type) {

    const char *dir_path = "work";

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
      case 'T': kvs_type = optarg; break;
      case '?': return 1;
      }
    }
    auto_ptr<KVS_tool> tool(new KVS_tool(dir_path, kvs_type));

    uc::Text_Source::set_locale(lang);

    if (argc - optind < 1) {
      cerr << "usage: " << argv[0] << " [-c][-k] <dbname> [source-file]" << endl;
      /*
	データベースが指定されていないため、
	定義済みデータベースの一覧を出力する。
       */
      vector<string> list;
      
      tool->db->get_kvs_list(list);
      vector<string>::iterator it = list.begin();
      for (;it != list.end(); it++) {
	cout << *it << endl;
      }
      cout << list.size() << " databases found." << endl; 
      tool->db->show_report(stderr);

      return EXIT_FAILURE;
    }

    if (argc - optind <= 1) {
      /*
	渡されたパラメータが一つ（データベース名）であれば内容を出力する。
       */
      const char *dbname = argv[optind];
      if (!tool->db->open_kvs(dbname, "r")) return EXIT_FAILURE;

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
    if (!tool->db->open_kvs(dbname, mode)) return 1;

    return tool->import_key_values(input_file);
  }

  static int cmd_bdb(int argc, char **argv) {
    return cmd_kvs00(argc, argv, "bdb");
  }

  static int cmd_qdbm(int argc, char **argv) {
    return cmd_kvs00(argc, argv, "qdbm");
  }

  static int cmd_ndbm(int argc, char **argv) {
    return cmd_kvs00(argc, argv, "ndbm");
  }

  static int cmd_gdbm(int argc, char **argv) {
    return cmd_kvs00(argc, argv, "gdbm");
  }

};

// --------------------------------------------------------------------------------


#ifdef USE_SUBCMD
#include "subcmd.h"

subcmd kvs_cmap[] = {
  { "kvs", cmd_bdb, },
  { "bdb", cmd_bdb, },
  { "qdbm", cmd_qdbm, },
  { "depot", cmd_qdbm, },
  { "ndbm", cmd_ndbm, },
  { "gdbm", cmd_gdbm, },
  { 0 },
};

#endif

