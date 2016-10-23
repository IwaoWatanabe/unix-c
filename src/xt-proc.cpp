/*! \file
 * \brief Intrinsic 共通で利用できる手続きが含まれます。
 */

#include <iostream>

#include <X11/IntrinsicP.h>
#include <X11/ObjectP.h>
#include <X11/CoreP.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>

#include "xt-proc.h"

using namespace std;

extern "C" {
  /// クラス名を入手する
  const char *getXtClassName(WidgetClass klass) {
    return ((WidgetClassRec *)klass)->core_class.class_name;
  }

  /// 親クラスを入手する
  WidgetClass getXtSuperClass(WidgetClass klass) {
    return ((WidgetClassRec *)klass)->core_class.superclass;
  }


  String getXtName(Widget widget) {
    return ((WidgetRec *)widget)->core.name;
  }

  WidgetList getXtChildren(Widget widget) {
    return ((CompositeRec *)widget)->composite.children;
  }

  Cardinal getXtChildrenCount(Widget widget) {
    return ((CompositeRec *)widget)->composite.num_children;
  }

  /// コンポーネント・ツリーを表示する
  void show_component_tree(Widget target, int level) {
    
    if (!target || !XtIsWidget(target)) return;
  
    for (int i = 0; i < level; i++) cout << ' ';
    cout << getXtName(target) << ":" << getXtClassName(XtClass(target)) << endl;

    if (!XtIsComposite(target)) return;

    WidgetList children = getXtChildren(target);
    Cardinal n = getXtChildrenCount(target);
  
    while ( n-- > 0 ) {
      show_component_tree(*children++, level + 1);
    }
  }

  /// アプリケーション・シェルの数
  /** 複数利用する場合は増やすこと
   */
  int app_shell_count;

  /// ウィジェットを破棄するワーク処理
  Boolean dispose_work_proc(XtPointer closure) {
    Widget w = (Widget)closure;

    if (XtIsApplicationShell(w)) {
      if (--app_shell_count <= 0) 
	XtAppSetExitFlag(XtWidgetToApplicationContext(w));
    }

    cerr << "TRACE: disposing .. " << XtName(w) << endl;
    XtDestroyWidget(w);
    cerr << "TRACE: dispose end" << endl;
    return True;
  }

  /// シェルを入手する
  Widget find_shell(Widget t, Boolean need_top_level) {

    if (need_top_level) {
      // トップレベルのシェルを探索する
      Widget p;
      while (t && (p = XtParent(t))) { t = p; }
      if (t && XtIsShell(t)) return t;
      return None;
    }

    while (t) {
      if (XtIsShell(t)) return t;
      t = XtParent(t);
    }
    return None;
  }

  /// シェルを破棄する
  void dispose_shell(Widget w) {
    cerr << "TRACE: dispose called." << endl;
    if (!w) return;

    Widget shell = find_shell(w, True);
    if (!shell) return;

    cerr << "TRACE: toplevel shell: " << XtName(shell) << endl;

    XtAppContext context = XtWidgetToApplicationContext(shell);
    XtAppAddWorkProc(context, dispose_work_proc, shell);
    /*
     * 他のコールバック処理を考慮して、
     * このタイミングでは予約するだけで
     * 実際の削除処理は後回しにする
     */
  }

};


