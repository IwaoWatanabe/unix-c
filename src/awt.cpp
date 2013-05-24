/*! \file
 * \brief Athena Widget Setを使ったサンプルコード
 */

#include <iostream>
#include <string>
#include <cstring>
#include <cctype>
#include <time.h>
#include <map>

#include <X11/StringDefs.h>
#include <X11/Shell.h>
#include <X11/Xutil.h>
#include <X11/Xaw/AsciiText.h>
#include <X11/Xaw/Box.h>
#include <X11/Xaw/Command.h>
#include <X11/Xaw/Dialog.h>
#include <X11/Xaw/Form.h>
#include <X11/Xaw/List.h>
#include <X11/Xaw/MenuButton.h>
#include <X11/Xaw/MultiSink.h>
#include <X11/Xaw/MultiSrc.h>
#include <X11/Xaw/Paned.h>
#include <X11/Xaw/Panner.h>
#include <X11/Xaw/Porthole.h>
#include <X11/Xaw/Repeater.h>
#include <X11/Xaw/Scrollbar.h>
#include <X11/Xaw/SimpleMenu.h>
#include <X11/Xaw/SmeBSB.h>
#include <X11/Xaw/SmeLine.h>
#include <X11/Xaw/StripChart.h>
#include <X11/Xaw/Tree.h>
#include <X11/Xaw/Toggle.h>
#include <X11/Xaw/Viewport.h>
#include <X11/Xmu/Atoms.h>
#include <X11/Xmu/Editres.h>

#include "xt-proc.h"

using namespace std;

// --------------------------------------------------------------------------------

extern int getXawListColumns(Widget list);
extern int getXawListRows(Widget list);

namespace xwin {

  /// トップレベル・ウィンドウだけ表示する
  static int onlytop(int argc, char **argv) {

    static String app_class = "OnlyTop", 
      fallback_resouces[] = { "*geometry: 120x100", 0, };
    /*
      fallbackのアドレスは、スタック上に作成してはいけないため、
      グローバル変数とするか、static　修飾を指定すること。
    */
    
    static XrmOptionDescRec options[] = {};
    XtAppContext context;
    Widget top_level_shell =
      XtVaAppInitialize(&context, app_class, options, XtNumber(options),
			&argc, argv, fallback_resouces, NULL);

    XtRealizeWidget(top_level_shell);
    show_component_tree(top_level_shell);
 
    XtAppMainLoop(context);
    return 0;
  }

};


// --------------------------------------------------------------------------------

namespace xwin {

  static Atom WM_PROTOCOLS, WM_DELETE_WINDOW, COMPOUND_TEXT, MULTIPLE;

  /// アトムを入手する
  static void xatom_initialize(Display *display) {
    struct {
      Atom *atom_ptr;
      char *name;
    } ptr_list[] = {
      { &WM_PROTOCOLS, "WM_PROTOCOLS", },
      { &WM_DELETE_WINDOW, "WM_DELETE_WINDOW", },
      { &COMPOUND_TEXT, "COMPOUND_TEXT", },
      { &MULTIPLE, "MULTIPLE", },
    }, *i;
    
    for (i = ptr_list; i < ptr_list + XtNumber(ptr_list); i++)
      *i->atom_ptr = XInternAtom(display, i->name, True );
  }

  /// トップレベル・シェルの破棄する
  static void quit_application(Widget widget, XtPointer client_data, XtPointer call_data) {
    dispose_shell(widget);
    cerr << "TRACE: quit application called." << endl;
  }
  
  /// WMからウインドウを閉じる指令を受け取った時の処理
  static void dispose_handler(Widget widget, XtPointer closure,
			      XEvent *event, Boolean *continue_to_dispatch) {
    switch(event->type) {
    default: return;
    case ClientMessage:
      if (event->xclient.message_type == WM_PROTOCOLS &&
	  event->xclient.format == 32 &&
	  (Atom)*event->xclient.data.l == WM_DELETE_WINDOW) {
	dispose_shell(widget);
      }
    }
  }

  /// 閉じるボタンだけのアプリケーション
  static void create_button_app(Widget shell) {
    Widget panel = 
      XtVaCreateManagedWidget("panel", boxWidgetClass, shell, NULL);
    Widget close = 
      XtVaCreateManagedWidget("close", commandWidgetClass, panel, NULL);
    XtAddCallback(close, XtNcallback, quit_application, 0);

    XtInstallAccelerators(panel, close);
    /*
      close のアクセラレータをpanelにも適応する
     */
    XtRealizeWidget(shell);

    XtAddEventHandler(shell, NoEventMask, True, dispose_handler, 0);
    XSetWMProtocols(XtDisplay(shell), XtWindow(shell), &WM_DELETE_WINDOW, 1 );
    /*
      DELETEメッセージを送信してもらうように
      ウィンドウ・マネージャに依頼している
     */
  }

  /// 複数のアプリケーション構成を実験
  static int awt_multi_apps(int argc, char **argv) {
    static String app_class = "Multi", 
      fallback_resouces[] = { 
      "Multi.geometry: 300x200",
      "*font: -adobe-helvetica-bold-r-*-*-34-*-*-*-*-*-*-*",
      "*.close.accelerators: #override "
      " Ctrl<KeyPress>w: set() notify() unset()\\n",
      NULL,
    };

    static XrmOptionDescRec options[] = { };

    XtAppContext context;
    Widget top = 
      XtVaOpenApplication(&context, app_class, options, XtNumber(options),
			  &argc, argv, fallback_resouces, applicationShellWidgetClass, NULL);
    app_shell_count++;

    xatom_initialize(XtDisplay(top));
    create_button_app(top);

    // ２つめ以後のシェルとアプリケーションを作成する
    Display *display = XtDisplay(top);
    for (int i = 0; i < 3; i++) {
      top = XtVaAppCreateShell(NULL, app_class, applicationShellWidgetClass, display, NULL);
      app_shell_count++;
      create_button_app(top);
    }

    XtAppMainLoop(context);
    XtDestroyApplicationContext(context);
    cerr << "TRACE: context destroyed." << endl;
    return 0;
  }
};

// --------------------------------------------------------------------------------

namespace xwin {

  /// メインループの関数とデストラクタを定義する
  struct xt00 {
    virtual ~xt00() { cerr << "TRACE: " << this << " deleting." << endl; }
    static int main_loop(XtAppContext context);
  };

  int xt00::main_loop(XtAppContext context) {
    XtAppMainLoop(context);
    XtDestroyApplicationContext(context);
    cerr << "TRACE: context destroyed." << endl;
    return 0;
  }
 
  /// メッセージを表示するだけの単純なアプリ
  struct hello_app : xt00 {
    /// リソース定義
    struct hello_res {
      String message;
      Boolean help;
    };
    hello_res res;
    virtual void initialize();
    virtual Widget create_shell(int *argc, char **argv);
    virtual void create_content(Widget shell, int argc, char **argv);
  };

  /// ツールキットの初期化
  void hello_app::initialize() {
    XtSetLanguageProc(NULL, NULL, NULL);
  }

  /// シェル・ウィジェットの作成
  Widget hello_app::create_shell(int *argc, char **argv) {
    static String app_class = "Hello", 
      fallback_resouces[] = { 
      "*international: True", // 国際化
      "*fontSet: -*-*-*-*-*--24-*", // フォントの指定 
      "*font: -adobe-helvetica-bold-r-*-*-34-*-*-*-*-*-*-*",
      "*geometry: 320x100",
      NULL,
    };
    /*
      フォールバック・リソースはstatic 領域に作成する必要がある。
     */

    // アプリケーション追加オプションの様式定義
    static XrmOptionDescRec options[] = {
      { "-msg", ".Message", XrmoptionSepArg, NULL, },
    };

    XtAppContext context;
    Widget top_level_shell = 
      XtVaAppInitialize(&context, app_class, options, XtNumber(options),
			argc, argv, fallback_resouces, NULL);
    return top_level_shell;
  }

  /// アプリケーション・インスタンスの破棄
  static void delete_app_proc( Widget widget, XtPointer client_data, XtPointer call_data) {
    xt00 *app = (xt00 *)client_data;
    cerr << "TRACE: app deleteing.. " << app << endl;
    delete app;
  }

  /// 引数の確認用
  static void show_args(int argc, char **argv) {
    cerr << "argc: " << argc << endl;
    cerr << "optind: " << optind << endl;
    
    for (int i = 0; i < argc; i++)
      cerr << i << ": " << argv[i] << endl;
  }

  /// アプリケーション・リソースの様式定義
  void hello_app::create_content(Widget shell, int argc, char **argv) {
    static XtResource resources[] = {
      { "message", "Message", XtRString, sizeof(String),
	XtOffset(hello_res *, message), XtRImmediate, (XtPointer)"Hello,world" },
    };
    
    XtVaGetApplicationResources(shell, &res, resources, XtNumber(resources), NULL);
    cerr << "TRACE: res.message:" << res.message << endl;
    show_args(argc,argv);
    
    Widget label = 
      XtVaCreateManagedWidget("message", labelWidgetClass, shell, 
			      XtNjustify, XtJustifyCenter,
			      XtNlabel, XtNewString(res.message),
			      NULL);
    if (argc > 1) {
      // 渡されたパラメータを表示する
      string cap;
      const char *rep = "";
      for (int i = 1; i < argc; i++) {
	cap.append(rep).append(argv[i]);
	rep = " ";
      }
      cerr << cap << endl;
      
      XtVaSetValues(label, XtNlabel, XtNewString(cap.c_str()), NULL);
    }
  
    XtAddCallback(shell, XtNdestroyCallback, delete_app_proc, this);
  }

  /// アプリケーションの実行
  static int run(hello_app *app,int *argc,char **argv) {
    app->initialize();
    Widget shell = app->create_shell(argc, argv);
    app->create_content(shell, *argc, argv);
    XtRealizeWidget(shell);

    xatom_initialize(XtDisplay(shell));
    XtAddEventHandler(shell, NoEventMask, True, dispose_handler, 0);
    XSetWMProtocols(XtDisplay(shell), XtWindow(shell), &WM_DELETE_WINDOW, 1 );
    
    XtAppContext context = XtWidgetToApplicationContext(shell);
    return app->main_loop(context);
  }

  /// メッセージを出現させる
  static int awt_hello(int argc, char **argv) {
    return run(new hello_app, &argc,argv);
  }

};


// --------------------------------------------------------------------------------

namespace xwin {

  /// ダイアログをウィジェット相対で表示する
  static void popup_dialog(Widget target, Widget shell, XtGrabKind kind) {
    Position x, y, rootx, rooty;
    Dimension width, height;

    if (!shell) return;

    // ポップアップすべき位置の計算
    XtVaGetValues(target, XtNx, &x, XtNy, &y, 
		  XtNwidth, &width, XtNheight, &height, NULL);
    
    XtTranslateCoords(XtParent(target), x, y, &rootx, &rooty );

    XtVaSetValues(shell, XtNx, rootx + width - 25,
		XtNy, rooty + height - 25, NULL );
    XtPopup(shell, kind);
  }

  /** Commandに登録するコールバック
   * @param widget この関数を呼び出したCommandの参照
   * @param client_data コールバック登録で渡したデータ
   * @param call_data Commandの場合は未使用
   */
  static void open_dialog(Widget widget, XtPointer client_data, XtPointer call_data) {
    Widget shell = (Widget)client_data;
    // モーダルダイアログ（ただし他のウィンドウとは非排他的)
    popup_dialog(widget, shell, XtGrabNonexclusive);
  }

  /// ダイアログのボタンが押されたタイミングでポップダウンさせるためのコールバック
  static void close_dialog(Widget widget, XtPointer client_data, XtPointer call_data) {
    Widget dialog = (Widget)client_data;
    XtPopdown(dialog);
  }

  /// ダイアログを表示する
  struct dialog_app : hello_app {
    virtual Widget create_shell(int *argc, char **argv);
    virtual void create_content(Widget shell, int argc, char **argv);
  };


  Widget dialog_app::create_shell(int *argc, char **argv) {
    static String app_class = "Hello2", 
      fallback_resouces[] = { 
      "*international: True",
      "*fontSet: -*-*-*-*-*--24-*",
      "*font: -adobe-helvetica-bold-r-*-*-34-*-*-*-*-*-*-*",
      "*geometry: 320x100",
      "*.press.label: Press Me!",
      "*.press.accelerators: Meta<KeyPress>p: set() notify() unset()\\n"
      " <KeyPress>space: set() notify() unset()",
      
      "*.dialog.label: Do You want to quit ?",
      "*.yes.accelerators: <KeyPress>y: set() notify() unset() \\n"
      " <KeyPress>space: set() notify() unset()",
      
      "*.no.accelerators: <KeyPress>n: set() notify() unset()\\n"
      " <KeyPress>Escape: set() notify() unset()",
      NULL,
    };

    XtAppContext context;
    return 
      XtVaAppInitialize(&context, app_class, NULL, 0,
			argc, argv, fallback_resouces, NULL);
  }
  
  /// 子ウィジェットを探して、アクセラレータを登録する
  static void install_accelerators(Widget parent, const char *name) {
    Widget wi = XtNameToWidget(parent, name);
    if (wi) {
      XtAccelerators accelerators;
      XtVaGetValues(wi, XtNaccelerators, &accelerators, NULL);
      XtInstallAccelerators(parent, wi);
      cerr << "TRACE: widget " << name << ":" << getXtName(wi) << endl;
      cerr << "TRACE: accelerators: " << accelerators << endl;
    }
  }

  void dialog_app::create_content(Widget top, int argc, char **argv) {
    Widget shell = XtVaCreatePopupShell("confirm", transientShellWidgetClass, top, NULL );
    Widget dialog = XtVaCreateManagedWidget("dialog", dialogWidgetClass, shell, NULL );
    /* ウィジット関数によってダイアログ内部のボタンをセットする */
    XawDialogAddButton(dialog, "yes", quit_application, 0 );
    XawDialogAddButton(dialog, "no", close_dialog, shell );

    install_accelerators(dialog, "yes");
    install_accelerators(dialog, "no");

    Widget box = XtVaCreateManagedWidget("box", boxWidgetClass, top, NULL);
    Widget press = XtVaCreateManagedWidget("press", commandWidgetClass, box, NULL);
    XtAddCallback(press, XtNcallback, open_dialog, shell);

#if 1  
    cerr << "install to box" << endl;
    XtInstallAccelerators(box, press);
#else
    // こちらは必要なかった
    cerr << "install to shell" << endl;
    XtInstallAccelerators(top, press);
#endif
  }

  /// ポップアップ・ダイアログを出現させる
  static int awt_dialog(int argc, char **argv) {
    return run(new dialog_app, &argc,argv);
  }

  /// ダイアログを出現して終了する
  static int awt_dialog02(int argc, char **argv) {

    static String app_class = "MessageBox", 
      fallback_resouces[] = { 
      "*international: True",
      "*fontSet: -*-*-*-*-*--24-*",
      "*font: -adobe-helvetica-bold-r-*-*-34-*-*-*-*-*-*-*",
      "*.ok.accelerators: <KeyPress>Return: set() notify() unset()\\n"
      " <KeyPress>space: set() notify() unset()",
      NULL,
    };

    XtSetLanguageProc(NULL, NULL, NULL);

    XtAppContext context;
    static XrmOptionDescRec options[] = { };

    Widget top = 
      XtVaOpenApplication(&context, app_class, options, XtNumber(options),&argc,argv,
			  fallback_resouces, applicationShellWidgetClass, NULL);

    Widget dialog = XtVaCreateManagedWidget("dialog", dialogWidgetClass, top, NULL );
    /* ウィジット関数によってダイアログ内部のボタンをセットする */
    XawDialogAddButton(dialog, "ok", quit_application, 0 );
    XtVaSetValues(dialog, XtNlabel, XtNewString("press ok to close application."), NULL);

    install_accelerators(dialog, "ok");
  
    XtRealizeWidget(top);
    XtAppMainLoop(context);
    XtDestroyApplicationContext(context);
    cerr << "TRACE: context destroyed." << endl;
    return 0;
  }

};

// --------------------------------------------------------------------------------

namespace xwin {

  /// クラス階層を表示するアプリのデータを保持する
  struct tree_app : xt00 { 
    /// クラス名とそれを表示するウィジェットの組を保持する
    map<string, Widget> nodes;

    Widget find_node(WidgetClass wc);
    Widget add_node(WidgetClass wc, Widget tree);
    void create_contents(Widget top);
  };

  /// クラスに対応するwidgetを入手する
  Widget tree_app::find_node(WidgetClass wc) {
    const char *cn = getXtClassName(wc);
    map<string, Widget>::iterator it = nodes.find(cn);
    // クラス名でウィジェットを引き出す
    if (it == nodes.end()) return None;
    return (*it).second;
  }

  /// クラスに対応するwidgetを追加する
  Widget tree_app::add_node(WidgetClass wc, Widget tree) {
    if (!wc) return None;

    Widget t = find_node(wc);
    if (t) return t;

    const char *cn = getXtClassName(wc);
    WidgetClass superClass = getXtSuperClass(wc);
    if (!superClass) {
      // トップレベルのノード
      Widget re = 
	XtVaCreateManagedWidget(cn, commandWidgetClass, tree, 
				XtNlabel, XtNewString(cn), NULL);
      nodes[cn] = re;
      return re;
    }

    Widget parent = find_node(superClass);
    if (parent == None) parent = add_node(superClass, tree);
  
    Widget re = 
      XtVaCreateManagedWidget(cn, commandWidgetClass, tree, 
			      XtNtreeParent, parent, 
			      XtNlabel, XtNewString(cn), NULL);
    nodes[cn] = re;
    return re;
  }

  /// pannaer が操作されたタイミングで portholeのウィジェットの位置を変えるためのコールバック
  static void panner_callback (Widget panner,XtPointer closure,XtPointer data) {
    XawPannerReport *rep = (XawPannerReport *)data;
    Widget target = (Widget)closure;
    Arg args[2];
    if (!target) return;
    XtSetArg (args[0], XtNx, -rep->slider_x);
    XtSetArg (args[1], XtNy, -rep->slider_y);
    XtSetValues (target, args, 2);
  }

  /// portholeの大きさが変わったときに、pannerの大きさを調整するためのコールバック
  static void porthole_callback(Widget porthole, XtPointer closure, XtPointer data) {
    Widget panner = (Widget) closure;
    XawPannerReport *rep = (XawPannerReport *)data;
    Arg args[6];
    Cardinal n = 2;

    XtSetArg (args[0], XtNsliderX, rep->slider_x);
    XtSetArg (args[1], XtNsliderY, rep->slider_y);

    if (rep->changed != (XawPRSliderX | XawPRSliderY)) {
      XtSetArg (args[2], XtNsliderWidth, rep->slider_width);
      XtSetArg (args[3], XtNsliderHeight, rep->slider_height);
      XtSetArg (args[4], XtNcanvasWidth, rep->canvas_width);
      XtSetArg (args[5], XtNcanvasHeight, rep->canvas_height);
      n = 6;
    }
    XtSetValues(panner, args, n);
  }

  void tree_app::create_contents(Widget top) {
    Widget form = 
      XtVaCreateManagedWidget("form",formWidgetClass, top, NULL);

    Widget panner = 
      XtVaCreateManagedWidget("panner", pannerWidgetClass, form, NULL);

    Widget porthole = 
      XtVaCreateManagedWidget("porthole",portholeWidgetClass, form,
			    XtNbackgroundPixmap, None, NULL);

    Widget tree = 
      XtVaCreateManagedWidget("tree", treeWidgetClass, porthole, NULL);

    XtAddCallback(porthole, XtNreportCallback, porthole_callback, panner);
    XtAddCallback(panner, XtNreportCallback, panner_callback, tree);

    WidgetClass awclasses[] = {
      applicationShellWidgetClass,
      asciiSinkObjectClass,
      asciiSrcObjectClass,
      asciiTextWidgetClass,
      boxWidgetClass,
      commandWidgetClass,
      dialogWidgetClass,
      labelWidgetClass,
      listWidgetClass,
      menuButtonWidgetClass,
      panedWidgetClass,
      pannerWidgetClass,
      portholeWidgetClass,
      repeaterWidgetClass,
      scrollbarWidgetClass,
      stripChartWidgetClass,
      toggleWidgetClass,
      treeWidgetClass,
      multiSinkObjectClass,
      multiSrcObjectClass,
      smeBSBObjectClass,
      smeLineObjectClass,
      sessionShellWidgetClass,
      viewportWidgetClass,
      simpleMenuWidgetClass,
      NULL,
    }, *wc = awclasses;

    while (*wc) {
      add_node(*wc++, tree);
    }

    XtRealizeWidget(top);
    XtAddEventHandler(top, NoEventMask, True, dispose_handler, 0);
    XSetWMProtocols(XtDisplay(top), XtWindow(top), &WM_DELETE_WINDOW, 1 );
  
    Dimension canvas_width, canvas_height, slider_width, slider_height;
    XtVaGetValues(tree,
		  XtNwidth, &canvas_width,
		  XtNheight, &canvas_height,
		  NULL);

    XtVaGetValues(porthole,
		  XtNwidth, &slider_width,
		  XtNheight, &slider_height,
		  NULL);
  
    XtVaSetValues(panner,
		  XtNcanvasWidth, &canvas_width,
		  XtNcanvasHeight, &canvas_height,
		  XtNsliderWidth, &slider_width,
		  XtNsliderHeight, &slider_height,
		  NULL);
  
    XRaiseWindow (XtDisplay(panner), XtWindow(panner));

    XtAddCallback(top, XtNdestroyCallback, delete_app_proc, this);
  }

  /// アテナ・ウィジェットのクラス階層を表示する
  static int awt_class_tree(int argc, char **argv) {

    static String app_class = "AwTree", 
      fallback_resouces[] = { 
      "*tree.gravity: west",
      "*geometry: 800x400",
      NULL,
    };

    static XrmOptionDescRec options[] = { };

    XtAppContext context;
    Widget top = 
      XtVaOpenApplication(&context, app_class, options, XtNumber(options),
			  &argc, argv, fallback_resouces, applicationShellWidgetClass, NULL);

    xatom_initialize(XtDisplay(top));
    tree_app *app = new tree_app();
    app->create_contents(top);
    return app->main_loop(context);
  }

};


// --------------------------------------------------------------------------------

#include "subcmd.h"

subcmd awt_cmap[] = {
  { "top", xwin::onlytop, },
  { "win03", xwin::onlytop, },
  { "multi", xwin::awt_multi_apps, },
  { "hello02", xwin::awt_hello, },
  { "dialog", xwin::awt_dialog, },
  { "dialog02", xwin::awt_dialog02, },
  { "tree", xwin::awt_class_tree, },
  { 0 },
};

