/*! \file
 * \brief ファイル・コメントをここに記載する
 */

#include <cstdio>
#include <cstdlib>
#include <memory>

// --------------------------------------------------------------------------------

using namespace std;

namespace {
  // namespace の中にクラス定義を記載する
  // namespace名を省略すると、ファイルに閉じた名前空間となる

  class AA {
    virtual int test01();
  public:
    virtual ~AA() { }
    /// 検証開始
    static int run(int argc, char **argv);
  };

  int AA::test01() {
    fprintf(stderr,"aa\n");
    return 0;
  }
  
  int AA::run(int argc, char **argv) {
    auto_ptr<AA> aa(new AA());
    return aa->test01();
  }

};

// --------------------------------------------------------------------------------

#ifdef USE_SUBCMD

#include "subcmd.h"

static int test_temp01(int argc, char **argv) {
  fprintf(stderr,"temp01\n");
  return 0;
}

subcmd temp_cmap[] = {
  { "temp", test_temp01,  },
  { "temp02", AA::run,  },
  { 0 },
};

#endif
