// -*- mode: C++ -*-

/*! \file
 * \brief MySQL C APIのリソース管理をサポートする

参考URL

http://dev.mysql.com/doc/refman/5.1/ja/c-api-function-overview.html

http://search.net-newbie.com/mysql41/clients.html
 */

#include "uc/mysqlpp.hpp"
#include "uc/elog.hpp"

#include <assert.h>
#include <map>

using namespace std;

namespace {

  /// DBの接続情報を保持する
  class My_DB_Info : public mysqlpp::DB_Info {
    int name,user,passwd,socket,host,port;
    string sbuf;

    int offset(const char *tt) {
      if (!tt) return 0;
      size_t of = sbuf.size();
      sbuf.append(tt).append('\0');
      return of;
    }

  public:
    My_DB_Info(DB_Info &di) : sbuf("\0") {
      name = offset(di.get_db_name());
      user = offset(di.get_db_user());
      passwd = offset(di.get_db_password());
      socket = offset(di.get_db_socket_path());
      host = offset(di.get_db_host());
      port = di.get_db_port();
    }

    My_DB_Info() 
      : name(0),user(0),passwd(0),socket(0),host(0),port(0), sbuf("\0") { }

    virtual ~My_DB_Info() { }
    virtual const char *get_db_name() { return sbuf.data() + name; }
    virtual const char *get_db_user() { return sbuf.data() + user; }
    virtual const char *get_db_password() { return sbuf.data() + passwd; }
    virtual const char *get_db_socket_path() { return sbuf.data() + socket; }
    virtual const char *get_db_host() { return sbuf.data() + host; }
    virtual int get_db_port() { return port; }
  };

  class My_Cursor_Impl : public mysqlpp::Cursor, uc::ELog {
  protected:
    MYSQL_STMT *cur;
    mysqlpp::Connection *ref;
    bool truncated, verbose;

  public:
    My_Cursor_Impl(MYSQL_STMT *stmt, mysqlpp::Connection *conn) 
      : mysqlpp::Cursor(stmt, conn) { }

    virtual ~My_Cursor_Impl() { free(); }
    virtual void free();
    virtual void release();
    virtual unsigned long affected_rows();
    virtual unsigned long insert_id();
    virtual bool prepare(const std::string &query_text);
    virtual void free_result();
    virtual int param_count();
    virtual bool bind(MYSQL_BIND *bind);
    virtual int field_count();
    virtual bool bind_result(MYSQL_BIND *bind);
    virtual bool execute(bool populate = false);
    virtual bool fetch();
    virtual MYSQL_ROW_OFFSET row_tell();
    virtual MYSQL_ROW_OFFSET row_seek(MYSQL_ROW_OFFSET offset);
  };


  /// 関連リソースを解放する
  void My_Cursor_Impl::free() {
    if (cur) {
      int rc = mysql_stmt_close(cur);
      if (rc) elog(W, "while statemet closing:(%u)\n", rc);
    }
    cur = 0;
  }

  void My_Cursor_Impl::release() {
    elog(T, "release notify: %p", this);
    free();
  }

  /// クエリにより影響があった行数を入手する
  unsigned long My_Cursor_Impl::affected_rows() {
    if (cur) return (unsigned long)mysql_stmt_affected_rows(cur);
    return (unsigned long)-1;
  }
  
  /// AUTO_INCREMENT インデックスを利用した最後のクエリが生成したIDを返す
  unsigned long My_Cursor_Impl::insert_id() {
    if (cur) return (unsigned long)mysql_stmt_insert_id(cur);
    return 0lu;
  }
  
  /// SQLテキストを設定する
  bool My_Cursor_Impl::prepare(const std::string &query_text) {
    if (!cur) return false;

    int rc = mysql_stmt_prepare(cur, query_text.c_str(), query_text.size());
    if (rc != 0) {
      elog("prepare failure:(%u):%s\n", mysql_stmt_errno(cur), mysql_stmt_error(cur));
      elog(T, "\t%s\n", query_text.c_str());
    }
    return rc == 0;
  }

  /// 結果セットのリソースを解放する
  void My_Cursor_Impl::free_result() {
    if (!cur) return;
    my_bool rc = mysql_stmt_free_result(cur);
    if (rc == 0) return;
    elog(W,"free result failure:(%u):%s\n", mysql_stmt_errno(cur), mysql_stmt_error(cur));
  }

  /// 必要とするパラメータ数
  int My_Cursor_Impl::param_count() {
    if (cur) return mysql_stmt_param_count(cur);
    return 0;
  }

  /// パラメータをバインドする
  bool My_Cursor_Impl::bind(MYSQL_BIND *bind) {
    if (cur == 0) return false;

    my_bool rc = mysql_stmt_bind_param(cur, bind);
    if (rc == 0) return true;
    elog("bind failure:(%u):%s\n", mysql_stmt_errno(cur), mysql_stmt_error(cur));
    return false;
  }

  /// 結果セットの構成カラム数を得る
  int My_Cursor_Impl::field_count() {
    if (!cur) return 0;
    return mysql_stmt_field_count(cur);
  }

  /// 結果セットの受け取り領域をバインドする
  bool My_Cursor_Impl::bind_result(MYSQL_BIND *bind) {
    if (cur == 0) return false;
    my_bool rc = mysql_stmt_bind_result(cur, bind);
    if (rc == 0) return true;

    elog("bind result failure:(%u):%s\n", mysql_stmt_errno(cur), mysql_stmt_error(cur));
    return false;
  }
  
  /// クエリを実行する 
  bool My_Cursor_Impl::execute(bool populate) {
    if (!cur) return false;

    int rc = mysql_stmt_execute(cur);
    if (rc != 0) {
      // 成功した場合ゼロ。エラーが起こった場合、ゼロ以外。
      elog("execute failure:(%u):%s\n", mysql_stmt_errno(cur), mysql_stmt_error(cur));
      return false;
    }
    if (!populate) return true;

    rc = mysql_stmt_store_result(cur);
    // 成功した場合ゼロ。エラーが起こった場合、ゼロ以外。
    if (rc == 0) return true;

    elog("store result failure:(%u):%s\n", mysql_stmt_errno(cur), mysql_stmt_error(cur));
    return false;
  }

  /// 行データを取り寄せる
  bool My_Cursor_Impl::fetch() {
    if (!cur) return false;

    int rc = mysql_stmt_fetch(cur);
    switch(rc) {
    case MYSQL_DATA_TRUNCATED:
      if (!truncated) {
	elog(W, "fetch: data truncated.\n");
	truncated = true;
      }
    case 0:
      return true;
    case 1:
      elog("fetch failure:(%u):%s\n", mysql_stmt_errno(cur), mysql_stmt_error(cur));
    case MYSQL_NO_DATA:
      return false;
    }

    elog("unkown fetch code:(%d)\n", rc);
    return false;
  }

  /// 行位置を確認する
  MYSQL_ROW_OFFSET My_Cursor_Impl::row_tell() {
    // mysql_stmt_store_result()の後にだけ使える
    if (!cur) return 0;
    return mysql_stmt_row_tell(cur);
  }

  /// 行位置を移動する
  MYSQL_ROW_OFFSET My_Cursor_Impl::row_seek(MYSQL_ROW_OFFSET offset) {
    if (!cur) return 0;
    return mysql_stmt_row_seek(cur,offset);
  }

  // --------------------------------------------------------

  /// MySQLの接続情報とそれに関連するリソースを管理する。
  /** 
      実装クラスはコンテナが提供する。
      このインスタンス単位でMySQLサーバに接続する。
   */
  class My_Connection_Impl : public mysqlpp::Connection, uc::ELog {
  private:
    /// 接続情報
    mysqlpp::DB_Info *cached_info;

    /// 名前で引けるCurosr
    map<string, My_Cursor_Impl *>stat;

    /// 名前で引ける Resource
    map<string, mysqlpp::Resource *>resources;

    bool verbose;
    virtual void show_rd(MYSQL_ROW rd, unsigned long *len, int cols);
    virtual bool connect_internal(MYSQL *conn, mysqlpp::DB_Info *info);
    virtual void free_cursor_all();

  public:
    My_Connection_Impl();
    virtual ~My_Connection_Impl();
    virtual bool connect(mysqlpp::DB_Info *info);
    virtual MYSQL *handler();
    virtual void disconnect();
    virtual bool query(const std::string &query_text, bool store = false);
    virtual mysqlpp::Result *get_result();
    virtual unsigned long insert_id();
    virtual unsigned long affected_rows();
    virtual bool select_db(const char *dbname);
    virtual void get_db_names(std::vector<std::string> &name_list);
    virtual void set_character_set(const char *names);
    virtual void set_autocommit(bool flag);
    virtual void commit();
    virtual void rollback();
    virtual void escape_string(std::string &buf, std::string &text);
    virtual int warning_count();
    virtual void fetch_table_names(std::vector<std::string> &cnames, const char *tbl = "%");
    virtual void fetch_column_names(std::vector<std::string> &cnames, const char *tbl, const char *wild = "%");
    virtual bool add_resource(const char *name, mysqlpp::Resource *res);
    virtual void remove_resourece(const char *name);
    virtual mysqlpp::Resource *find_resource(const char *name);
    virtual void get_resource_names(std::vector<std::string> &cnames);
    virtual mysqlpp::Cursor *find_cursor(const char *query_name, const std::string *query_text = 0);
    virtual void get_cursor_names(std::vector<std::string> &name_list);
  };


  My_Connection_Impl::My_Connection_Impl()
    : cached_info(0) { }

  My_Connection_Impl::~My_Connection_Impl() {
    disconnect();
    elog(T,"Connection deleting: %p\n", this);
  }

  bool
  My_Connection_Impl::connect_internal(MYSQL *conn, mysqlpp::DB_Info *info)
  {
    unsigned long client_flag = 0;

    const char *host = info->get_db_host();
    const char *name = info->get_db_name();
    const char *user = info->get_db_user();
    const char *pass = info->get_db_password();
    const char *socket = info->get_db_socket_path();
    int port = info->get_db_port();
    
    MYSQL *res = 
      mysql_real_connect(conn, host, user, pass, name, port, socket, client_flag);
    if (!res) {
      elog("mysql:%s@%s:%d/%s;socket=%s;flag=%#x (%u):%s", 
	   user, host, port, name, socket, client_flag,
	   mysql_errno(res), mysql_error(res) );
      return false;
    }
    return true;
  }

  bool
  My_Connection_Impl::connect(mysqlpp::DB_Info *info)
  {
    My_DB_Info *cp = new My_DB_Info(*info);
    MYSQL *res = mysql_init(0);

    if (!connect_internal(res, cp)) {
      /*
	接続情報に誤りがある場合は記憶しない。
      */
      mysql_close(res);
      return false;
    }

    disconnect();
    delete cached_info;
    /*
      前の接続あれば解除する。
    */

    cached_info = info;
    conn = res;

    elog(T, "connected: %p\n", this);
    return true;
  }

  MYSQL *
  My_Connection_Impl::handler()
  {
    assert(cached_info);

    if (conn) {
      // サーバとの接続が切れているときは、mysql_close()して接続しなおす
      // 違うDBを参照している可能性があるため、カレントDBを変えておく
      if (mysql_ping(conn) == 0 && 
	  mysql_select_db(conn, cached_info->get_db_name()) == 0) return conn;

      const char *host = mysql_get_host_info(conn);
      elog(W, "dead mysql connection %s closing.\n", host);
      if (host) mysql_close(conn);
    }

    MYSQL *res = mysql_init(conn);

    if (!connect_internal(res, cached_info)) { return 0; }
    set_character_set("utf8");
    conn = res;

    return res;
  }

  bool
  My_Connection_Impl::select_db(const char *dbname)
  {
    if (!conn) return false;

    if (mysql_select_db(conn, dbname) == 0) {
      elog(I, "db changed: %s", dbname);
      return true;
    }
    elog("cannot change db:(%u): %s\n",
	 mysql_errno(conn), mysql_error(conn) );
    return false;
  }

  /// テーブル名を入手する
  void
  My_Connection_Impl::fetch_table_names(vector<string> &cnames, const char *tbl)
  {
    cnames.clear();

    MYSQL_RES *rs = mysql_list_tables(conn, tbl);
    if (!rs) {
      elog(W, "cannot fetch table names:(%u): %s\n",
	   mysql_errno(conn), mysql_error(conn) );
      return;
    }

    MYSQL_ROW row;

    while ((row = mysql_fetch_row(rs))) {
      unsigned long *lengths = mysql_fetch_lengths(rs);
      cnames.push_back(string(row[0], lengths[0]));
    }

    mysql_free_result(rs);
  }

  void My_Connection_Impl::show_rd(MYSQL_ROW rd, unsigned long *len, int cols) {
    for (int i = 0; i < cols; i++) {
      printf("\t%.*s", (int)(len[i] + 2), rd[i]);
    }
    puts("");
  }

  /// テーブルのカラム名を入手する
  void
  My_Connection_Impl::fetch_column_names(vector<string> &cnames, const char *tbl, const char *wild)
  {
    cnames.clear();

#if 0
    MYSQL_RES *rs = mysql_list_fields(conn, tbl, wild);
#else
    string qbuf = "SHOW COLUMNS FROM ";
    qbuf += tbl;
    if (wild) {
      qbuf += " LIKE \"";
      qbuf += wild;
      qbuf += "\"";
    }
    if (mysql_query(conn, qbuf.c_str()) != 0) {
      elog("cannot execute to fetch column names: (%u):%s\n",
	   mysql_errno(conn), mysql_error(conn));
      return;
    }
    if (verbose)
      elog(T, "column-list query was %s\n",qbuf.c_str());

    MYSQL_RES *rs = mysql_store_result(conn);
#endif

    if (!rs) {
      elog(W, "cannot fetch column names:(%u): %s\n",
	   mysql_errno(conn), mysql_error(conn) );
      return;
    }

#if 0
    MYSQL_FIELD *fd;

    while ((fd = mysql_fetch_field(rs)) != NULL) { 
      cnames.push_back(fd->name);
    }

#else
    MYSQL_ROW row;

    while ((row = mysql_fetch_row(rs))) {
      unsigned long *lengths = mysql_fetch_lengths(rs);
      cnames.push_back(string(row[0], lengths[0]));
    }

    string rbuf;
    const char *rep = "";
    vector<string>::iterator it = cnames.begin();
    for (;it != cnames.end(); it++) {
      rbuf += rep;
      rbuf += it->c_str();
      rep = ", ";
    }
    if (verbose)
      elog(T, "column-list for %s was %s\n", tbl, rbuf.c_str());

#endif

    mysql_free_result(rs);
  }

  /**
     データベースとの接続に関連するリソースを解放し接続を閉じます。
  */
  void 
  My_Connection_Impl::disconnect()
  {
    elog(T, "disconnect called: %p\n", this);
    if (!conn) return;
    
    if (last_result) mysql_free_result(last_result);
    last_result = 0;

    const char *host_info = mysql_get_host_info(conn);
    elog(I, "mysql closing: %s\n", host_info);
    mysql_close(conn);
    conn = 0;
  }

  unsigned long
  My_Connection_Impl::affected_rows()
  {
    if (conn) return (unsigned long)mysql_affected_rows(conn);
    return (unsigned long)-1;
  }

  unsigned long
  My_Connection_Impl::insert_id()
  {
    if (conn) return (unsigned long)mysql_insert_id(conn);
    return 0lu;
  }

  /// クエリを実行する
  bool
  My_Connection_Impl::query(const string &query_text, bool store)
  {
    if (mysql_query(conn, query_text.c_str()) != 0) {
      elog("Illegal Query:(%u):%s\n", mysql_errno(conn), mysql_error(conn));
      return false;
    }

    if (last_result) mysql_free_result(last_result);

    last_result = store ? mysql_use_result(conn) : mysql_store_result(conn);
    if (!last_result || mysql_num_fields(last_result) == 0) return false;
    return true;
  }

  /// 結果セットを入手する
  mysqlpp::Result *
  My_Connection_Impl::get_result()
  {
    if (!last_result) return NULL;
    mysqlpp::Result *rs = new mysqlpp::Result(last_result);
    last_result = 0;
    return rs;
  }


  /// Cursor を名前を指定して入手する
  mysqlpp::Cursor *
  My_Connection_Impl::find_cursor(const char *name, const std::string *query_text)
  {
    // 名前に結びついたステートメントを入手する
    map<string, My_Cursor_Impl *>::iterator it = stat.find(name);
    if (it == stat.end()) {
      MYSQL_STMT *st = mysql_stmt_init(conn);
      if (!st) {
	elog("mysql statemet %s finding:(%u):%s", 
	     name, mysql_errno(conn), mysql_error(conn));
	return 0;
      }

      if (query_text) {
	int rc = mysql_stmt_prepare(st, query_text->c_str(), query_text->size());
	if (rc != 0) {
	  elog("prepare failure:(%u):%s\n", mysql_stmt_errno(st), mysql_stmt_error(st));

	  rc = mysql_stmt_close(st);
	  if (rc) elog(W, "while statemet closing:(%u)\n", rc);
	  return 0;
	}
      }

      // 登録する
      My_Cursor_Impl *cur = new My_Cursor_Impl(st, this);
      stat.insert(map<string, My_Cursor_Impl *>::value_type(name, cur));
      return cur;
    }
    return (*it).second;
  }

  void
  My_Connection_Impl::get_cursor_names(std::vector<std::string> &name_list)
  {
    name_list.clear();
    map<string, My_Cursor_Impl *>::iterator it = stat.begin();
    while (it != stat.end()) {
      name_list.push_back((*it).first);
      it++;
    }
  }

  void
  My_Connection_Impl::free_cursor_all()
  {
    elog(T, "free_cursor_all called.");

    // MAPに確保されているステートメントを解放する
    map<string, My_Cursor_Impl *>::iterator it = stat.begin();
    while (it != stat.end()) {
      mysqlpp::Cursor *cur = (*it).second;
      delete cur;
      it++;
    }
    stat.clear();
  }

  /// トランザクション・モードを切り替える
  void
  My_Connection_Impl::set_autocommit(bool flag)
  {
    if (!conn) return;
    my_bool rc = mysql_autocommit(conn, flag ? 0 : 1);
    if (rc)
      elog(W, "rollbacl failed.:(%u):%s\n", mysql_errno(conn), mysql_error(conn));
  }

  void
  My_Connection_Impl::commit()
  {
    if (!conn) return;
    my_bool rc = mysql_commit(conn);
    // 正常に動作した場合は 0。エラーが発生した場合は 0 以外。
    if (rc)
      elog(W, "commit failed.:(%u):%s\n", mysql_errno(conn), mysql_error(conn));
  }

  void
  My_Connection_Impl::rollback()
  {
    if (!conn) return;

    my_bool rc = mysql_rollback(conn);
    // 正常に動作した場合は 0。エラーが発生した場合は 0 以外。
    if (rc)
      elog(W, "rollbacl failed.:(%u):%s\n", mysql_errno(conn), mysql_error(conn));
  }

  int
  My_Connection_Impl::warning_count()
  {
    if (!conn) return 0;
    return mysql_warning_count(conn);
  }

  void
  My_Connection_Impl::escape_string(string &buf, string &text)
  {
    if (!conn) return;
    char *wbuf = new char[text.size() * 2 + 1];
    size_t len = mysql_real_escape_string(conn, wbuf, text.c_str(), text.size());
    buf.append(wbuf, len);
    delete wbuf;
  }

  void My_Connection_Impl::get_db_names(vector<string> &name_list)
  {
    
  }

  void
  My_Connection_Impl::set_character_set(const char *name)
  {
    if (!conn) return;
    if (0 != mysql_set_character_set(conn, name))
      elog(W, "mysql set names %s: %s", name, mysql_error(conn));
  }

  bool
  My_Connection_Impl::add_resource(const char *name, mysqlpp::Resource *res)
  {
    map<string, mysqlpp::Resource *>::iterator it = resources.find(name);
    if (it == resources.end()) {
      resources.insert(map<string, mysqlpp::Resource *>::value_type(name, res));
      return true;
    }
    return false;
  }

  void
  My_Connection_Impl::remove_resourece(const char *name)
  {
    map<string, mysqlpp::Resource *>::iterator it = resources.find(name);
    if (it == resources.end()) return;
    // TODO: 削除する
  }

  mysqlpp::Resource *
  My_Connection_Impl::find_resource(const char *name)
  {
    map<string, mysqlpp::Resource *>::iterator it = resources.find(name);
    if (it == resources.end()) return 0;
    return it->second;
  }

  void
  My_Connection_Impl::get_resource_names(std::vector<std::string> &names)
  {
    names.clear();
    map<string, mysqlpp::Resource *>::iterator it = resources.begin();
    for (; it != resources.end(); it++) {
      names.push_back(it->first);
    }
  }

  static void show_db_info(FILE *fp, MYSQL *my)
  {
    if (!my) return;

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

  // --------------------------------------------------------
};

#include "uc/kvs.hpp"

namespace {

  /// DBの接続情報を保持する
  struct My_DB_Info02 : public mysqlpp::DB_Info {
    const char *name,*user,*passwd,*socket,*host,*port;
    My_DB_Info02() { name = user = passwd = socket = host = port = 0; }
    virtual const char *get_db_name() { return name; }
    virtual const char *get_db_user() { return user; }
    virtual const char *get_db_password() { return passwd; }
    virtual const char *get_db_socket_path() { return socket; }
    virtual const char *get_db_host() { return host; }
    virtual int get_db_port() { return atoi(port); }
  };

  /// 素朴な　Connection_Manager　の実装
  class Connection_Manager_Impl : public mysqlpp::Connection_Manager, uc::ELog {
    /// 設定情報を保存するkvs
    uc::KVS *db;
    /// 接続インスタンスを保持する
    map<string,mysqlpp::Connection *>cmap;

  public:
    Connection_Manager_Impl() {
      init_elog("Connection_Manager_Impl");
      const char *path = "work", *type = "bdb";
      db = uc::KVS::get_kvs_instance(path, type);
    }

    ~Connection_Manager_Impl() {
      close_all_connection();
      delete db;
    }

    virtual void get_db_names(vector<string> &name_list);
    virtual void store_db_parameter(const char *name, const map<string,string> &params);
    virtual void fetch_db_parameter(const char *name, map<string,string> &params);
    virtual mysqlpp::DB_Info *get_DB_Info(const char *name);
    virtual mysqlpp::Connection *get_Connection(const char *name = 0);
    virtual const char *get_last_connection();
    virtual void close_all_connection();
  };

  /// 登録済み接続名の入手
  void
  Connection_Manager_Impl::get_db_names(vector<string> &name_list)
  {
  }

  /// 接続情報の保存
  void
  Connection_Manager_Impl::store_db_parameter(const char *name, const map<string,string> &params)
  {
  }

  /// 接続情報の入手
  void
  Connection_Manager_Impl::fetch_db_parameter(const char *name, map<string,string> &params)
  {
  }

  /// 接続情報の入手
  mysqlpp::DB_Info *
  Connection_Manager_Impl::get_DB_Info(const char *name)
  {
    My_DB_Info02 info;
    return new My_DB_Info(info);
  }

  /// 接続済みのDB接続を得る
  mysqlpp::Connection *
  Connection_Manager_Impl::get_Connection(const char *name)
  {
    map<string,mysqlpp::Connection *>::iterator it = cmap.find(name);
    if (it != cmap.end()) { return it->second; }
    
    mysqlpp::Connection *conn = new My_Connection_Impl();
    mysqlpp::DB_Info *info = get_DB_Info(name);
    if (!conn->connect(info))

    cmap.insert(map<string, mysqlpp::Connection *>::value_type(name, conn));
    return 0;
  }

  /// 最後の接続名を入手する
  const char *
  Connection_Manager_Impl::get_last_connection()
  {
    return 0;
  }

  /// 管理下にある全ての接続を閉じる
  void
  Connection_Manager_Impl::close_all_connection()
  {
    map<string,mysqlpp::Connection *>::iterator it = cmap.begin();
    for (; it != cmap.end(); it++) { it->second->disconnect(); }
  }


};

namespace mysqlpp {

  unsigned int
  Result::num_fields()
  {
    if (res) return mysql_num_fields(res);
    return 0;
  }

  MYSQL_FIELD *
  Result::fetch_field(int idx)
  {
    if (!res) return NULL;
    if (idx < 0) return mysql_fetch_field(res);
    return mysql_fetch_field_direct(res, idx);
  }

  MYSQL_ROW
  Result::fetch_row()
  {
    if (res) return mysql_fetch_row(res);
    return 0;
  }

  /**
     mysql_fetch_lengths()は結果セットの現在列に対してのみ有効です。
     mysql_fetch_row()を呼び出す前
     もしくは結果セット中のすべての列を復元した後に呼び出すとNULLを戻します。
  */
  unsigned long *
  Result::fetch_length()
  {
    if (res) return mysql_fetch_lengths(res);
    return 0;
  }

  void
  Result::free()
  {
    if (res) {
      mysql_free_result(res);
      fprintf(stderr,"DEBUG: Result#free_result %p\n", res);
    }
    res = 0;
  }

  Resource::~Resource() { }

  DB_Info::~DB_Info() { }

  Cursor::~Cursor() { }
    
  Connection::Connection() { }

  Connection::~Connection() { }

  void Connection::free_cursor_all() { }

  Connection_Manager::Connection_Manager() { }

  Connection_Manager *
  Connection_Manager::get_Connection_Manager(const char *name)
  {
    return new Connection_Manager_Impl();
  }
};

// --------------------------------------------------------

extern "C" {

  /// MySQLの接続情報の登録と確認
  int cmd_my_account(int argc, char **argv) {
    puts("cmd_my_account not implemented yet.");
    return 0;
  }

  /// MySQLの基本操作
  int cmd_my_query(int argc, char **argv) {
    puts("cmd_my_query not implemented yet.");
    return 0;
  }

};

// --------------------------------------------------------------------------------


#ifdef USE_SUBCMD
#include "subcmd.h"

subcmd mysql_cmap[] = {
  { "mysql-account", cmd_my_account, },
  { "mysql-query", cmd_my_query, },
  { 0 },
};

#endif


