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

extern "C" int ends_with(const char *target, const char *suffix);

namespace {

  /// DBの接続情報を保持する
  class My_DB_Info : public mysqlpp::DB_Info {
    int name,user,passwd,socket,host,port;
    string sbuf;

    int offset(const char *tt) {
      if (!tt) return 0;
      size_t of = sbuf.size();
      sbuf.append(tt).append("",1);
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
    mysqlpp::Connection *ref;
    string cname;
    bool truncated, verbose;

  public:
    My_Cursor_Impl(MYSQL_STMT *stmt, mysqlpp::Connection *conn, const char *name) 
      : mysqlpp::Cursor(stmt, conn), cname(name)
    {
      init_elog("My_Cursor_Impl");
    }

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
    if (st) {
      int rc = mysql_stmt_close(st);
      if (rc) elog(W, "while statemet closing:(%u)\n", rc);
    }
    st = 0;
  }

  void My_Cursor_Impl::release() {
    elog(T, "release notify: %p", this);
    free();
  }

  /// クエリにより影響があった行数を入手する
  unsigned long My_Cursor_Impl::affected_rows() {
    if (st) return (unsigned long)mysql_stmt_affected_rows(st);
    return (unsigned long)-1;
  }
  
  /// AUTO_INCREMENT インデックスを利用した最後のクエリが生成したIDを返す
  unsigned long My_Cursor_Impl::insert_id() {
    if (st) return (unsigned long)mysql_stmt_insert_id(st);
    return 0lu;
  }
  
  /// SQLテキストを設定する
  bool My_Cursor_Impl::prepare(const std::string &query_text) {
    if (!st) return false;

    int rc = mysql_stmt_prepare(st, query_text.c_str(), query_text.size());
    if (rc != 0) {
      elog("prepare failure:(%u):%s\n", mysql_stmt_errno(st), mysql_stmt_error(st));
    }

    elog(T, "prepared: %s\n %s\n", cname.c_str(), query_text.c_str());
    return rc == 0;
  }

  /// 結果セットのリソースを解放する
  void My_Cursor_Impl::free_result() {
    if (!st) return;
    my_bool rc = mysql_stmt_free_result(st);
    if (rc == 0) return;
    elog(W,"free result failure:(%u)\n", rc);
  }

  /// 必要とするパラメータ数
  int My_Cursor_Impl::param_count() {
    if (st) return mysql_stmt_param_count(st);
    return 0;
  }

  /// パラメータをバインドする
  bool My_Cursor_Impl::bind(MYSQL_BIND *bind) {
    if (st == 0) return false;

    my_bool rc = mysql_stmt_bind_param(st, bind);
    if (rc == 0) return true;
    elog("bind failure:(%u):%s\n", mysql_stmt_errno(st), mysql_stmt_error(st));
    return false;
  }

  /// 結果セットの構成カラム数を得る
  int My_Cursor_Impl::field_count() {
    if (!st) return 0;
    return mysql_stmt_field_count(st);
  }

  /// 結果セットの受け取り領域をバインドする
  bool My_Cursor_Impl::bind_result(MYSQL_BIND *bind) {
    if (st == 0) return false;
    my_bool rc = mysql_stmt_bind_result(st, bind);
    if (rc == 0) return true;

    elog("bind result failure:(%u):%s\n", mysql_stmt_errno(st), mysql_stmt_error(st));
    return false;
  }
  
  /// クエリを実行する 
  bool My_Cursor_Impl::execute(bool populate) {
    if (!st) return false;

    int rc = mysql_stmt_execute(st);
    if (rc != 0) {
      // 成功した場合ゼロ。エラーが起こった場合、ゼロ以外。
      elog("execute failure:(%u):%s\n", mysql_stmt_errno(st), mysql_stmt_error(st));
      return false;
    }
    if (!populate) return true;

    rc = mysql_stmt_store_result(st);
    // 成功した場合ゼロ。エラーが起こった場合、ゼロ以外。
    if (rc == 0) return true;

    elog("store result failure:(%u):%s\n", mysql_stmt_errno(st), mysql_stmt_error(st));
    return false;
  }

  /// 行データを取り寄せる
  bool My_Cursor_Impl::fetch() {
    if (!st) return false;

    int rc = mysql_stmt_fetch(st);
    switch(rc) {
    case MYSQL_DATA_TRUNCATED:
      if (!truncated) {
	elog(W, "fetch: data truncated.\n");
	truncated = true;
      }
    case 0:
      return true;
    case 1:
      elog("fetch failure:(%u):%s\n", mysql_stmt_errno(st), mysql_stmt_error(st));
    case MYSQL_NO_DATA:
      return false;
    }

    elog("unkown fetch code:(%d)\n", rc);
    return false;
  }

  /// 行位置を確認する
  MYSQL_ROW_OFFSET My_Cursor_Impl::row_tell() {
    // mysql_stmt_store_result()の後にだけ使える
    if (!st) return 0;
    return mysql_stmt_row_tell(st);
  }

  /// 行位置を移動する
  MYSQL_ROW_OFFSET My_Cursor_Impl::row_seek(MYSQL_ROW_OFFSET offset) {
    if (!st) return 0;
    return mysql_stmt_row_seek(st, offset);
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
    virtual bool ping();
    virtual MYSQL *handler();
    virtual void disconnect();
    virtual bool query(const std::string &query_text, bool store = false);
    virtual mysqlpp::Result *get_result();
    virtual unsigned long insert_id();
    virtual unsigned long affected_rows();
    virtual bool select_db(const char *dbname);
    virtual void set_character_set(const char *names);
    virtual void set_autocommit(bool flag);
    virtual void commit();
    virtual void rollback();
    virtual std::string &escape_string(std::string &buf, std::string &text);
    virtual int warning_count();
    virtual void fetch_db_names(std::vector<std::string> &name_list, const char *tbl = "%");
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
    : cached_info(0) { init_elog("My_Connection_Impl"); }

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
      elog(E, "mysql:%s@%s:%d/%s;socket=%s;flag=%#x (%u):%s", 
	   user, host, port, name, socket, client_flag,
	   mysql_errno(conn), mysql_error(conn) );
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

    elog(T, "connected: %s\n", mysql_get_host_info(conn));
    return true;
  }

  bool
  My_Connection_Impl::ping()
  {
    if (!conn) return false;
    return mysql_ping(conn) == 0;
  }

  MYSQL *
  My_Connection_Impl::handler()
  {
    assert(cached_info);

    if (conn) {
      if (ping() && mysql_select_db(conn, cached_info->get_db_name()) == 0) return conn;

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

  void My_Connection_Impl::fetch_db_names(vector<string> &cnames, const char *db)
  {
    cnames.clear();

    MYSQL_RES *rs = mysql_list_dbs (conn, db);
    if (!rs) {
      elog(W, "cannot fetch database names:%s:(%u): %s\n",
	   db, mysql_errno(conn), mysql_error(conn) );
      return;
    }

    MYSQL_ROW row;

    while ((row = mysql_fetch_row(rs))) {
      unsigned long *lengths = mysql_fetch_lengths(rs);
      cnames.push_back(string(row[0], lengths[0]));
    }

    mysql_free_result(rs);
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
    if (it != stat.end()) return (*it).second;

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
      elog(T, "prepared: %s:%d\n %s\n", name, mysql_stmt_param_count(st), query_text->c_str());
    }

    // 登録する
    My_Cursor_Impl *cur = new My_Cursor_Impl(st, this, name);
    stat.insert(map<string, My_Cursor_Impl *>::value_type(name, cur));

    return cur;
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
    if (rc != 0)
      elog(W, "set_autocommit failed.:(%u):%s\n", mysql_errno(conn), mysql_error(conn));
    else
      elog(T, "autocommit mode changed: %s\n", flag ? "true" : "false");
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

  string &
  My_Connection_Impl::escape_string(string &buf, string &text)
  {
    if (!conn) return buf;
    char *wbuf = new char[text.size() * 2 + 1];
    size_t len = mysql_real_escape_string(conn, wbuf, text.c_str(), text.size());
    buf.append(wbuf, len);
    delete wbuf;
    return buf; 
  }

  void
  My_Connection_Impl::set_character_set(const char *name)
  {
    if (!conn) return;
    if (0 != mysql_set_character_set(conn, name))
      elog(W, "mysql set names %s: %s\n", name, mysql_error(conn));
    else
      elog(T, "character set: %s\n", name);
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

  // --------------------------------------------------------
};

#include "uc/datetime.hpp"
#include "uc/kvs.hpp"
#include "memory"

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
    uc::KVS *props;
    /// 接続インスタンスを保持する
    map<string,mysqlpp::Connection *>cmap;
    static const char *auth_dbname;

  public:
    Connection_Manager_Impl() {
      init_elog("Connection_Manager_Impl");
      const char *path = "work", *type = "bdb";
      props = uc::KVS::get_kvs_instance(path, type);
    }

    ~Connection_Manager_Impl() {
      close_all_connection();
      delete props;
    }

    virtual void get_db_names(vector<string> &name_list);
    virtual void store_db_parameter(const char *name, const map<string,string> &params);
    virtual bool fetch_db_parameter(const char *name, map<string,string> &params);
    virtual mysqlpp::DB_Info *get_DB_Info(const char *name);
    virtual mysqlpp::Connection *get_Connection(const char *name = 0);
    virtual const char *get_last_connection();
    virtual void close_all_connection();
    virtual void drop_db_parameter(const char *name);
  };

  const char *Connection_Manager_Impl::auth_dbname = "auth";

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

  /// キーの一式を入手する
  // 巨大なデータを扱う場合はメモリに注意
  static void fetch_keys(vector<string> &keys, uc::KVS *kvs)
  {
    string key;
    kvs->begin_next_key();
    while (kvs->fetch_next_key(key)) {
      keys.push_back(key);
    }
    kvs->end_next_key();
  }

  /// サフィックスが合致するキーの一覧を入手する
  static void fetch_suffix_match_keys(const char *suffix, vector<string> &keys, 
				      uc::KVS *kvs, bool trim = true)
  {
    string key;
    size_t slen = strlen(suffix);

    kvs->begin_next_key();
    while (kvs->fetch_next_key(key)) {
      if (ends_with(key.c_str(), suffix)) {
	if (trim) key.resize(key.size() - slen);
	keys.push_back(key);
      }
    }
    kvs->end_next_key();
  }

  /// プレフィックスが合致するキーの一覧を入手する
  static void fetch_prefix_match_keys(const char *prefix, vector<string> &keys, 
				      uc::KVS *kvs, bool trim = true)
  {
    size_t plen = strlen(prefix);
    string key;
    kvs->begin_next_key();
    while (kvs->fetch_next_key(key)) {
      if (strncmp(key.c_str(), prefix, plen) == 0) {
	if (trim) key.erase(0, plen);
	keys.push_back(key);
      }
    }
    kvs->end_next_key();
  }


  /// 登録済み接続名の入手
  void
  Connection_Manager_Impl::get_db_names(vector<string> &name_list)
  {
    props->open_kvs(auth_dbname, "r");
    fetch_suffix_match_keys(".mysql.dd.stored", name_list, props);
  }

  /// 接続情報の保存
  void
  Connection_Manager_Impl::store_db_parameter(const char *name, const map<string,string> &params)
  {
    props->open_kvs(auth_dbname, "w");

    string pname(name);

    uc::Date ti;
    pname += ".mysql.dd.stored";
    props->store_value(pname.c_str(), ti.now().get_date_text(0).c_str());

    map<string,string>::const_iterator it = params.begin();
    for (;it != params.end(); it++) {
      pname = name;
      pname += ".mysql.";
      pname += it->first.c_str();
      props->store_value(pname.c_str(), it->second.c_str());
    }
    
    props->close_kvs();
  }

  /// 接続情報の入手
  bool
  Connection_Manager_Impl::fetch_db_parameter(const char *name, map<string,string> &params)
  {
    props->open_kvs(auth_dbname, "r");
    string prefix(name);
    prefix += ".mysql.";

    vector<string> pkeys;
    fetch_prefix_match_keys(prefix.c_str(), pkeys, props);
    show_names(stdout, pkeys);

    params.clear();
    vector<string>::const_iterator it = pkeys.begin();

    for (;it != pkeys.end(); it++) {
      if (strncmp("dd.", it->c_str(), 3) == 0) continue;
      string pval, pname(prefix);
      pname += *it;
      if (!props->fetch_value(pname.c_str(), pval)) continue;
      params.insert(map<string, string>::value_type(*it,pval));
    }

    return true;
  }

  const char *param_value(const char *key, map<string,string> &params, const char *default_value = "") {
    string kbuf(key);
    map<string,string>::const_iterator it = params.find(key);
    if (it == params.end()) return default_value;
    return it->second.c_str();
  }

  /// 接続情報の入手
  mysqlpp::DB_Info *
  Connection_Manager_Impl::get_DB_Info(const char *name)
  {
    map<string,string> params;
    if (!fetch_db_parameter(name, params)) return 0;

    My_DB_Info02 info;
    info.name = param_value("db",params);
    info.user = param_value("user",params);
    info.passwd = param_value("password",params);
    info.socket = param_value("socket",params);
    info.host = param_value("host",params, "localhost");
    info.port = param_value("port",params);

    return new My_DB_Info(info);
  }

  /// 接続済みのDB接続を得る
  mysqlpp::Connection *
  Connection_Manager_Impl::get_Connection(const char *name)
  {
    map<string,mysqlpp::Connection *>::iterator it = cmap.find(name);
    if (it != cmap.end()) { return it->second; }
    /*
      接続済みであれば、それを返す
     */
    
    mysqlpp::DB_Info *info = get_DB_Info(name);
    if (!info) return 0;

    mysqlpp::Connection *conn = new My_Connection_Impl();
    if (!conn->connect(info)) return 0;

    cmap.insert(map<string, mysqlpp::Connection *>::value_type(name, conn));
    return conn;
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
    if (cmap.empty()) {
      elog(T, "no need close connection.\n");
      return;
    }

    elog(T, "all connection closing..\n");
    map<string,mysqlpp::Connection *>::iterator it = cmap.begin();
    for (; it != cmap.end(); it++) { it->second->disconnect(); }
    elog(T, "all connection closed.\n");
  }

  /// 接続情報の破棄
  void
  Connection_Manager_Impl::drop_db_parameter(const char *name)
  {
    map<string,mysqlpp::Connection *>::iterator fk = cmap.find(name);
    if (fk != cmap.end()) {
      elog("cannot drop using connection.\n");
      return;
    }

    props->open_kvs(auth_dbname, "w");
    string prefix(name);
    prefix += ".mysql.";

    vector<string> pkeys;
    fetch_prefix_match_keys(prefix.c_str(), pkeys, props);
    show_names(stdout, pkeys);

    if (pkeys.empty()) {
      elog(W, "%s: no drop target.\n", name);
      return;
    }

    vector<string>::const_iterator it = pkeys.begin();
    for (;it != pkeys.end(); it++) {
      string pname(prefix);
      props->store_value(pname.append(*it).c_str(), "");
    }

    elog(I, "%s: db paramster droped.\n", name);

    props->close_kvs();
  }

};

/// MySQL データベースを操作する一連の機能が含まれる

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
    
  Connection::Connection() : conn(0), last_result(0) { }

  Connection::~Connection() { }

  void Connection::free_cursor_all() { }

  Connection_Manager::Connection_Manager() { }

  Connection_Manager::~Connection_Manager() { }

  Connection_Manager *
  Connection_Manager::get_instance(const char *name)
  {
    return new Connection_Manager_Impl();
  }
};

