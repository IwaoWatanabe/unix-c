
/*! \file
 * \brief JOBコンテナのサンプル実装
 */

#include "uc/container.hpp"


/// Unix C/C++ サンプル・クラス一式が含まれる。
/**
   UNIX C/C++で利用できる汎用処理を取りまとたパッケージ
 */
namespace uc {
  // 機能が変更されたら、公開する前に第1桁を増分する
  // 機能が追加されたら、公開する前に第2桁を増分する
  // バグフィックスの対応や、実装クラスの差し替えは第3桁を増分する
  const static char *version = "UC:0.1.0";

  // ライブラリのバージョン番号を入手する
  const char *get_version() { return version; }
};

