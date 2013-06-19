/*! \file
 * \brief ローカル・ファイルの操作をサポートするクラスを提供する
 */

#ifndef __uc_local_file_hpp
#define __uc_local_file_hpp

#include "uc/text-reader.hpp"

#include <cstdlib>
#include <string>

namespace uc {

  /// ローカル・ファイルの基本操作をサポートする
  class Local_File {
  public:
    Local_File() { }
    virtual ~Local_File();
    /// ディレクトリであるか診断する
    virtual bool isdir(const char *dirpath) = 0;
    /// 作業ディレクトリ名を入手する
    virtual std::string getcwd() = 0;
    /// 作業ディレクトリ名を変更する
    virtual bool chdir(const char *dirpath) = 0;
    /// 再帰的にディレクトリを作成する。
    virtual bool mkdirs(const char *dirpath) = 0;
    /// 再帰的にディレクトリを削除する。空では削除できない
    virtual int rmdirs(const char *dirpath) = 0;
    /// パスのファイル名部を得る
    virtual std::string basename(const char *path) = 0;
    /// パスのディレクトリ部を得る
    virtual std::string dirname(const char *path) = 0;

    /// 一般ファイルを削除する
    virtual int remove_file(const char *filepath, bool recurce) = 0;
    /// 一般ファイルを複製する
    virtual int copy_file(const char *dst, const char * const *src, size_t src_count, bool recurce) = 0;
    /// 一般ファイルを移動する
    virtual int move_file(const char *dst, const char * const *src, size_t src_count) = 0;

    /// テキスト・ファイルを読み込むインスタンスを入手する
    virtual Local_Text_Source *create_Text_Source(const char *file_name);
  };
};

#endif

