/*! \file                                                                                                           
 * \brief 標準Cライブラリの機能を確認する
 */

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <memory>

// --------------------------------------------------------------------------------

using namespace std;

namespace {

  /// virutal の振る舞いの確認用（継承元）
  class AA {
    int aa;
  public:
    AA(int _aa) : aa(_aa) { }
    virtual ~AA() {
      cerr << "TRACE: AA deleting: " << this
	   << "(" << aa << ")" << endl;
    };

    virtual void hello() { 
      cerr << "TRACE: AA hello: " << this
	   << "(" << aa << ")" << endl; 
    };
  };

  /// virutal の振る舞いの確認用（継承クラス）
  class BB : public AA {
    int bb;
  public:
    BB(int _bb) : AA(_bb * 10), bb(_bb) { }
    virtual ~BB() { cerr << "TRACE: BB deleting: " << this
			 << "(" << bb << ")" << endl; };
    virtual void hello() { 
      cerr << "TRACE: BB hello: " << this
	   << "(" << bb << ")" << endl; 
    };
  };

  /// virutal なしの確認用（継承元）
  class AA01 {
    int aa;
  public:
    AA01(int _aa) : aa(_aa) { }
    ~AA01() { cerr << "TRACE: AA01 deleting: " << this
			 << "(" << aa << ")" << endl; };
    void hello() { 
      cerr << "TRACE: AA01 hello: " << this
	   << "(" << aa << ")" << endl; 
    };
  };

  /// virutal なしの確認用（継承クラス）
  class BB01 : public AA01 {
    int bb;
  public:
    BB01(int _bb) : AA01(_bb * 10), bb(_bb) { }
    ~BB01() { cerr << "TRACE: BB01 deleting: " << this
			 << "(" << bb << ")" << endl; };
    void hello() { 
      cerr << "TRACE: BB01 hello: " << this
	   << "(" << bb << ")" << endl; 
    };
  };

};

/// auto_ptr と virutal 修飾の振舞の確認

static int test_auto01(int argc, char **argv) {
  AA aa(1);
  BB bb(2);
  AA01 aa01(1);
  BB01 bb01(2);

  // スコープを外れたら、自動でポインタを開放する。

  auto_ptr<AA> aaa(new AA(3));
  auto_ptr<AA> bbb(new BB(4));
  auto_ptr<AA01> aaa01(new AA01(5));
  auto_ptr<AA01> bbb01(new BB01(6)); // BB01のデストラクタがうまく動かない

  AA *a = &aa;
  a->hello(); //AAの処理が動く
  a = &bb;
  a->hello(); //BBの処理が動く

  aaa->hello(); // AAの処理が動く
  bbb->hello(); // BBの処理が動く(virutal 効果)
  aaa01->hello(); // AA01の処理が動く
  bbb01->hello(); // AA01の処理が動く

  /*
    この関数を抜ける直前に、デストラクタが呼ばれる。
    通常は継承元、継承クラスの両方のデストラクタが呼ばれることが期待される。
   */

  return EXIT_SUCCESS;
}

// --------------------------------------------------------------------------------

extern "C" {
  
  /// atoi でテキストを整数値に変換する
  static int test_atoi(int argc, char **argv) {
    int sum = 0;

    for (int i = 1; i < argc; i++) {
      sum += printf("%s => %d\n", argv[i], atoi(argv[i]));
    }
    fprintf(stderr,"%s: %d bytes output.\n", argv[0], sum);
    
    return EXIT_SUCCESS;
  }


  /// sscanf でテキストを整数値に変換する
  static int test_sscanf(int argc, char **argv) {
    int sum = 0;

    for (int i = 1; i < argc; i++) {
      int value = 0;
      
      if (1 == sscanf(argv[i], "%i", &value))
	sum += printf("%s => %d\n", argv[i], atoi(argv[i]));
      else
	sum += printf("%s => X\n", argv[i]);
    }
    fprintf(stderr,"%s: %d bytes output.\n", argv[0], sum);
    
    return EXIT_SUCCESS;
  }
};

// --------------------------------------------------------------------------------

#include "subcmd.h"

subcmd stdc_cmap[] = {
  { "stdc-auto", test_auto01, },
  { "stdc-atoi", test_atoi, },
  { "stdc-sscanf01", test_sscanf, },
  { NULL, },
};

