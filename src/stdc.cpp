/*! \file                                                                                                           
 * \brief 標準C/C++ライブラリの機能を確認する
 */

#include <clocale>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <memory>

// --------------------------------------------------------------------------------

using namespace std;

namespace {
  /*
    無名namespace により、このファイルで定義されるクラスが
    他のファイルとバッティングしないように保護している
  */

  /// 生成規則の確認用オブジェクト
  class XA {
    string a;
  public:
    /// デフォルト・コンストラクタ
    XA() { cerr << "alloc XA:" << this << endl; }
    /// コピー・コンストラクタ
    XA(XA &x) : a(x.a) { cerr << "copy XA:" << a << endl; }
    /// デストラクタ
    ~XA() { cerr << "free XA:" << this << ":" << a << endl; }
    /// パラメータ付きコンストラクタ
    XA(string a0) : a(a0) { cerr << "alloc XA:" << this << ":" << a << endl; }
    /// 代入オペレータ
    XA& operator=(XA &x) { a = x.a; cerr << "assing XA:" << a << endl; return *this; }
    /*
      メンバにプリミティブ以外の要素が含まれるため、このオペレータの定義は必須。
      未定義だと、メモリ破壊につながるので注意！
     */
  };

  /// オブジェクトの生成規則の確認
  static int test_allocation(int argc, char **argv) {

    XA aa("aa");
    {
      XA bb("bb");
      XA *bb0 = new XA("bb0");

      XA *cc = new XA[3];
      /*
	それぞれの要素が、デフォルトコンストラクタで初期化される
       */

      XA dd(aa);
      /*
	コピー・コンストラクタが呼ばれる
       */

      cc[0] = aa;
      /*
	= オペレータが呼ばれる
       */

      delete bb0;
      /*
	ポインタの場合は、明示的に delete を呼ばないと開放されない。
       */

      delete [] cc;
      /*
	配列の場合は、delete のスタイルが変わる。
	それぞれの要素のデストラクタが呼ばれる
       */

      /*
	スコープを外れるので bb, dd のデストラクタが呼ばれる
       */
    }

    XA ee("ee");

    return 0;
  }

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

  /// 型のサイズを確認する
  static int test_type_size(int argc, char **argv) {
    cerr << "char: " << sizeof(char) << endl;
    cerr << "int: " << sizeof(int) << endl;
    cerr << "short: " << sizeof(short) << endl;
    cerr << "unsigned: " << sizeof(unsigned) << endl;
    cerr << "long: " << sizeof(long) << endl;
    cerr << "unsinged long: " << sizeof(unsigned long) << endl;
    cerr << "long long: " << sizeof(long long) << endl;
    cerr << "double: " << sizeof(double) << endl;
    cerr << "float: " << sizeof(float) << endl;
    cerr << "void *: " << sizeof(void *) << endl;
    cerr << "char *: " << sizeof(char *) << endl;
#if 0
    void *a = (void *)0x123456789abcdeful;
    printf("%#lx\n",a);
#endif
    return EXIT_SUCCESS;
  }

};

// --------------------------------------------------------------------------------

#include "subcmd.h"

subcmd stdc_cmap[] = {
  { "stdc-alloc", test_allocation, },
  { "stdc-auto", test_auto01, },
  { "stdc-atoi", test_atoi, },
  { "stdc-sscanf01", test_sscanf, },
  { "stdc-size", test_type_size, },
  { NULL, },
};

