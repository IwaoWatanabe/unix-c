/*! \file
 * \brief 公開鍵を操作する一連の機能を定義する
 */

#include <string>
#include <vector>
#include <map>

namespace uc {

  /// 公開鍵を扱うAPIを提供する
  struct Key_Tool {
    enum encypt_type { PUBLIC_KEY, PRIVATE_KEY, };
    virtual ~Key_Tool();

    /// キーストア・ディレクトリの設定
    virtual void set_keystore_dir(const char *key_store_dir) = 0;
    /// キーストア・ディレクトリの入手
    virtual std::string get_keystore_dir() = 0;
    /// RSAキー名の作成
    virtual bool create_rsa_key(const char *alias, const std::map<std::string,std::string> &params) = 0;
    /// 登録済みキー名の入手
    virtual void get_rsa_key_names(std::vector<std::string> &name_list) = 0;
    /// RSAキーの取り込み
    virtual bool import_rsa_key(const char *alias, std::string &pem) = 0;
    /// RSAキーの取り出し
    virtual bool export_rsa_key(const char *alias, const char *option, std::string &pem) = 0;
    /// 暗号・複合の出力バッファサイズ情報
    virtual size_t get_size_hint(const char *alias) = 0;
    /// 暗号化する
    virtual size_t encrypt(const char *buf, size_t bufsize, char *outbuf, int type) = 0;
    /// 複合化する
    virtual size_t decrypt(const char *buf, size_t bufsize, char *outbuf, int type) = 0;
    /// キーストアの実装クラスを入手する
    static Key_Tool *create_Key_Tool(const char *name = "");
  };

};

#include "uc/elog.hpp"

using namespace std;

namespace {

  class Key_Tool_Impl : public uc::Key_Tool, uc::ELog {

  public:
    Key_Tool_Impl();
    ~Key_Tool_Impl();
    void set_keystore_dir(const char *key_store_dir);
    string get_keystore_dir();
    bool create_rsa_key(const char *alias, const map<string,string> &params);
    void get_rsa_key_names(vector<string> &name_list);
    bool import_rsa_key(const char *alias, string &pem);
    bool export_rsa_key(const char *alias, const char *option, string &pem);
    size_t get_size_hint(const char *alias);
    size_t encrypt(const char *buf, size_t bufsize, char *outbuf, int type);
    size_t decrypt(const char *buf, size_t bufsize, char *outbuf, int type);
  };

  Key_Tool_Impl::Key_Tool_Impl() { }

  Key_Tool_Impl::~Key_Tool_Impl() { }

  /// キーストア・ディレクトリの設定
  void Key_Tool_Impl::set_keystore_dir(const char *key_store_dir) {
  }

  /// キーストア・ディレクトリの入手
  string Key_Tool_Impl::get_keystore_dir() {
    return "";
  }

  /// RSAキー名の作成
  bool Key_Tool_Impl::create_rsa_key(const char *alias, const map<string,string> &params) {
    return false;
  }

  /// 登録済みキー名の入手
  void Key_Tool_Impl::get_rsa_key_names(vector<string> &name_list) {
  }

  /// RSAキーの取り込み
  bool Key_Tool_Impl::import_rsa_key(const char *alias, string &pem) {
    return false;
  }

  /// RSAキーの取り出し
  bool Key_Tool_Impl::export_rsa_key(const char *alias, const char *option, string &pem) {
    return false;
  }

  /// 暗号・複合の出力バッファサイズ情報
  size_t Key_Tool_Impl::get_size_hint(const char *alias) {
    return 0;
  }

  /// 暗号化する
  size_t Key_Tool_Impl::encrypt(const char *buf, size_t bufsize, char *outbuf, int type) {
    return 0;
  }

  /// 複合化する
  size_t Key_Tool_Impl::decrypt(const char *buf, size_t bufsize, char *outbuf, int type) {
    return 0;
  }
};

namespace uc {
  /// キーストアの実装クラスを入手する

  Key_Tool::~Key_Tool() { }

  Key_Tool *Key_Tool::create_Key_Tool(const char *name) {
    return new Key_Tool_Impl();
  }

};

// --------------------------------------------------------------------------------

enum { I = uc::ELog::I };

/// RSA鍵の作成
static int rsa_create01(int argc, char **argv) {
  return 0;
}

/// RSA鍵をPEMで出力
static int rsa_export01(int argc, char **argv) {
  return 0;
}

/// RSA鍵（PEM形式）をキーストア領域に取り込む
static int rsa_import01(int argc, char **argv) {
  return 0;
}

/// RSA鍵による暗号化
static int rsa_encrypt01(int argc, char **argv) {
  return 0;
}

/// RSA鍵による複合化
static int rsa_decrypt01(int argc, char **argv) {
  return 0;
}

// --------------------------------------------------------------------------------

#ifdef USE_SUBCMD
#include "subcmd.h"

subcmd key_cmap[] = {
  { "rsa-create", rsa_create01, },
  { "rsa-export", rsa_export01, },
  { "rsa-import", rsa_import01, },
  { "rsa-encrypt", rsa_encrypt01, },
  { "rsa-decrypt", rsa_decrypt01, },
  { 0 },
};

#endif
