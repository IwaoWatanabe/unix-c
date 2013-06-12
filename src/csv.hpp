/*! \file
 * \brief CSVの操作をサポートするクラス
 */

#ifndef _csv_h00
#define _csv_h00

namespace uc {
  /// CSVを読み込むクラスが実装すべきメソッドを定義
  /**
     CSV_Source の実装クラスにインスタンスを渡して呼び出してもらう。
  */
  class CSV_Reader {
  public:
    virtual ~CSV_Reader() {}
    /// 処理を開始する前に呼び出される
    virtual bool begin_read_csv() { return true; }
    
    /// 1行読み込む毎に呼び出される
    /**
       row を読み取って処理すること。
       0以外の値を返すと処理を中断する。
    */
    virtual int read_csv(const char **row, int columns) = 0;

    /// 処理が開始されていれば、終了あるいは中断のタイミングで呼び出される
    virtual void end_read_csv(bool cancel) { }
  };

  extern void load_csv(const char *fname, CSV_Reader *reader);
};

#endif

