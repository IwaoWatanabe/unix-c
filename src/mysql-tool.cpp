/*! \file
 * \brief MySQL を操作する実験コード
 */

#include "uc/csv.hpp"
#include "uc/elog.hpp"
#include "uc/mysqlpp.hpp"
#include "uc/text-reader.hpp"

#include <cstdio>
#include <cstring>
#include <map>
#include <memory>
#include <string>
#include <vector>



extern uc::Local_Text_Source *create_Local_Text_Source();

using namespace std;

// --------------------------------------------------------
namespace {

  static MYSQL_BIND EMPTY_BIND;

  // 読み込んだデータを別のファイルに書き出す
  class DumpCSVReader : public uc::CSV_Reader, public uc::ELog {
    string outfile;
    FILE *fp;

  protected:
    void out_csv(const char **row, FILE *fp);

  public:
    DumpCSVReader(const char *fname) : outfile(fname), fp(0) {}

    bool begin_read_csv();
    int read_csv(const char **row, int columns);
    void end_read_csv(bool cancel);
  };

  bool
  DumpCSVReader::begin_read_csv() {
    if (outfile.empty()) return false;

    fp = fopen(outfile.c_str(),"a+");
    if (!fp) {
      elog("fopen %s failed:(%d) %s\n", outfile.c_str(), errno, strerror(errno));
      return false;
    }
    return true;
  }

  void
  DumpCSVReader::out_csv(const char **row, FILE *fp)
  {
    char *sep = "";

    for (; *row; row++) {
      char *p = strchr(*row, '"');
      if (!p) p = strchr(*row, '\n');

      if(!p)
	fprintf(fp,"%s%s",sep,*row);
      else {
	fputs(sep,fp);
	fputc('\"',fp);
	for (const char *t = *row; *t; t++) {
	  fputc(*t,fp);
	  if (*t == '"') fputc(*t,fp);
	}
	fputc('\"',fp);
      }
      sep = ",";
    }
    fputc('\n',fp);
  }

  int
  DumpCSVReader::read_csv(const char **row, int columns) {
    if (!fp) return 1;
    out_csv(row, fp);
    return 0;
  }

  void
  DumpCSVReader::end_read_csv(bool cancel) {
    if (!fp) return;
    if (fclose(fp) != 0) {
      elog(W, "fclose %s failed:(%d) %s\n", outfile.c_str(), errno, strerror(errno));
    }
  }


  // 読み込んだデータをMySQLのテーブルに格納する
  /*
    CSVフィールドとテーブルのカラムの対応は定義ファイルで渡す
    定義ファイルは、サフィックスを取り除いて、そのままテーブル名として利用される。
   */
  class StoreTable_CSV_Reader : public DumpCSVReader {
    string ctrl_file;

    /// 取り込み対象とするCSVカラム位置リストが格納される
    vector<int> poslist;

    mysqlpp::Connection *conn;
    mysqlpp::Cursor *cur;
    MYSQL_BIND *params;
    unsigned long *column_length;

    void show_cmap(map<string, int> &cmap, FILE *fp);
    void load_column_list(const char *fname, map<string, int> &cmap);
    void build_insert_query(string &qbuf, 
			    const vector<string> &cnames, 
			    const map<string, int> &column_map);
  public:
    int counter, max_records;

    StoreTable_CSV_Reader(const char *fname, mysqlpp::Connection *port)
      : DumpCSVReader(""), ctrl_file(fname), conn(port), counter(0), max_records(200000) { }

    bool begin_read_csv();
    int read_csv(const char **row, int columns);
    void end_read_csv(bool cancel);
  };

  /// CSVの読み位置と、テーブルのカラムの対応を出力する
  void
  StoreTable_CSV_Reader::show_cmap(map<string, int> &cmap, FILE *fp)
  {
    fprintf(fp,"------- load map -------\n");

    map<string, int>::iterator it = cmap.begin();
    while (it != cmap.end()) {
      fprintf(fp,"%s: %d\n", (it->first).c_str(), it->second);
      it++;
    }
  }

  /// テキストの前後の空白文字を詰める

  char *trim(char *t) {
      while(isspace(*t)) *t++;
      /*
	先頭の空白文字をスキップ
       */

      size_t len = strlen(t);
      char *u = t + len - 1;
      while (u > t && isspace(*u)) u--;
      u[1] = 0;
      /*
	末尾の空白文字を詰める
       */
      return t;
  }

  /// テーブルに取り込むカラム名を記述したファイルを読込む
  void
  StoreTable_CSV_Reader::load_column_list(const char *cfname, map<string, int> &cmap)
  {
    auto_ptr<uc::Local_Text_Source> src(new uc::Local_Text_Source());

    if (!src->open_read_ffile(cfname)) return;

    cmap.clear();

    /// CSVで読み込むデータのカラム名を保持する
    vector <string> flist;

    char *t;

    // 行単位でカラム名を記述する
    while ((t = src->read_line()) != NULL) {
      if (*t == '#') continue;
      t = trim(t);
      if (!*t) continue; // 空行はスキップ

      flist.push_back(t);
    }

    src->close_source();

    // カラム名と桁位置の対応が記録される
    //fprintf(stderr,"------- cols -------\n");

    for (int i = 0, n = flist.size(); i < n; i++) {
      cmap.insert( map<string, int>::value_type( flist[i], i ));
      //fprintf(stderr,"%d:%s\n", i, flist[i].c_str());
    }

    show_cmap(cmap, stderr);
  }

  void
  StoreTable_CSV_Reader::build_insert_query(string &qbuf, 
					    const vector<string> &cnames, 
					    const map <string, int> &column_map) {
    poslist.clear();

    const char *tp = "(";

    for (int i = 0,n = cnames.size(); i < n; i++) {
      map<string, int>::const_iterator it = column_map.find(cnames[i]);

      if (it != column_map.end()) {
	fprintf(stderr,"%s => %d\n", cnames[i].c_str(), it->second);
	qbuf += tp;
	qbuf += cnames[i];
	tp = ",";
	poslist.push_back(it->second);
      }
    }

    tp = ") VALUES (";

    for (int i = 0, n = poslist.size(); i < n; i++) {
      qbuf += tp;
      qbuf += "?";
      tp = ",";
    }

    qbuf += ")";
  }


  // データ移行のカラム・マップ定義ファイルを読み込む
  bool
  StoreTable_CSV_Reader::begin_read_csv()
  {
    /// CSVで読み込むデータ読み込位置を格納
    /// KEYはフィールド名。VALUEはそのフィールドのカラム位置
    map<string, int> src_column_map;

    // 対象CSVのフィールド名定義を読み込む
    load_column_list(ctrl_file.c_str(), src_column_map);

    if (src_column_map.empty()) {
      fprintf(stderr,"no control entry\n");
    }

    char *tp0 = strdup(ctrl_file.c_str()), *tp = basename(tp0), *p = strchr(tp, '.');
    string tbl(tp, p - tp);

    free(tp0);


    fprintf(stderr,"=======\n");

    fprintf(stderr,"target tables: %s\n", tbl.c_str());

    // 現行のテーブルの定義済カラム名を入手
    vector<string> cnames;

    conn->fetch_column_names(cnames, tbl.c_str());

    if (cnames.empty()) {
      fprintf(stderr,"ERRROR: not exists or cannot fetch column name\n");
      return false;
    }

    /// 取り込みテーブルのデータを削除
    string qbuf;
    qbuf = "DELETE FROM ";
    if (conn->query(qbuf.append(tbl))) {
      elog(I, "%lu recored deleted before import.", conn->affected_rows());
    }

    /// SQLの組み立て
    qbuf = "INSERT INTO ";

    build_insert_query(qbuf.append(tbl), cnames, src_column_map);
    if (poslist.empty()) {
      fprintf(stderr,"no match column entry.\n");
      return false;
    }

    /// カーソルの準備

    string qname(tbl);
    cur = conn->find_cursor(qname.append("-INSERT").c_str(), &qbuf);
    if (!cur) return false;

    int n_params = cur->param_count();
    params = new MYSQL_BIND[n_params];

    for (int i = 0; i < n_params; i++) {
      params[i] = EMPTY_BIND;
      params[i].buffer_type= MYSQL_TYPE_STRING;
      params[i].buffer= 0;
      params[i].is_null= 0;
      params[i].length = 0;
    }

    counter = 0;

    conn->set_autocommit(false);

    return true;
  }

  int
  StoreTable_CSV_Reader::read_csv(const char **row, int columns)
  {
    // ここでは bind して executeの繰り返し

    int n_params = cur->param_count();

    for (int i = 0; i < n_params; i++) {
      int pos = poslist[i];

      size_t len = strlen(row[pos]);
      params[i].buffer = (void *)row[pos];
      params[i].buffer_length = len;
    }

    if (counter % 2000 == 0) {
      conn->commit();
      putc('.', stderr);
    }

    if (!cur->bind(params)) return 1;

    if (!cur->execute(params)) {
      // TODO: reject データをどうしよう
      return 1;
    }

    return ++counter < max_records ? 0 : 1;
  }

  void
  StoreTable_CSV_Reader::end_read_csv(bool cancel)
  {
    if (cancel)
      conn->rollback();
    else
      conn->commit();

    // 後始末
    delete [] params;
  }

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


  /// CSVを読み込んでそのまま別ファイルに出力する
  static int
  cmd_csv01(int argc, char **argv)
  {
    const char *csv_file = getenv("CSV_IN");
    if (!csv_file) csv_file = "work/aa.csv";

    const char *csv_out_file = getenv("CSV_OUT");
    if (!csv_out_file) csv_out_file = "work/bb.csv";

    load_csv(csv_file, new DumpCSVReader(csv_out_file));
    return EXIT_SUCCESS;
  }

  /// CSVを読み込んでテーブルに登録する
  static int
  cmd_csv02(int argc, char **argv)
  {
    auto_ptr<mysqlpp::Connection_Manager> cm(mysqlpp::Connection_Manager::get_instance());

    if (optind == argc) {
      puts("usage");
      return EXIT_FAILURE;
    }

    const char *alias = argv[optind];

    mysqlpp::Connection *conn = cm->get_Connection(alias);
    if (!conn) {
      elog("%s cannot get connection\n", alias);
      return EXIT_FAILURE;
    }

    conn->set_character_set("utf8");

    /// 郵政省のデータベースを読み込む
    const char *csv_file = getenv("KEN_ALL");
    if (!csv_file) csv_file = "work/KEN_ALL-utf8.csv";

    /// カラム名の対応
    /// ファイル名がテーブル名として利用される。
    const char *column_file = getenv("COLS");
    if (!column_file) column_file = "work/wdzip03e.cols";

    load_csv(csv_file, new StoreTable_CSV_Reader(column_file, conn));

    return EXIT_SUCCESS;
  }

};

// --------------------------------------------------------------------------------


#ifdef USE_SUBCMD
#include "subcmd.h"

subcmd mysql_cmap[] = {
  { "csv-load", cmd_csv01, },
  { "csv-import", cmd_csv02, },
  { "mysql-account", cmd_my_account, },
  { "mysql-query", cmd_my_report, },
  { 0 },
};

#endif


