/*! \file                                                                                                           
 * \brief Athena Widget 依存の拡張モジュール                                                                        
 */

#include <cstdio>
#include <cerrno>
#include <dirent.h>
#include <iostream>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

#include <X11/IntrinsicP.h>
#include <X11/ObjectP.h>
#include <X11/CoreP.h>

#ifdef XAW3D
#include <X11/Xaw3d/ListP.h>
#include <X11/Xaw3d/TextP.h>
#else
#include <X11/Xaw/ListP.h>
#endif

/// リストのカラム数を入手する                                                                                      
int getXawListColumns(Widget list) {
  return ((struct _ListRec *)list)->list.ncols;
}

/// リストの行数を入手する                                                                                          
int getXawListRows(Widget list) {
  return ((struct _ListRec *)list)->list.nrows;
}

#ifdef XAW3D
extern "C" {

/// テキストの最後の位置を返す                                                                                      
XawTextPosition XawTextLastPosition(Widget textw) {
  return (((TextWidget)textw)->text.lastPos);
}

};
#endif

using namespace std;

namespace xwin {

  /// ディレクトリ・エントリを入手する
  bool load_dirent(const char *dirpath, vector<string> &entries, bool with_hidden_file = false) {
    DIR *dir = opendir(dirpath);
    if (dir == NULL) {
      cerr << "opendir:" << dirpath << ":" << strerror(errno) << endl;
      return false;
    }
    struct dirent *ent;
    while (ent = readdir(dir)) {
      if (!with_hidden_file && ent->d_name[0] == '.') continue;
      string dname = ent->d_name;
      entries.push_back(dname);
    }
    if (closedir(dir) < 0) {
      cerr << "closedir:" << dirpath << ":" << strerror(errno) << endl;
    }
    return true;
  }

  /// std:string をXtのStrinの配列で参照する
  String *as_String_array(vector<string> &entries) {
    int n = entries.size();
    String *a = (String *)XtMalloc(sizeof (String) * (n + 1));
    for (int i = 0; i < n; i++) {
      a[i] = (char *)entries[i].c_str();
      //　c_str で得たポインタなので、編集しないことが前提
    }
    a[n] = 0;
    return a;
  }

};

