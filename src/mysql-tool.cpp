/*! \file
 * \brief MySQL を操作する実験コード
 */

#include "uc/mysqlpp.hpp"
#include "uc/elog.hpp"

#include <map>
#include <string>
#include <vector>


using namespace std;

// --------------------------------------------------------
namespace {

};


extern "C" {
  /// 名前の一覧を出力する
  static void show_names(FILE *fout, const vector<string> &names, const char *sep = ", ") {
    vector<string>::const_iterator it = names.begin();
    const char *rsep = "";
    for (;it != names.end(); it++) {
      fprintf(fout,"%s%s",rsep, it->c_str());
      rsep = sep;
    }
    if (!names.empty()) fputc('\n',fout);
  }

  /// 設定の一覧を出力する
  static void show_params(FILE *fout, const map<string, string> &params, const char *sep = "=") {
    map<string,string>::const_iterator it = params.begin();
    for (;it != params.end(); it++) {
      fprintf(fout,"%s%s%s\n",it->first.c_str(),sep,it->second.c_str());
    }
  }

  static void show_db_info(FILE *fp, MYSQL *my)
  {
    if (!my) {
      fprintf(fp,"MySQL: no connection given.\n");
      return;
    }

    // MySQLクライアントライブラリ・バージョンを表す文字ストリング
    const char *client = mysql_get_client_info();
    // クライアントライブラリ・バージョンを表す整数
    unsigned long client_ver = mysql_get_client_version();
    // 現在の接続によって使われたプロトコルのバージョンを表す無署名の整数
    unsigned int proto = mysql_get_proto_info(my);

    // サーバのバージョンナンバーを表すストリング
    const char *server_info = mysql_get_server_info(my);
    // MySQLサーバー・バージョンを表す
    unsigned long server_ver = mysql_get_server_version(my);
    // サーバーホスト名と接続タイプを示す文字
    const char *host = mysql_get_host_info(my);

    fprintf(stderr,"MySQL Server %s:%lu (%s)\n", server_info, server_ver, host);
    fprintf(stderr,"MySQL Client %s:%lu, Protocol:%u\n", client, client_ver, proto);
    
    // クライアントがスレッドセーフとしてコンパイルされたかどうかを示す
    // クライアントがスレッドセーフの場合は 1、それ以外の場合は 0
    fprintf(stderr,"Thread Safe Library: %u: Thread ID: %lu\n", mysql_thread_safe(), mysql_thread_id(my));
  }

  void usage_my_account(const char *cmd) {
    fprintf(stderr,"usage: %s [-l] [<db-name> [<param-name> <value>]..]\n");
    fprintf(stderr,"usage: %s [-d] <db-name> ..\n");
  }

  enum { I = uc::ELog::I, W = uc::ELog::W, };

  /// MySQLの接続情報の登録と確認
  int cmd_my_account(int argc, char **argv) {
    int opt;
    bool show_db_list = false, drop_db = false;

    while ((opt = getopt(argc,argv,"dl")) != -1) {
      switch(opt) { 
      case 'l': show_db_list = true; break;
      case 'd': drop_db = true; break;
      case '?': usage_my_account(argv[0]); return 1;
      }
    }

    auto_ptr<mysqlpp::Connection_Manager> cm(mysqlpp::Connection_Manager::get_instance());
    
    if (optind == argc || show_db_list) {
      // 登録済みアカウントの一覧
      vector<string> names;
      cm->get_db_names(names);
      show_names(stdout, names);
      elog(I, "%d accounts aviable.\n", names.size());
      return 0;
    }

    if (drop_db) {
      for (int i = optind; i < argc; i++) {
	cm->drop_db_parameter(argv[i]);
      }
      return 0;
    }

    const char *alias = argv[optind];
    map<string, string> params;

    if (optind + 1 == argc) {
      // 接続情報を表示
      if (!cm->fetch_db_parameter(alias, params)) {
	elog("%s: no such db alias.\n", alias);
	return 1;
      }
      show_params(stdout, params);
      elog(I, "%s has %d parameters.\n", alias, params.size());
      return 0;
    }

    for (int i = optind + 1; i + 1 < argc; i += 2) {
      params.insert(map<string,string>::value_type(argv[i],argv[i + 1]));
    }
    if(0)
    show_params(stdout, params);
    cm->store_db_parameter(alias, params);
    elog(I, "%s store %d parameters.\n", alias, params.size());

    mysqlpp::Connection *conn = cm->get_Connection(alias);
    if (!conn) {
      elog("%s cannot get connection\n", alias);
      return 1;
    }

    return 0;
  }

  void usage_my_report(const char *cmd) {
    fprintf(stderr,"usage: %s [-l] <db-name>\n");
  }

  /// MySQLの基本操作
  int cmd_my_report(int argc, char **argv) {
    int opt;
    bool show_db_list = false, drop_db = false;

    while ((opt = getopt(argc,argv,"dl")) != -1) {
      switch(opt) { 
      case 'l': show_db_list = true; break;
      case '?': usage_my_report(argv[0]); return 1;
      }
    }

    auto_ptr<mysqlpp::Connection_Manager> cm(mysqlpp::Connection_Manager::get_instance());
    
    if (optind == argc || show_db_list) {
      // 登録済みアカウントの一覧
      vector<string> names;
      cm->get_db_names(names);
      show_names(stdout, names);
      elog(I, "%d accounts aviable.\n", names.size());
      return 0;
    }

    const char *alias = argv[optind];

    mysqlpp::Connection *conn = cm->get_Connection(alias);
    if (!conn) {
      elog("%s cannot get connection\n", alias);
      return 1;
    }

    show_db_info(stdout, conn->handler());

    vector<string> names;

    printf("-------- database names\n");
    char *pattern = "%";
    conn->fetch_db_names(names, pattern);
    show_names(stdout, names);

    printf("-------- table names\n");
    conn->fetch_table_names(names, pattern);
    show_names(stdout, names);

    vector<string> cols;
    for (int i = 0, n = names.size(); i < n; i++) {
      const char *tbl = names.at(i).c_str();

      printf("-------- coluns for %s\n", tbl);
      conn->fetch_column_names(cols, tbl, pattern);
      show_names(stdout, cols);
    }


    printf("-------- \n");

    return 0;
  }



};

// --------------------------------------------------------------------------------


#ifdef USE_SUBCMD
#include "subcmd.h"

subcmd mysql_cmap[] = {
  { "mysql-account", cmd_my_account, },
  { "mysql-query", cmd_my_report, },
  { 0 },
};

#endif


