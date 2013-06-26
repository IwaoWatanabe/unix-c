// -*- mode: C++ -*-

/*! \file
 * \brief MySQL C APIのリソース管理をサポートする
 */

#ifndef __mysqlpp_hpp
#define __mysqlpp_hpp

#include <mysql.h>
#include <string>
#include <vector>
#include <map>

namespace mysqlpp {
  
  /// MySQLの結果セットの操作のリソース管理
  /**
     実装クラスはコンテナが提供する。
     処理終了後は関連リソースを解放する。
  */
  class Result {
  protected:
    MYSQL_RES *res;

  public:
    Result(MYSQL_RES *result) : res(result) { }
    virtual ~Result() { free(); }
    /// カラム件数を得る
    virtual unsigned int num_fields();
    /// カラム情報を得る
    virtual MYSQL_FIELD *fetch_field(int n);
    /// 行データを得る
    virtual MYSQL_ROW fetch_row();
    /// カラムのデータ長さが格納された配列を得る
    virtual unsigned long *fetch_length();
    /// 関連リソースを解放する
    virtual void free();
  };

  /// コネクションと連動するリソースへの通知
  /*
    Connectionと同じライフサイクルを持たせるユーザ・オブジェクトを
    名前を付けて登録することができる。
   */
  class Resource {
  public:
    virtual ~Resource();
    /// DB接続が切れたことが発覚したタイミングで呼び出される。
    virtual void release() { }
  };

  class Connection;

  /// 準備されたステートメントのリソース管理
  /**
     実装クラスはコンテナが提供する。
     生成のタイミングでクエリを登録していないなら、
     prepare を使って登録すること。
     処理終了後は関連リソースを解放する。
     複数クエリには対応していない。一つのクエリだけ扱う。
  */
  class Cursor : Resource {
  protected:
    MYSQL_STMT *cur;
    Connection *ref;
    bool truncated, verbose;

  public:
    Cursor(MYSQL_STMT *stmt, Connection *conn) 
      : cur(stmt), ref(conn), truncated(false) { }

    virtual ~Cursor() = 0;
    /// このカーソルと連携するコネクションを得る
    virtual Connection *get_Connection() { return ref; }
    /// 関連リソースを解放する
    virtual void free() = 0;
    /// クエリにより影響があった行数を入手する
    virtual unsigned long affected_rows() = 0;
    /// AUTO_INCREMENT インデックスを利用した最後のクエリが生成したIDを返す
    virtual unsigned long insert_id() = 0;
    /// SQLテキストを設定する
    virtual bool prepare(const std::string &query_text) = 0;
    /// 結果セットのリソースを解放する
    virtual void free_result() = 0;
    /// 必要とするパラメータ数
    virtual int param_count() = 0;
    /// パラメータをバインドする
    virtual bool bind(MYSQL_BIND *bind) = 0;
    /// 結果セットの構成カラム数を得る
    virtual int field_count() = 0;
    /// 結果セットの受け取り領域をバインドする
    virtual bool bind_result(MYSQL_BIND *bind) = 0;
    /// クエリを実行する 
    virtual bool execute(bool populate = false) = 0;
    /// 行データを取り寄せる
    virtual bool fetch() = 0;
    /// 行位置を確認する
    virtual MYSQL_ROW_OFFSET row_tell() = 0;
    /// 行位置を移動する
    virtual MYSQL_ROW_OFFSET row_seek(MYSQL_ROW_OFFSET offset) = 0;
  };

  /// MySQLの基本接続情報を入手する
  class DB_Info {
  public:
    virtual ~DB_Info() = 0;
    virtual const char *get_db_name() = 0;
    virtual const char *get_db_user() = 0;
    virtual const char *get_db_password() = 0;
    virtual const char *get_db_socket_path() = 0;
    virtual const char *get_db_host() = 0;
    virtual int get_db_port() = 0;
  };

  /// MySQLの接続情報とそれに関連するリソースを管理する。
  /** 
      実装クラスはコンテナが提供する。
      このインスタンス単位でMySQLサーバに接続する。
   */
  class Connection {
  protected:
    /// 接続管理
    MYSQL *conn;
    /// 最後のクエリの参照
    MYSQL_RES *last_result;
    /// 確保しているMYSQL_STMTを全解放する
    virtual void free_cursor_all();

  public:
    Connection();
    virtual ~Connection() = 0;

    /// 接続情報を渡してDB接続を行う
    virtual bool connect(DB_Info *info) = 0;
    /// DB接続の参照を入手する
    virtual MYSQL *handler() = 0;
    /// DB接続を解除する
    virtual void disconnect() = 0;
    /// クエリを実行する
    virtual bool query(const std::string &query_text, bool store = false) = 0;
    /// 結果セットを入手する
    virtual Result *get_result() = 0;
    /// クエリにより影響があった行数を入手する
    virtual unsigned long insert_id() = 0;
    /// AUTO_INCREMENT インデックスを利用した最後のクエリが生成したIDを返す
    virtual unsigned long affected_rows() = 0;
    /// DBを選択する
    virtual bool select_db(const char *dbname) = 0;
    /// （接続ユーザが操作可能な）DB名一覧を入手する
    virtual void get_db_names(std::vector<std::string> &name_list) = 0;
    /// 現在の接続のためのデフォルト文字セットをセットする
    virtual void set_character_set(const char *names) = 0;
    /// トランザクション・モードを切り替える
    virtual void set_autocommit(bool flag) = 0;
    /// トランザクションをコミットする
    virtual void commit() = 0;
    /// トランザクションをロールバックする
    virtual void rollback() = 0;
    /// SQLステートメントの中に使うことができる文字列に変換する
    virtual void escape_string(std::string &buf, std::string &text) = 0;
    /// 前回実行された SQL ステートメントの実行中に発生した警告数を返す
    virtual int warning_count() = 0;
    /// テーブル名を入手する
    virtual void fetch_table_names(std::vector<std::string> &cnames, const char *tbl = "%") = 0;
    /// テーブルのカラム名を入手する
    virtual void fetch_column_names(std::vector<std::string> &cnames, const char *tbl, const char *wild = "%") = 0;
    /// リソースを登録する
    /*
      登録したリソースは、Connection の開放のタイミングで合わせて開放されるため、
      削除するユーザコードは特に記述する必要はない。
    */
    virtual bool add_resource(const char *name, mysqlpp::Resource *res) = 0;
    /// リソースの登録を解除する
    virtual void remove_resourece(const char *name) = 0;
    /// 登録済みリソース名を入手する
    virtual void get_resource_names(std::vector<std::string> &cnames) = 0;
    /// 登録しているリソースを入手する
    virtual Resource *find_resource(const char *name) = 0;
    /// Cursor を名前を指定して入手する
    /**
      このタイミングでクエリ・テキストを渡さない場合は、
      Cursorに対してクエリを設定する必要がある。
     */
    virtual Cursor *find_cursor(const char *query_name, const std::string *query_text = 0) = 0;
    /// 登録済Cursor名の入手
    virtual void get_cursor_names(std::vector<std::string> &name_list) = 0;
  };

  /// 接続情報を取りまとめる
  class Connection_Manager {
  public:
    static Connection_Manager *get_Connection_Manager(const char *name = "");

    /// 登録済み接続名の入手
    virtual void get_db_names(std::vector<std::string> &name_list) = 0;
    /// 接続情報の保存
    virtual void store_db_parameter(const char *name, const std::map<std::string,std::string> &params) = 0;
    /// 接続情報の入手
    virtual void fetch_db_parameter(const char *name, std::map<std::string,std::string> &params) = 0;
    /// 接続済みのDB接続を得る
    virtual Connection *get_Connection(const char *name = 0) = 0;
    /// 最後の接続名を入手する
    virtual const char *get_last_connection() = 0;
    /// 管理下にある全ての接続を閉じる
    virtual void close_all_connection() = 0;

  protected:
    Connection_Manager();
    /// 接続情報の入手
    virtual DB_Info *get_DB_Info(const char *name) = 0;

  private:
    Connection_Manager(Connection_Manager &not_allow) { }
  };

};

#endif
