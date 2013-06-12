/*! \file
 * \brief .INIファイルを取り扱うAPI
 */

#ifndef __INI_H
#define __INI_H

#include <string>
#include <map>
#include <vector>

namespace uc {

  /// INIファイルを読み込む簡易機能を提供する
  /*＊
    テキスト形式で設定パラメータが入手できるので、
    それを数値/bool値として使用する場合はアプリが対応する必要がある。
   */
  class INI_Loader {
  protected:
    std::string ini_file, current_section, default_section;

  public:
    virtual ~INI_Loader() {}
    INI_Loader() {}

    /// 読み取り対象のiniファイルを設定する
    /*
      このタイミングでINIファイルが取り込まれる
     */
    virtual bool set_ini_filename(const char *file_name) = 0;
    /// 対象のiniファイル名を入手する
    virtual std::string get_ini_filename() { return ini_file; };
    /// 含まれるセッション名の一覧を入手する
    virtual void fetch_section_names(std::vector<std::string> &names) = 0;
    /// デフォルトのセッション名を設定する
    /**
      NULLあるいは空文字を設定すると、最初に出現するセクションを扱う。
      セクションの定義が無い状態で定義されたパラメータがあると、
      それは空のセクションとなる。
    */
    virtual void set_section(const char *name) = 0;
    /// デフォルトのセッション名を入手する
    virtual std::string get_current_section() { return current_section; }
    /// 登録済のパラメータ名の一覧を入手する
    /*
      sectionを省略すると、デフォルトのセッション名を利用する
     */
    virtual void fetch_config_names(std::vector<std::string> &names, const char *section = 0) = 0;
    /// デフォルトのセッション名を設定する
    /*
      これで指定したセクションについて透過して設定として見えるようになる。
      NULLあるいは空文字を設定すると、この機能は働かない。
     */
    virtual void set_default_section(const char *name) = 0;
    /// デフォルトのセッション名を入手する
    virtual std::string get_default_section() { return default_section; };

    /// 設定パラメータを入手する
    /*
      sectionを省略すると、デフォルトのセッション名を利用する
     */
    virtual std::string get_config_value(const char *name, const char *section = 0) = 0;
  };

  /// 処理インスタンスを入手する
  extern INI_Loader *create_INI_Loader();
};

#endif

