/*! \file
 * \brief コンテナが呼び出すインスタンスのインタフェースを定義する
 */

#ifndef uc_continer_hpp
#define uc_container_hpp

#include "uc/local-file.hpp"
#include "uc/datetime.hpp"
#include <vector>

namespace uc {

  /// 設定情報を入手するインタフェース
  /*
    実装クラスは処理系が提供する。
   */
  struct Property {
    virtual ~Property();
    /// ジョブの定義済みプロパティを入手
    virtual const char *get_property(const char *name, const char *default_value = "") = 0;
    /// ジョブの定義済みプロパティ(数値)を入手
    virtual long get_property_value(const char *name, long default_value = 0) = 0;
    /// 有効なプロパティ名を入手
    virtual bool get_property_names(std::vector<const char *> &names) = 0;
  };

  /// サービスの起動と停止を制御する
  /*
    ユーザコードとして提供されることもあるが、
    コンテナ側から提供されるものもある。
  */
  struct Service {
    virtual ~Service();
    /// サービスの利用を開始する
    virtual void start() = 0;
    /// サービスの利用を停止する
    virtual void stop() = 0;
    /// サービスの利用を停止する
    virtual const char *get_service_status() = 0;
    /// サービスの名称を入手する
    virtual const char *get_service_name() = 0;
    /// サービスのバージョンを入手する
    virtual const char *get_service_version() = 0;
  };
  
  /// サービスをインスタンス化するクラスが実装するインタフェース
  /*
    コンテナは、このインタフェースを利用してサービスを生成する。
   */
  struct Service_Factory {
    virtual ~Service_Factory() = 0;
    /// サービスの処理インスタンスを作成する
    virtual Service *create_Service_instance(const char *name_hint) = 0;
    /// サービス・ファクトリの名称を入手する
    virtual const char *get_factory_name() = 0;
    /// コンテナは、このメソッドを使って設定パラメータを渡す
    virtual void set_Property(Property *props) = 0;
  };

  class Job_Context;

  /// コンテナの様々な機能にアクセスするためのインタフェース
  /*
    実装クラスは処理系が提供する。
    コンテナが拡張されたら、このインタフェースにも機能が追加される。
   */
  class Job_Context : public Property {
  public:
    virtual ~Job_Context() = 0;
    /// ジョブの登録名を入手
    virtual const char *get_job_name() = 0;
    /// ジョブの実行オプションを入手
    virtual int get_job_option(std::vector<const char *> args) = 0;

    /// 当日の業務日付を入手する
    virtual Date get_buzzines_day() = 0;
    /// 翌日業務日付を入手する(カレンダと連動する)
    virtual Date get_next_buzzines_day(int day = 1) = 0;

    /// ローカルファイルの操作インスタンスを入手する
    virtual Local_File *create_Local_File() = 0;

    /// サービスの接続を入手する
    virtual Service *find_Service(const char *name) = 0;
  };

  /// コンテナに登録して動かすユーザ・コードが実装するインタフェース
  class Job {
  public:
    virtual ~Job() = 0;
    /// インスタンス化のあとにコンテナからコンテキスを設定するために呼び出される
    virtual void set_Job_Context(Job_Context *context) { }
    /// ジョブを開始するために呼び出される。これで例外が生じたら、後のフェーズは呼び出されない。
    virtual void begin_job() { }
    /// ユーザコードはこのタイミングで処理を行う。返り値は rc として利用される。
    virtual int process_job() = 0;
    /// ジョブを完了後に後始末処理のために呼び出される。
    virtual void end_job() { }
  };

  /// ジョブ・インスタンスが実装するインタフェース
  /*
   */
  class Job_Factory {
  protected:
    Job_Factory();
  public:
    virtual ~Job_Factory() = 0;
    virtual Job *create_Job_instance() = 0;
  };

};

#endif

