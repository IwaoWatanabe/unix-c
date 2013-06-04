/*! \file
 * \brief STL; Standard Template Library を利用する例を含めています。
 */

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <set>
#include <string>
#include <map>
#include <vector>

using namespace std;

// --------------------------------------------------------------------------------

/// std::string の振る舞いの確認
static int string01(int argc, char **argv) {

  string aa("abc");
  string bb;

  bb += aa;
  bb += "def";
  /*
    このように += 演算子を使うことにより、文字列に追加できる。
    ただし、数値や任意のオブジェクトを追加できるわけではない。
    オブジェクトに、自身の状態をstringの形で出力するメソッドを作り、
    それを呼び出して追加する必要がある。
   */

  printf("bb: %s: %d\n", bb.c_str(), bb.size());
  /*
    string の値を出力する場合は、c_str() を呼び出す。
    ゼロ文字で終了する const char * を返してくれる。
   */

  double pi = 3.141592;
  char buf[80];
  string rvalue_text(buf, snprintf(buf,sizeof buf,"real value: %f", pi));
  puts(rvalue_text.c_str());

  /*
    数値を文字列表現に変えるには、このように　snprintf を利用するとよい。
    これなら出力書式も調整できる。
   */

  return 0;
}

// --------------------------------------------------------------------------------

/// 要素の内容を出力する

static void show_vector(vector<int> &aa, const char *sep = ", ") {
  vector<int>::iterator it = aa.begin();
  const char *xsep = "";
  for (; it != aa.end(); it++) {
    printf("%s%d", xsep, *it);
    xsep = sep;
    /*
      イテレータで走査して要素を出力する。
     */
  }
}

/// std::vector の振る舞いの確認
/**
  配列のように振る舞うコンテナ。
  要素を追加すると自動的に領域を拡張する。
  配列の先頭に要素を追加する操作は苦手。その場合は deque を利用すればよい。
*/
static int vector01(int argc, char **argv) {
  vector<int> aa;

  printf("empty:%d\n", aa.empty() ? 1 : 0);
  /*
    生成した直後は空判定で true となる。
   */

  int bb[] = { 3, 1, 4, 1, 5, 9, 2, };
  for (int i = 0, n = sizeof bb/sizeof bb[0]; i < n; i++) {
    aa.push_back(bb[i]);
    /*
      末尾に要素を追加する。
    */
  }

  printf("size:%d\n", aa.size());
  /*
    要素数を入手する。
   */

  for (int i = 0, n = aa.size(); i < n; i++) {
    printf("vec %d:%d\n", i, aa[i]);
    /*
      配列の様に[] 演算子を使ってアクセスできる。
    */
  }

  aa[0] = 33;
  /*
    配列のように[]演算子で値を代入できる。
    ただし、まだ要素が格納されていない領域に対するアクセスは
    通常の配列と同様、ふるまいは未定義である。
   */

  int cc = 10;
  vector<int>::iterator pos = find(aa.begin(), aa.end(), cc);
  /*
    algorithm で定義される find を使って要素の場所を探す。
    find には、探索範囲(イテレータ)と、要素を渡す。
    見付からなければ end が返る。
   */
  if (pos == aa.end())
    printf("%d not found.\n", cc);

  cc = 5;

  pos = find(aa.begin(), aa.end(), cc);
  if (pos != aa.end()) {
    vector<int>::iterator next_pos = aa.erase(pos);
    /*
      eraseは削除した要素の次の要素の位置を返す。
     */
    printf("%d erased.\n", cc);
  }
  /*
    場所(イテレータ)を指定してerase で削除する。
    その場所以後の要素は詰まってくるため、
    先頭付近での操作は処理に時間がかかる。
   */

  int bb2[] = { 1, 2, 3, 4, 5, };
  for (int i = 0, n = sizeof bb2/sizeof bb2[0]; i < n; i++) {
    vector<int>::iterator pos = aa.begin();
    aa.insert(pos, bb2[i]);
    /*
      位置を指定して登録する。
      この例では、先頭に追加している。
    */
  }
  show_vector(aa);
  putchar('\n');

  stable_sort(aa.begin(), aa.end());
  /*
    要素を安定なアルゴリズム(マージソート)で整列する。
    同じ要素が複数あった場合に、その位置関係が整列後も保たれます。
    その必要がなければ、sort を使えばクイックソートで整列する。　
   */
  show_vector(aa);
  putchar('\n');

  
  aa.clear();
  /*
    要素をクリアする
   */
  printf("size:%d\n", aa.size());
}

// --------------------------------------------------------------------------------

/// 要素の内容を出力する

static void show_map(map<int,string> &aa, const char *sep = ", ") {
  map<int,string>::iterator it = aa.begin();
  const char *xsep = "";
  for (; it != aa.end(); it++) {
    printf("%s%d:%s", xsep, it->first, it->second.c_str());
    xsep = sep;
    /*
      イテレータで走査して要素を出力する。
     */
  }
}

/// std::map の振る舞いの確認
/**
  検索キーとその値を組で保持するコンテナ。
*/
static int map01(int argc, char **argv) {
  map<int,string> aa;

  printf("empty:%d\n", aa.empty() ? 1 : 0);
  /*
    生成した直後は空判定で true となる。
   */

  char nbuf[100];
  int bb[] = { 3, 1, 4, 1, 5, 9, 2, };
  for (int i = 0, n = sizeof bb/sizeof bb[0]; i < n; i++) {
    string cc(nbuf, (size_t)snprintf(nbuf, sizeof nbuf, "text%03d",bb[i]));
    aa.insert(map<int,string>::value_type(bb[i], cc));
    /*
      要素を追加する。
    */
  }

  printf("size:%d\n", aa.size());
  /*
    要素数を入手する。
   */

  for (int i = 0, n = sizeof bb/sizeof bb[0]; i < n; i++) {
    printf("map %d:%s\n", bb[i], aa[bb[i]].c_str());
    /*
      配列のように[]演算子で検索キーを指定して
      要素を取り出すことができる。
    */
  }

  aa[100] = "text100";
  /*
    配列のように[]演算子で検索キーを指定して
    要素を代入することができる。
   */

  int cc = 99;
  printf("map %d:%s\n", cc, aa[cc].c_str());
  /*
    存在しない検索キーで参照すると、デフォルト・コンストラクタで生成された値が格納される。
    文字列の場合は 空テキスト。
   */

  show_map(aa);
  putchar('\n');
}


// --------------------------------------------------------------------------------

/// std::set の振る舞いの確認
/**
   重複する要素が含まれないことを保証するために利用するコンテナ。
*/
static int set01(int argc, char **argv) {
  set<int> aa;

  printf("empty:%d\n", aa.empty() ? 1 : 0);
  /*
    生成した直後は空判定で true となる。
   */

  int bb[] = { 3, 1, 4, 1, 5, 9, 2, };
  for (int i = 0, n = sizeof bb/sizeof bb[0]; i < n; i++) {
    aa.insert(bb[i]);
    /*
      insert で値を登録する
      set の場合は、重複する要素は登録されない。
    */
  }

  printf("size:%d\n", aa.size());
  /*
    要素数を入手する。
   */

  set<int>::iterator it = aa.begin();
  char *sep = "";
  for (; it != aa.end(); it++) {
    printf("%s%d", sep, *it);
    sep = ", ";
    /*
      イテレータで走査して要素を出力する。
     */
  }
  putchar('\n');

  int cc = 10;

  set<int>::iterator pos = aa.find(cc);
  /*
    find で要素の場所を探す。
    見付からなければ end が返る。
   */
  if (pos == aa.end())
    printf("%d not found.\n", cc);

  cc = 5;

  pos = aa.find(cc);
  if (pos != aa.end()) {
    aa.erase(pos);
    printf("%d erased.\n", cc);
  }
  /*
    場所(イテレータ)を指定してerase で削除する。
   */
  
  aa.clear();
  /*
    要素をクリアする
   */
  printf("size:%d\n", aa.size());
}

// --------------------------------------------------------------------------------

#include "subcmd.h"

subcmd stl_cmap[] = {
  { "stl-str01", string01 },
  { "stl-vec01", vector01 },
  { "stl-set01", set01 },
  { "stl-map01", map01 },
  { 0 },
};

