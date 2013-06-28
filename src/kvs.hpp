/*! \file
 * \brief Key-Value-Storeを取り扱うAPI
 */

#ifndef __UC_KVS_HPP
#define __UC_KVS_HPP

#include <string>
#include <vector>
#include <map>

namespace uc {

  /// Key-Value Store の基本機能を利用するインタフェース
  class KVS {
  protected:
    std::string db_dir_path;
    KVS() { }

  private:
    KVS(KVS &not_allow) { }
    KVS &operator=(KVS &not_allow) { }

  public:
    /// 操作用インスタンスの入手
    static KVS *get_kvs_instance(const char *dir_path, const char *dba_type = "");
    virtual ~KVS();

    /// 基準ディレクトリの設定
    virtual bool set_kvs_directory(const char *dir) = 0;
    /// 基準ディレクトリの設定(書式付)
    virtual bool set_kvs_fdirectory(const char *format, ...);

    /// 基準ディレクトリの入手
    virtual std::string &get_kvs_directory() { return db_dir_path; }

    /// データベース・ファイルの一覧を入手する
    virtual void get_kvs_list(std::vector<std::string> &list) = 0;
    /// データベース・ファイルを破棄する
    virtual void drop_kvs(const char *dbname) = 0;

    /// データベース利用開始
    virtual int open_kvs(const char *dbname, const char *mode) = 0;
    /// データベース利用開始(書式付)
    virtual int open_fkvs(const char *db_format, const char *mode, ...);
    /// 値の入手
    virtual bool fetch_value(const char *key, std::string &value) = 0;
    /// 値の入手(書式付)
    virtual bool fetch_fvalue(const char *key_format, std::string &value, ...);
    /// 値の登録
    virtual bool store_value(const char *key, const char *value) = 0;
    /// 値の登録(書式付)
    virtual bool store_fvalue(const char *key_format, const char *value, ...);
    /// 登録キー名の確認
    virtual bool has_key(const char *key) = 0;
    /// 登録キー名の確認(書式付)
    virtual bool has_fkey(const char *key_format, ...);

    /// 登録キー名の入手開始
    virtual void begin_next_key() = 0;
    /// 登録キー名の入手
    virtual bool fetch_next_key(std::string &key) = 0;
    /// キーの入手の終了
    virtual void end_next_key() = 0;

    /// リソースの開放
    virtual void close_kvs() = 0;
    /// データのストレージ同期
    virtual void sync_kvs() = 0;
    /// ストアの実装名とバージョン文字列を返す
    virtual const char *get_kvs_version() { return "KVS: 0.0"; }
    /// ストアの状態をレポートする
    virtual void show_report(FILE *fout);
  };
};

#endif

