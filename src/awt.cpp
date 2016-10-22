/*! \file
 * \brief Athena Widget Setを使ったサンプルコード

  Xlibがサポートするロケールと libcがサポートするロケールが合致していないと
  エディタはうまく動作しない点に注意すること。

  CentOS5.x で提供される ja_JP.utf8 はXlibがサポートしていない。
  一見動作しているようだが、処理中に バッファオーバフローが検出されて停止してしまう。

  この問題を回避するために例えば
  # localedef -f EUC-JP -i ja_JP /usr/lib/locale/ja_JP.eucJP
  として、ja_JP.eucJP を定義して、これを利用する必要がある。
 */

#include <iostream>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <clocale>
#include <time.h>
#include <vector>
#include <map>
#include <typeinfo>

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
#include <X11/Xaw/XawInit.h>
#include <X11/Xmu/Atoms.h>
#include <X11/Xmu/Editres.h>

#include "xt-proc.h"
#include "elog.hpp"

#include "SmeCascade.h"

using namespace std;

// --------------------------------------------------------------------------------

extern int getXawListColumns(Widget list);
extern int getXawListRows(Widget list);

extern "C" {
  extern char *trim(char *t);
  extern char *demangle(const char *demangle);

  static void report_action(Widget wi, XEvent *event, String *params, Cardinal *num);
};


// --------------------------------------------------------------------------------

namespace {

  using namespace xwin;

  /// 空フレームだけのアプリ
  class Simple_Frame : public Frame { };

  /// 空フレームの生成クラス
  class Simple_Frame_Factory : public Frame_Factory {
    Frame *get_instance() { return new Simple_Frame(); }
  };
};

// --------------------------------------------------------------------------------

namespace xwin {

  extern wstring load_wtext(const char *path, size_t buf_len = 4096);
  extern bool load_dirent(const char *dirpath, vector<string> &entries, bool with_hidden_file = false);
  extern String *as_String_array(vector<string> &entries);


  Frame::~Frame() { }

  /// 一連のファクトリを記録する
  vector<Frame_Factory *> frame_factories;

  Frame_Factory::Frame_Factory() { frame_factories.push_back(this); }

  Server_Resource::Server_Resource(Widget shell)
    : owner(shell), WM_PROTOCOLS(0), WM_DELETE_WINDOW(0), COMPOUND_TEXT(0) { }


  Server_Resource::~Server_Resource() { }
};

namespace {

  enum Error_Level { F, E, W, N, I, A, D, T };

/// トップレベル・ウィンドウを管理する
  class Frame_Manager {
  protected:
    Server_Resource *resource;
  };

  /// サーバリソース管理の実装クラス
  class Server_Resource_Impl : public Server_Resource {
  public:
    Server_Resource_Impl(Widget owner);
    ~Server_Resource_Impl();

    Atom atom_value_of(const char *name);
    std::string text_of(Atom atom);

    void open_frame(Frame *frame);

    static void dispose_handler(Widget widget, XtPointer closure,
				XEvent *event, Boolean *continue_to_dispatch);

    static void delete_frame_proc( Widget widget, XtPointer client_data, XtPointer call_data);
  };

  Server_Resource_Impl::Server_Resource_Impl(Widget shell)
    : Server_Resource(shell)
  {
    WM_PROTOCOLS = atom_value_of("WM_PROTOCOLS");
    WM_DELETE_WINDOW = atom_value_of("WM_DELETE_WINDOW");
    COMPOUND_TEXT = atom_value_of("COMPOUND_TEXT");
  }

  Server_Resource_Impl::~Server_Resource_Impl() { }

  Atom Server_Resource_Impl::atom_value_of(const char *name) {
    return XInternAtom(XtDisplay(owner), name, True);
  }

  string Server_Resource_Impl::text_of(Atom atom) {
    return XGetAtomName(XtDisplay(owner), atom);
  }

  void Server_Resource_Impl::open_frame(Frame *frame) {
    frame->create_contents(owner);
    XtRealizeWidget(owner);

    XtAddEventHandler(owner, NoEventMask, True, dispose_handler, this);

    XSetWMProtocols(XtDisplay(owner), XtWindow(owner), &WM_DELETE_WINDOW, 1 );
    /*
      DELETEメッセージを送信してもらうように
      ウィンドウ・マネージャに依頼している
     */

    XtAddCallback(owner, XtNdestroyCallback, delete_frame_proc, frame);
  }

  /// WMからウインドウを閉じる指令を受け取った時の処理
  void Server_Resource_Impl::dispose_handler(Widget widget, XtPointer closure,
			      XEvent *event, Boolean *continue_to_dispatch) {

    Server_Resource *sr = (Server_Resource *)closure;
    switch(event->type) {
    default: return;
    case ClientMessage:
      if (event->xclient.message_type == sr->WM_PROTOCOLS &&
	  event->xclient.format == 32 &&
	  (Atom)*event->xclient.data.l == sr->WM_DELETE_WINDOW) {
	dispose_shell(widget);
      }
    }
  }

  /// フレーム・インスタンスの破棄
  void Server_Resource_Impl::delete_frame_proc( Widget widget, XtPointer client_data, XtPointer call_data) {
    Frame *fr = (Frame *)client_data;
    if (fr) fr->release();
    cerr << "TRACE: frame deleteing.. " << fr << endl;
    delete fr;
  }

  /// トップレベル・シェルの破棄する
  static void quit_application(Widget widget, XtPointer client_data, XtPointer call_data) {
    dispose_shell(widget);
    cerr << "TRACE: quit application called." << endl;
  }

  /// パラメータにキーが含まれているか診断する
  static bool param_contains(String name, String *params, Cardinal num) {
    if (name == NULL || strcasecmp(name,"") == 0 || num == 0) return false;
    for (Cardinal n = 0; n < num; n++) {
      if (strcasecmp(name, params[n]) == 0) return true;
    }
    return false;
  }

  /// リスト項目で次のエントリに移動するアクション
  /**
   * パラメータとして有効なのは next, previous, above, below
   * それ以外のパラメータが渡ってきた場合は無視する
   */
  static void change_item_action(Widget list, XEvent *event, String *params, Cardinal *num) {
    XawListReturnStruct *rs = XawListShowCurrent(list);

    int numberStrings, columns;
    Boolean isVertical;

    XtVaGetValues(list, XtNnumberStrings, &numberStrings,
		  XtNdefaultColumns, &columns,
		  XtNverticalList, &isVertical,
		  NULL );

    columns = getXawListColumns(list);

#if 0
    cerr << "TRACE: " << numberStrings << " items." << endl;
    cerr << "TRACE: index: " << rs->list_index << " "
	 << "columns: " << columns << " "
	 << "vertical: " << isVertical << endl;
#endif

    if (num == 0 || param_contains("next",params,*num)) {
      if (rs->list_index + 1 < numberStrings)
	XawListHighlight(list, ++rs->list_index);
    }
    else if (param_contains("previous",params,*num)) {
      if (rs->list_index > 0)
	XawListHighlight(list, --rs->list_index);
    }
    else if (param_contains("below",params,*num)) {
      if (rs->list_index + columns < numberStrings)
	XawListHighlight(list, rs->list_index += columns);
    }
    else if (param_contains("above",params,*num)) {
      if (rs->list_index >= columns)
	XawListHighlight(list, rs->list_index -= columns);
    }
    else {
      cerr << "WARN: unnkown no operation" << endl;
      return;
    }

    rs = XawListShowCurrent(list);
    XtCallCallbacks(list, XtNcallback, (XtPointer)rs);
  }

  // ウィジェットの名前を元にメニューを探す
  static Widget find_menu(Widget widget, String name) {
    Widget w, menu;

    for (w = widget; w != NULL; w = XtParent(w))
      if ((menu = XtNameToWidget(w, name)) != NULL) return menu;
    return NULL;
  }

  /// メニューの表示位置を調整する
  static void PositionMenu(Widget w, XPoint *loc) {
    Arg arglist[2];
    Cardinal num_args = 0;

    XtRealizeWidget(w);

    XtSetArg(arglist[num_args], XtNx, loc->x); num_args++;
    XtSetArg(arglist[num_args], XtNy, loc->y); num_args++;
    XtSetValues(w, arglist, num_args);
  }

  /// ポップアップ・メニューの表示
  static void popup_action(Widget w, XEvent *event, String *params, Cardinal *num) {
    cerr << "TRACE: " << XtName(w) << ":popup." << endl;

    Widget menu;
    XPoint loc;

    if (*num != 1) {
      XtAppWarning(XtWidgetToApplicationContext(w),
		   "SimpleMenuWidget: position menu action expects "
		   "only one parameter which is the name of the menu.");
      return;
    }

    if ((menu = find_menu(w, params[0])) == NULL) {
      char error_buf[BUFSIZ];
      snprintf(error_buf, sizeof(error_buf),
	       "SimpleMenuWidget: could not find menu named %s.",
	       params[0]);
      XtAppWarning(XtWidgetToApplicationContext(w), error_buf);
      return;
    }

    Boolean spring_loaded = False;
    switch (event->type) {
    case ButtonPress:
      spring_loaded = True;
    case ButtonRelease:
      loc.x = event->xbutton.x_root;
      loc.y = event->xbutton.y_root;
      PositionMenu(menu, &loc);
      break;
    case EnterNotify:
    case LeaveNotify:
      loc.x = event->xcrossing.x_root;
      loc.y = event->xcrossing.y_root;
      PositionMenu(menu, &loc);
      break;
    case MotionNotify:
      loc.x = event->xmotion.x_root;
      loc.y = event->xmotion.y_root;
      PositionMenu(menu, &loc);
      break;
    default:
      PositionMenu(menu, NULL);
      break;
    }
#if 0
    XtMenuPopupAction(w,event,params,num);
#else
    if (spring_loaded)
      XtPopupSpringLoaded(menu);
    else
      XtPopup(menu, XtGrabNonexclusive);
#endif
  }

  static void menu_popup_proc( Widget widget, XtPointer client_data, XtPointer call_data) {
    cout << "TRACE: " << XtName(widget) << " popup" << endl;
  }

  static void menu_popdown_proc( Widget widget, XtPointer client_data, XtPointer call_data) {
    cout << "TRACE: " << XtName(widget) << " popdown" << endl;
  }

  /** AWのポップアップメニューを作成する
   */
  static Widget create_popup_menu(String menu_name, Widget shell, Menu_Item *items) {

    Widget menu =
      XtVaCreatePopupShell( menu_name, simpleMenuWidgetClass, shell, NULL );

    XtAddCallback(menu, XtNpopupCallback, menu_popup_proc, 0);
    XtAddCallback(menu, XtNpopdownCallback, menu_popdown_proc, 0);

    Widget item;

    for (int i = 0; items[i].name; i++) {
      if (strcmp("-", items[i].name) == 0) {
	XtVaCreateManagedWidget("-", smeLineObjectClass, menu, NULL );
	continue;
      }

      if (items[i].sub_menu) {
	Widget sub = create_popup_menu(items[i].name, shell, items[i].sub_menu);
	item = XtVaCreateManagedWidget(items[i].name, smeCascadeObjectClass, menu,
				       XtNsubMenu, sub, NULL );
	continue;
      }

      item = XtVaCreateManagedWidget(items[i].name, smeBSBObjectClass, menu, NULL );
      if (items[i].proc)
	XtAddCallback(item, XtNcallback, items[i].proc, items[i].closure);
    }
    return menu;
  }

  // --------------------------------------------------------------------------------

  /// ラベルだけ配置したフレーム
  class Message_Frame : public Frame {
    /// リソース定義
    struct hello_res { String message; Boolean help; };
    hello_res res;
  public:
    Widget create_contents(Widget shell);
  };

  /// ラベルだけ・フレームの生成クラス
  class Message_Frame_Factory : public Frame_Factory {
    Frame *get_instance() { return new Message_Frame(); }
  };

  Widget Message_Frame::create_contents(Widget shell) {

    static XtResource resources[] = {
      { "message", "Message", XtRString, sizeof(String),
	XtOffset(hello_res *, message), XtRImmediate, (XtPointer)"Hello,world" },
    };

    XtVaGetApplicationResources(shell, &res, resources, XtNumber(resources), NULL);
    cerr << "TRACE: res.message:" << res.message << endl;

    Widget label =
      XtVaCreateManagedWidget("message", labelWidgetClass, shell,
			      XtNjustify, XtJustifyCenter,
			      XtNlabel, XtNewString(res.message),
			      NULL);

    const char *argv[]= { "aa", "bb", "cc", };
    int argc = XtNumber(argv);

    if (0 && argc > 1) {
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

  }

  // --------------------------------------------------------------------------------

  /// ボタン一つだけ配置したフレーム
  class Button_Frame : public Frame {

    Widget create_contents(Widget shell) {
      Widget panel =
	XtVaCreateManagedWidget("panel", boxWidgetClass, shell, NULL);
      Widget close =
	XtVaCreateManagedWidget("close", commandWidgetClass, panel, NULL);

      XtAddCallback(close, XtNcallback, quit_application, 0);
      XtInstallAccelerators(panel, close);
      /*
	close のアクセラレータをpanelにも適応する
      */
      return 0;
    }
  };

  /// ボタン・フレームの生成クラス
  class Button_Frame_Factory : public Frame_Factory {
    Frame *get_instance() { return new Button_Frame(); }
  };

  // --------------------------------------------------------------------------------

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

  /// ダイアログを出現させるボタンを配置したフレーム
  class Dialog_Button_Frame : public Frame {
  public:
    Widget create_contents(Widget shell);
  };

  class Dialog_Button_Frame_Factory : public Frame_Factory {
    Frame *get_instance() { return new Dialog_Button_Frame(); }
  };

  Widget Dialog_Button_Frame::create_contents(Widget top) {
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
    return 0;
  }

  // --------------------------------------------------------------------------------

  /// イベント受け取りの確認用
  class Event_View : public Frame {
    static void event_handler(Widget widget, XtPointer closure, XEvent *event, Boolean *continue_to_dispatch);
    XIM xim;
    XIMStyles *xim_styles;
    XIMStyle xim_style;
    XIC xic;
  public:
    Event_View() : xim(0), xim_styles(0), xim_style(0), xic(0) { }
    Widget create_contents(Widget shell);
  };

  class Event_View_Factory : public Frame_Factory {
    Frame *get_instance() { return new Event_View(); }
  };

  void Event_View::event_handler(Widget widget, XtPointer closure, XEvent *event, Boolean *continue_to_dispatch) {
    report_action(widget, event, 0, 0);
    *continue_to_dispatch = True;
  }

  Widget Event_View::create_contents(Widget top) {
    char *modifiers;
    char *imvalret;

    Widget pane =
      XtVaCreateManagedWidget("pane", panedWidgetClass, top, NULL );
    Widget menu_bar =
      XtVaCreateManagedWidget("menu_bar", boxWidgetClass, pane, NULL);

    Widget close =
      XtVaCreateManagedWidget("close", commandWidgetClass, menu_bar, NULL);
    XtAddCallback(close, XtNcallback, quit_application, 0);

#define INNER_WINDOW_WIDTH 50
#define INNER_WINDOW_HEIGHT 50
#define INNER_WINDOW_BORDER 4
#define INNER_WINDOW_X 10
#define INNER_WINDOW_Y 10
#define OUTER_WINDOW_MIN_WIDTH (INNER_WINDOW_WIDTH + \
				2 * (INNER_WINDOW_BORDER + INNER_WINDOW_X))
#define OUTER_WINDOW_MIN_HEIGHT (INNER_WINDOW_HEIGHT + \
				2 * (INNER_WINDOW_BORDER + INNER_WINDOW_Y))
#define OUTER_WINDOW_DEF_WIDTH (OUTER_WINDOW_MIN_WIDTH + 100)
#define OUTER_WINDOW_DEF_HEIGHT (OUTER_WINDOW_MIN_HEIGHT + 100)
#define OUTER_WINDOW_DEF_X 100
#define OUTER_WINDOW_DEF_Y 100

    Widget outer =
      XtVaCreateManagedWidget("event",simpleWidgetClass, pane,
			      XtNwidth, OUTER_WINDOW_DEF_WIDTH,
			      XtNheight, OUTER_WINDOW_DEF_HEIGHT, NULL);

    Widget outer2 =
      XtVaCreateManagedWidget("event02",simpleWidgetClass, pane,
			      XtNwidth, OUTER_WINDOW_DEF_WIDTH,
			      XtNheight, OUTER_WINDOW_DEF_HEIGHT, NULL);


    XtRealizeWidget(top);

    unsigned long border, background;
    XtVaGetValues(outer,
		  XtNforeground, &border,
		  XtNbackground, &background,
		  NULL);

    Window subw = XCreateSimpleWindow(XtDisplay(top), XtWindow(outer),
				      INNER_WINDOW_X, INNER_WINDOW_Y,
				INNER_WINDOW_WIDTH, INNER_WINDOW_HEIGHT,
				INNER_WINDOW_BORDER,
				border, background);

    XMapWindow(XtDisplay(top), subw);

    printf ("Outer window is 0x%lx, inner window is 0x%lx\n", XtWindow(outer), subw);

    xim = XOpenIM (XtDisplay(top), NULL, NULL, NULL);
    if (xim == NULL) {
      fprintf (stderr, "XOpenIM failed\n");
    }

    if (xim) {
      imvalret = XGetIMValues (xim, XNQueryInputStyle, &xim_styles, NULL);
      if (imvalret != NULL || xim_styles == NULL) {
	fprintf (stderr, "input method doesn't support any styles\n");
      }

      if (xim_styles) {
	xim_style = 0;
	for (int i = 0;  i < xim_styles->count_styles;  i++) {
	  if (xim_styles->supported_styles[i] ==
	      (XIMPreeditNothing | XIMStatusNothing)) {
	    xim_style = xim_styles->supported_styles[i];
	    break;
	  }
	}

	if (xim_style == 0) {
	  fprintf (stderr, "input method doesn't support the style we support\n");
	}
	XFree (xim_styles);
      }
    }

    EventMask event_mask = KeyPressMask | KeyReleaseMask | ButtonPressMask |
      ButtonReleaseMask | EnterWindowMask |
      LeaveWindowMask | PointerMotionMask |
      Button1MotionMask |
      Button2MotionMask | Button3MotionMask |
      Button4MotionMask | Button5MotionMask |
      ButtonMotionMask | KeymapStateMask |
      ExposureMask | VisibilityChangeMask |
      StructureNotifyMask | /* ResizeRedirectMask | */
      SubstructureNotifyMask | SubstructureRedirectMask |
      FocusChangeMask | PropertyChangeMask |
      ColormapChangeMask | OwnerGrabButtonMask;

    XtAddEventHandler(outer, event_mask, True, event_handler, NULL);

    if (xim && xim_style) {
      xic = XCreateIC (xim,
		       XNInputStyle, xim_style,
		       XNClientWindow, XtWindow(outer),
		       XNFocusWindow, XtWindow(outer),
		       NULL);

      if (xic == NULL) {
	fprintf (stderr, "XCreateIC failed\n");
      }
    }

  }


  // --------------------------------------------------------------------------------

  /// AWのクラスの継承ツリーを出力する
  class Class_Tree : public Frame {
    /// クラス名とそれを表示するウィジェットの組を保持する
    map<string, Widget> nodes;
    Widget find_node(WidgetClass wc);
    Widget add_node(WidgetClass wc, Widget tree);

  public:
    Widget create_contents(Widget shell);
  };

  class Class_Tree_Factory : public Frame_Factory {
    Frame *get_instance() { return new Class_Tree(); }
  };

  /// クラスに対応するwidgetを入手する
  Widget Class_Tree::find_node(WidgetClass wc) {
    const char *cn = getXtClassName(wc);
    map<string, Widget>::iterator it = nodes.find(cn);
    // クラス名でウィジェットを引き出す
    if (it == nodes.end()) return None;
    return (*it).second;
  }

  /// クラスに対応するwidgetを追加する
  Widget Class_Tree::add_node(WidgetClass wc, Widget tree) {
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


  Widget Class_Tree::create_contents(Widget top) {

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

    return 0;
  }

  // --------------------------------------------------------------------------------

  /// portholeの大きさが変わったときに、pannerの大きさを調整するためのコールバック
  static void viewport_callback(Widget porthole, XtPointer closure, XtPointer data) {
    XawPannerReport *rep = (XawPannerReport *)data;

    cerr << "TRACE: y:" << rep->slider_y << " x:" << rep->slider_x
	 << " slider width:" << rep->slider_width << " height:" << rep->slider_height
	 << " canvas width:" << rep->canvas_width << " height:" << rep->canvas_height
	 << " changed:" << rep->changed << endl;
  }

  /// メニューアイテムを選択した時の処理
  static void menu_selected( Widget widget, XtPointer client_data, XtPointer call_data) {
    cout << "TRACE: " << XtName(widget) << " selected." << endl;
  }

  /// AWのクラスの継承ツリーを出力する
  class Folder_List : public Frame {
    Widget list;
    vector<string> entries;

    XawListReturnStruct last_selected;
    XtIntervalId timer;
    long interval; // ms

    static void item_selected_timer(XtPointer closure, XtIntervalId *id);
    static void item_selected( Widget widget, XtPointer client_data, XtPointer call_data);

  public:
    Folder_List() : list(0), timer(0), interval(400) { }
    Widget create_contents(Widget shell);
  };

  class Folder_List_Factory : public Frame_Factory {
    Frame *get_instance() { return new Folder_List(); }
  };

  /// リストアイテムを選択した時の処理
  void Folder_List::item_selected_timer(XtPointer closure, XtIntervalId *id) {
    Folder_List *app = (Folder_List *)closure;
    app->timer = 0;
    cerr << "TRACE: item selected:" << app->last_selected.list_index
	 << ":" << app->last_selected.string << endl;
  }

  /// リストアイテムを選択した時の処理
  void Folder_List::item_selected( Widget widget, XtPointer client_data, XtPointer call_data) {
    XawListReturnStruct *rs = (XawListReturnStruct *)call_data;
    Folder_List *app = (Folder_List *)client_data;

    app->last_selected = *rs;
    // このタイミングではタイマーを設定するだけで、主要な処理は遅延実行する

    if (app->timer) XtRemoveTimeOut(app->timer);
    app->timer =
      XtAppAddTimeOut(XtWidgetToApplicationContext(widget),
		      app->interval, item_selected_timer, app);
    if (0)
      cout << "TRACE: " << XtName(widget) << " item selected." << endl;
  }

  Widget Folder_List::create_contents(Widget top) {

    xwin::load_dirent(".", entries, false);
    String *slist = xwin::as_String_array(entries);

    Widget pane =
      XtVaCreateManagedWidget("pane", panedWidgetClass, top, NULL );
    Widget menu_bar =
      XtVaCreateManagedWidget("menu_bar", boxWidgetClass, pane, NULL);

    XtVaCreateManagedWidget("list", menuButtonWidgetClass, menu_bar,
			    XtNmenuName, "list-menu",
			    NULL);

    struct Menu_Item sub_items[] = {
      { "sub-item1", menu_selected, },
      { "sub-item2", menu_selected, },
      { "sub-item3", menu_selected, },
      { 0, },
    };

    struct Menu_Item items[] = {
      { "item1", menu_selected, },
      { "item2", menu_selected, },
      { "menu-item2", menu_selected, 0, sub_items },
      { "-", },
      { "close", quit_application, },
      { 0, },
    };

    create_popup_menu("list-menu", pane, items);

    Widget viewport =
      XtVaCreateManagedWidget("viewport", viewportWidgetClass, pane,
			      XtNallowVert, True,
			      XtNuseRight, True,
			      NULL);
    XtAddCallback(viewport, XtNreportCallback, viewport_callback, 0);
    list =
      XtVaCreateManagedWidget("fslist", listWidgetClass, viewport,
			      XtNlist, (XtArgVal)slist,
			      XtNpasteBuffer, True,
			      //XtNdefaultColumns, (XtArgVal)1,
			      NULL);
    XtAddCallback(list, XtNcallback, item_selected, this);
    XtSetKeyboardFocus(pane, list);

    Widget close = 
      XtVaCreateManagedWidget("close", commandWidgetClass, menu_bar, NULL);
    XtAddCallback(close, XtNcallback, quit_application, 0);

    XtInstallAccelerators(list, close);
    XtRealizeWidget(top);



    // 初期状態で選択されているようにする
    XawListHighlight(list, 0);

    Widget vertical = XtNameToWidget(viewport, "vertical");
    if (vertical) XtInstallAccelerators(list, vertical);

    return 0;
  }

  // --------------------------------------------------------------------------------

  /// テキスト・バッファを操作する基本APIを提供する
  class Text_Buffer {
    /// カレット位置にテキストを追記する
    int append_vftext(const char *format,va_list ap);
    /// カレット位置にテキストを追記する
    int append_ftext(const char *format, ...);
    /// カレット位置にテキストを追記する
    int append_text(const char *text);
    /// 指定する行に移動する
    bool goto_line(int row);
    /// 指定する行に移動する
    bool set_carret_position(int row, int column);
    /// カレットの現在位置を入手する
    bool get_carret_position(int &row, int &column);
    /// 選択されたテキストを入手する
    wstring get_selected_text();
  };

  /// AWのクラスの継承ツリーを出力する
  class Editor_Frame : public Frame {
    /// GUIパーツ
    Widget buf, status;

    /// 物理行移動のためのダイアログ
    Widget goto_line_dialog;

    /// 時刻表示のタイマー
    XtIntervalId timer;
    /// 時刻表示のインターバル
    long interval; // ms

    XtIntervalId position_timer;
    long position_interval; // ms

    XawTextPositionInfo save_pos;

    static void show_time_proc(XtPointer closure, XtIntervalId *id);
    static void fetch_text_proc(Widget widget, XtPointer client_data, XtPointer call_data);
    static void select_all_text_proc(Widget widget, XtPointer client_data, XtPointer call_data);
    static void delete_selection_text_proc(Widget widget, XtPointer client_data, XtPointer call_data);
    static void append_hello_text_proc(Widget widget, XtPointer client_data, XtPointer call_data);
    static void goto_line_text_proc(Widget widget, XtPointer client_data, XtPointer call_data);
    static void open_goto_list_dialog_proc(Widget widget, XtPointer client_data, XtPointer call_data);
    static void show_potision_proc(XtPointer closure, XtIntervalId *id);
    static void position_callback(Widget widget, XtPointer client_data, XtPointer call_data);
    static Boolean text_ready(XtPointer closure);

  public:
    Editor_Frame() : goto_line_dialog(None),timer(0),interval(1000),
		 position_timer(0), position_interval(600) { }

    Widget create_contents(Widget shell);
  };

  class Editor_Frame_Factory : public Frame_Factory {
    Frame *get_instance() { return new Editor_Frame(); }
  };

  /// 地域時間を入手する
  static void fetch_localtime(struct tm *local) {
    time_t now;
    if (time(&now) == (time_t)-1) { perror("time"); return; }
    if (!localtime_r(&now, local)) {
      cerr << "unkown localtime." << endl;
    }
  }

  /// 現在時刻の表示
  void Editor_Frame::show_time_proc(XtPointer closure, XtIntervalId *id) {
    Editor_Frame *app = (Editor_Frame *)closure;

    struct tm local;
    fetch_localtime(&local);

    char buf[200];
    strftime(buf, sizeof buf, "%Y, %B, %d, %A %p%I:%M:%S", &local);
    strftime(buf, sizeof buf, "%p %I:%M", &local);
    XtVaSetValues(app->status, XtNstring, XtNewString(buf), NULL);

    // 再開するために再登録
    app->timer =
      XtAppAddTimeOut(XtWidgetToApplicationContext(app->status),
		      app->interval, show_time_proc, app);
  }

  /// 選択された領域のテキストを入手する
  static wstring fetch_selected_text(Widget textw) {
    XawTextPosition begin_pos, end_pos;

    XawTextGetSelectionPos(textw, &begin_pos, &end_pos);
    if (begin_pos == end_pos) return L"";

    XawTextBlock text;
    XawTextPosition pos = XawTextSourceRead(XawTextGetSource(textw), begin_pos, &text, end_pos - begin_pos);

#if 1
    cerr << "begin:" << begin_pos << endl;
    cerr << "end:" << end_pos << endl << endl;
    cerr << "pos:" << pos << endl << endl;
    cerr << "first:" << text.firstPos << endl;
    cerr << "length:" << text.length << endl;
    cerr << "format:" << text.format << endl;
#endif

    if (text.format == XawFmtWide) {
      // ワイド文字で返って来るようだ
#if 0
      cerr << "format: wide" << endl;
#endif
      wstring wstr(((wchar_t *)text.ptr), text.length);
      return wstr;
    }
    return L"";
  }

  /// メニューでテキスト抽出の指令を受け取った時の処理
  void Editor_Frame::fetch_text_proc( Widget widget, XtPointer client_data, XtPointer call_data) {
    Editor_Frame *app = (Editor_Frame *)client_data;

    cout << XtName(app->buf) << " selected" << endl;

    XawTextPosition begin_pos, end_pos;
    XawTextGetSelectionPos(app->buf, &begin_pos, &end_pos);
    if (begin_pos == end_pos) {
      String text;
      XtVaGetValues(app->buf, XtNstring, &text, NULL );
      cout << text << endl << flush;
      return;
    }
#if 1
    wstring wstr = fetch_selected_text(app->buf);
    printf("%ls\n", wstr.c_str());
    fflush(stdout);
#else
    XawTextBlock text;
    XawTextPosition pos = XawTextSourceRead(XawTextGetSource(app->buf), begin_pos, &text, end_pos - begin_pos);

    cerr << "begin:" << begin_pos << endl;
    cerr << "end:" << end_pos << endl << endl;
    cerr << "pos:" << pos << endl << endl;

    cerr << "first:" << text.firstPos << endl;
    cerr << "length:" << text.length << endl;
    cerr << "format:" << text.format << endl;

    if (text.format == XawFmtWide) {
      cerr << "format: wide" << endl;
      wstring wstr(((wchar_t *)text.ptr), text.length);
      printf("%ls\n", wstr.c_str());
    }
    else {
      string mbstr(text.ptr + text.firstPos, text.length);
      printf("%s\n", mbstr.c_str());
    }
#endif
  }

  /// すべてのテキストを選択(セレクションには入れない)
  void Editor_Frame::select_all_text_proc( Widget widget, XtPointer client_data, XtPointer call_data) {
    Editor_Frame *app = (Editor_Frame *)client_data;
    XawTextPosition last = XawTextLastPosition(app->buf);
    XawTextSetSelection(app->buf,0,last);
  }

  /// 選択テキストを置き換える
  static void replace_text(Widget text, wchar_t *buf, size_t len) {
    XawTextPosition begin_pos, end_pos;
    XawTextGetSelectionPos(text, &begin_pos, &end_pos);
    XawTextBlock block;
    block.firstPos = 0;
    block.length = len;
    block.ptr = (char *)buf;
    block.format = XawFmtWide;
    int rc = XawTextReplace(text, begin_pos, end_pos, &block);
    cerr << "TRACE: aw replace selection: " << rc << endl;
  }

  /// 選択テキストを削除する
  void Editor_Frame::delete_selection_text_proc( Widget widget, XtPointer client_data, XtPointer call_data) {
    Editor_Frame *app = (Editor_Frame *)client_data;
    replace_text(app->buf, L"", 0) ;
    cerr << "TRACE: aw delete selection: " << endl;
  }

  /// テキストを末尾に追加する
  static void append_text(Widget text, const wchar_t *buf, size_t len) {
    XawTextPosition last = XawTextLastPosition(text);
    XawTextBlock block;
    block.firstPos = 0;
    block.length = len;
    block.ptr = (char *)buf;
    block.format = XawFmtWide;
    int rc = XawTextReplace(text, last, last, &block);
    cerr << "TRACE: aw append: " << rc << endl;
    XawTextSetInsertionPoint(text, last + len);
  }

  /// テキストをカレット位置に追加する
  static void insert_text(Widget text, wchar_t *buf, size_t len) {
    XawTextPosition pos = XawTextGetInsertionPoint(text);
    XawTextBlock block;
    block.firstPos = 0;
    block.length = len;
    block.ptr = (char *)buf;
    block.format = XawFmtWide;
    int rc = XawTextReplace(text, pos, pos, &block);
    cerr << "TRACE: aw insert: " << rc << endl;
    XawTextSetInsertionPoint(text, pos + len);
  }

  ///　現在時刻をカレット位置に挿入する
  static void insert_datetime_text_proc( Widget widget, XtPointer client_data, XtPointer call_data) {
    Widget text = (Widget)client_data;

    struct tm local;
    fetch_localtime(&local);

    char buf[200];
    strftime(buf, sizeof buf, "%Y, %B, %d, %A %p%I:%M:%S", &local);

    size_t len = strlen(buf);
    wchar_t wcs[len + 1];
    len = mbstowcs(wcs, buf, len);
    if (len != (size_t)-1) insert_text(text, wcs, len);
  }

  void Editor_Frame::append_hello_text_proc( Widget widget, XtPointer client_data, XtPointer call_data) {
    Editor_Frame *app = (Editor_Frame *)client_data;
    wchar_t buf[] = L"Hello,world\n";
    append_text(app->buf, buf, wcslen(buf));
  }

  /// ダイアログテキストを初期化する
  static void clear_dialog_text_proc( Widget widget, XtPointer client_data, XtPointer call_data) {
    Widget dialog = (Widget )client_data;
    XtVaSetValues(dialog, XtNvalue, XtNewString(""), NULL);
  }

  /// テキストの物理行移動
  void Editor_Frame::goto_line_text_proc( Widget widget, XtPointer client_data, XtPointer call_data) {
    Editor_Frame *app = (Editor_Frame *)client_data;
    const char *value = XawDialogGetValueString(app->goto_line_dialog);

    int lineno = atoi(value);
    cerr << "TRACE: #goto_line_text_proc called:" << lineno << endl;

    // TODO: 移動する方法を探して実装する
  }

  /// テキストの物理行移動のダイアログ表示
  void Editor_Frame::open_goto_list_dialog_proc( Widget widget, XtPointer client_data, XtPointer call_data) {
    Editor_Frame *app = (Editor_Frame *)client_data;

    if (!app->goto_line_dialog) {
      Widget shell =
	XtVaCreatePopupShell("goto-line", transientShellWidgetClass, find_shell(widget, 0), NULL );

      Widget dialog = app->goto_line_dialog =
	XtVaCreateManagedWidget("dialog", dialogWidgetClass, shell,
				XtNvalue, XtNewString(""), NULL );

      /* ウィジット関数によってダイアログ内部のボタンをセットする */
      XawDialogAddButton(dialog, "jump", goto_line_text_proc, app );
      XawDialogAddButton(dialog, "clear", clear_dialog_text_proc, dialog );
      XawDialogAddButton(dialog, "cancel", close_dialog, shell );

      Widget value = XtNameToWidget(dialog, "value");

      install_accelerators(dialog, "jump");
      install_accelerators(dialog, "cancel");

      install_accelerators(value, "jump");
      install_accelerators(value, "cancel");

      XtVaSetValues(dialog, XtNlabel, XtNewString("line number:"), NULL);
    }

    XtVaSetValues(app->goto_line_dialog, XtNvalue, XtNewString(""), NULL);

    // モーダルダイアログ（ただし他のウィンドウとは非排他的)
    popup_dialog(widget, find_shell(app->goto_line_dialog,0), XtGrabNonexclusive);
  }

  /// 現在カーソル位置の表示
  void Editor_Frame::show_potision_proc(XtPointer closure, XtIntervalId *id) {
    Editor_Frame *app = (Editor_Frame *)closure;
    app->position_timer = 0;
    cerr << "Line: " << app->save_pos.line_number <<
      " Column: " << (app->save_pos.column_number + 1) << "     \r";
  }

  /// テキスト位置が変化したタイミングで呼び出される
  void Editor_Frame::position_callback(Widget widget, XtPointer client_data, XtPointer call_data) {
    XawTextPositionInfo *pos = (XawTextPositionInfo *)call_data;
    Editor_Frame *app = (Editor_Frame *)client_data;

    if (0)
      cerr << XtName(widget) << " position callback" << endl;

    app->save_pos = *pos;
    if (app->position_timer)
      XtRemoveTimeOut(app->position_timer);

    app->position_timer =
      XtAppAddTimeOut(XtWidgetToApplicationContext(app->status),
		      app->position_interval, show_potision_proc, app);
  }

  /// 処理するイベントがなくなった時に呼び出される
  Boolean Editor_Frame::text_ready(XtPointer closure) {
    Editor_Frame *app = (Editor_Frame *)closure;

    cerr << "TRACE: work proc called" << endl;

    wstring wtext = xwin::load_wtext(__FILE__);

    append_text(app->buf, wtext.c_str(), wtext.size());
    XawTextSetInsertionPoint(app->buf, 0);

    // 時刻表示のタイマーを登録
    app->timer =
      XtAppAddTimeOut(XtWidgetToApplicationContext(app->status),
		      app->interval, show_time_proc, app);
    return True;
  }

  Widget Editor_Frame::create_contents(Widget top) {
    Widget pane =
      XtVaCreateManagedWidget("pane", panedWidgetClass, top, NULL );
    Widget menu_bar =
      XtVaCreateManagedWidget("menu_bar", boxWidgetClass, pane, NULL);

    XtVaCreateManagedWidget("memo", menuButtonWidgetClass, menu_bar,
			    XtNmenuName, "memo-menu",
			    NULL);
    buf =
      XtVaCreateManagedWidget("textbuf", asciiTextWidgetClass, pane,
			      XtNeditType, XawtextEdit,
			      XtNtype, XawAsciiString,
			      XtNwrap, XawtextWrapWord,
			      XtNscrollVertical, XawtextScrollWhenNeeded, // or XawtextScrollAlways,
			      NULL);
    XtAddCallback(buf, XtNpositionCallback, position_callback, this);

    Widget close =
      XtVaCreateManagedWidget("close", commandWidgetClass, menu_bar, NULL);
    XtAddCallback(close, XtNcallback, quit_application, 0);
    XtInstallAccelerators(buf, close);

    struct Menu_Item submenu[] = {
      { "item1", menu_selected, },
      { "item2", menu_selected, },
      { "item3", menu_selected, },
      { 0, },
    };

    //create_popup_menu("memo-submenu", top, submenu);

    struct Menu_Item items[] = {
      { "select-all", select_all_text_proc, this, },
      { "delete-selection", delete_selection_text_proc, this, },
      { "hello", append_hello_text_proc, this, },
      { "fetch-text", fetch_text_proc, this, },
      { "now", insert_datetime_text_proc, buf, },
      { "goto-line", open_goto_list_dialog_proc, this, },
      { "sub-menu", 0, 0, submenu },
      { "-", },
      { "close", quit_application, },
      { 0, },
    };

    create_popup_menu("memo-menu", top, items);

    struct Menu_Item shortcut_item[] = {
      { "aaa", menu_selected, },
      { "bbb", menu_selected, },
      { "fetch-text", fetch_text_proc, this, },
      { "sub-menu", 0, 0, submenu },
      { "-", },
      { "close", quit_application, },
      { 0, },
    };

    create_popup_menu("text-shortcut", top, shortcut_item);

#if 1
    status =
      XtVaCreateManagedWidget("status", asciiTextWidgetClass, pane,
			      XtNscrollHorizontal, XawtextScrollNever,
			      XtNdisplayCaret, False,
			      XtNskipAdjust, True, NULL);
#endif

    //    XtAddEventHandler(top, NoEventMask, True, _XEditResCheckMessages, NULL);

    XtAppContext context = XtWidgetToApplicationContext(top);

    XtWorkProcId work = XtAppAddWorkProc(context, text_ready, this);
    cerr << "TRACE: ready proc: " << this << ":" << work << endl;
    cerr << "TRACE: now locale C_TYPE: " << setlocale (LC_CTYPE, 0) << endl;

    XtSetKeyboardFocus(pane, buf);

    return 0;
  }


  // --------------------------------------------------------------------------------

  /// Athena Widget Set を使うアプリケーションコンテナの利用開始

  static int awt_app01(int argc, char **argv) {

    struct { char *name; Frame_Factory *ff; } fflist[] = {
      { "top", new Simple_Frame_Factory(), },
      { "message", new Message_Frame_Factory(), },
      { "button", new Button_Frame_Factory(), },
      { "dialog", new Dialog_Button_Frame_Factory(), },
      { "ctree", new Class_Tree_Factory(), },
      { "folder", new Folder_List_Factory(), },
      { "memo", new Editor_Frame_Factory(), },
      { "event", new Event_View_Factory(), },
    };

    Frame_Factory *ff = 0;
    const char *app_class = 0;

    vector<Frame_Factory *>::iterator it = frame_factories.begin();
    for (; it != frame_factories.end(); it++) {
      ff = *it;

      const char *name = ff->get_class_name();
      if (!name) {
	name = demangle(typeid(*ff).name());
	const char *p;
	while ((p = strstr(name,"::"))) { name = p + 2; }
      }

      /// 取りあえずファクトリ名とバージョンを表示してみる
      printf("factory: %s: %s\n", name, ff->get_version());
    }

    elog(I, "%d factories declared.\n", frame_factories.size());

    if (!ff) return 1;

    for (int i = 0, n = XtNumber(fflist); i < n; i++) {
      if (strcasecmp(fflist[i].name, argv[0]) == 0) ff = fflist[i].ff;
    }

    // ここまでで、利用する Factoryが決まる。

    static String fallback_resouces[] = {
      "Simple_Frame_Factory.geometry: 300x200",
      "Message_Frame_Factory.geometry: 300x200",
      "Dialog_Button_Frame_Factory.geometry: 300x200",
      "Button_Frame_Factory.geometry: 300x200",
      "Class_Tree_Factory.geometry: 800x400",

      "*font: -adobe-helvetica-bold-r-*-*-34-*-*-*-*-*-*-*",
      "*.close.accelerators: #override "
      " Ctrl<KeyPress>w: set() notify() unset()\\n",

      "*.press.label: Press Me!",
      "*.press.accelerators: Meta<KeyPress>p: set() notify() unset()\\n"
      " <KeyPress>space: set() notify() unset()",

      "*.dialog.label: Do You want to quit ?",
      "*.yes.accelerators: <KeyPress>y: set() notify() unset() \\n"
      " <KeyPress>space: set() notify() unset()",

      "*.no.accelerators: <KeyPress>n: set() notify() unset()\\n"
      " <KeyPress>Escape: set() notify() unset()",

      "*tree.gravity: west",

      "*international: True", // 国際化を有効に
      "*fontSet: -*-*-*-*-*--24-*", // フォントの指定
      "*font: -adobe-helvetica-bold-r-*-*-34-*-*-*-*-*-*-*",
      "*List.font: -adobe-helvetica-bold-r-*-*-34-*-*-*-*-*-*-*",

      "Folder_List_Factory.geometry: 800x400",

      "*Viewport.vertical.accelerators: #override "
      " Ctrl<KeyPress>Next: StartScroll(Forward) NotifyScroll(Proportional) EndScroll()\\n"
      " Ctrl<KeyPress>Prior: StartScroll(Backward) NotifyScroll(Proportional) EndScroll()\\n"
      " <Btn4Down>: StartScroll(Backward)\\n"
      " <Btn5Down>: StartScroll(Forward)\\n"
      " <BtnUp>:NotifyScroll(Proportional) EndScroll()\n",

      "*Viewport.vertical.translations: #override "
      " <Btn4Down>: StartScroll(Backward)\\n"
      " <Btn5Down>: StartScroll(Forward)\\n",

      // キー割り当ての確認
      "*event02.translations: #override "
      " Ctrl<Key>1: XtDisplayTranslations()\\n"
      " Ctrl<Key>2: XtDisplayInstalledAccelerators()\\n"
      " <Key>Up: report(key-Up)\\n"
      " <Key>Down: report(key-Down)\\n"
      " <Key>a: report(key-a)\\n"
      " Ctrl<Key>b: report(key-C-b)\\n"
      " Ctrl<Key>Return: report(key-C-Return)\\n"
      " <Key>Return: report(key-Return)\\n"
      " <Key>Home: report(key-Home)\\n"
      " <Key>End: report(key-End)\\n"
      " Ctrl<Key>Home: report(key-C-Home)\\n"
      " Ctrl<Key>End: report(key-C-End)\\n"
      " <Key>: report(fallback)\\n"
      ,

      "*fslist.translations: #override"
      " Ctrl<Key>1: XtDisplayTranslations()\\n"
      " Ctrl<Key>2: XtDisplayInstalledAccelerators()\\n"
      " Ctrl<Key>BackSpace: report(C-backspace) change-item(previous)\\n"
      " None<KeyPress>BackSpace: change-item(previous) report(backspace)\\n"
      " None<KeyPress>Right: change-item(next)\\n"
      " None<KeyPress>Left: change-item(previous)\\n"
      " <KeyPress>Up: change-item(above)\\n"
      " <KeyPress>Down: change-item(below)\\n"
      " <Btn1Down>: Set() Notify() \\n"
      " Ctrl <Btn3Down>: XawPositionSimpleMenu(list-menu) XtMenuPopup(list-menu)\\n"
      " Ctrl<Key>BackSpace: report(C-BackSpace)\\n"
      " :Ctrl<Key>BackSpace: report(:C-BackSpace)\\n"
      " Ctrl<Key>f: change-item(next)\\n"
      " Ctrl<Key>b: change-item(previous)\\n"
      " Ctrl<Key>p: change-item(above)\\n"
      " Ctrl<Key>n: change-item(below)\\n"
      " <Key>BackSpace: report(BackSpace)\\n"
      " <Key>: report(key-fallback-report)\\n"
      ,

      "*inputMethod: kinput2",
      "*font: -adobe-helvetica-bold-r-*-*-34-*-*-*-*-*-*-*",
      "*preeditType: OverTheSpot,OffTheSpot,Root",
      "*cursorColor: red",

      "*textbuf.width: 500",
      "*textbuf.height: 300",
      "*textbuf.textSource.enableUndo: True",
      "*sub-menu.menuName: memo-submenu",
#if 0
      "*Text.translations: #override "
      " <Key>Right: forward-character()\\n"
      " <Key>Left: backward-character()\\n"
      " <Key>Down: next-line()\\n"
      " <Key>Up: previous-line()\\n"
      " <Key>KP_Down: next-line()\\n"
      " <Key>KP_Up: previous-line()\\n"
      " <Key>KP_Right: forward-character()\\n"
      " <Key>KP_Left: backward-character()\\n"
      " <Key>Home: beginning-of-file()\\n"
      " <Key>End: end-of-file()\\n"
      " <Key>Next: next-page()\\n"
      " <Key>Prior: previous-page()\\n"
      "Shift<Key>Insert: insert-selection(PRIMARY, CUT_BUFFER0)\\n"
      "Ctrl<Key>Right: forward-word()\\n"
      "Ctrl<Key>Left: backward-word()\\n"
      "Ctrl<Key>space: select-start()\\n"
      "Shift<Key>Right: forward-character()select-adjust()extend-end(PRIMARY, CUT_BUFFER0)\\n"
      "<Key>Home: beginning-of-file()\\n"
      "<Key>End: end-of-file()\\n"
      "<Key>Next: next-page()\\n"
      "<Key>Prior: previous-page()\\n",
#endif

      "*textbuf.translations: #override "
      " Ctrl<Key>1: XtDisplayTranslations()\\n"
      " Ctrl<Key>2: XtDisplayInstalledAccelerators()\\n"
      " Ctrl<Key>F2: XawPositionSimpleMenu(text-shortcut) XtMenuPopup(text-shortcut)\\n"
#if 0
      " Ctrl<Key>space: select-start()\\n"
      " Shift<Key>Right: forward-character()select-adjust()extend-end(PRIMARY, CUT_BUFFER0)\\n"
      " <Key>BackSpace: delete-previous-character()\\n"
      " <Key>Right: forward-character()\\n"
      " <Key>Left: backward-character()\\n"
      " Shift<Key>Insert: insert-selection(PRIMARY, CUT_BUFFER0)\\n"
      " Ctrl<Key>Right: forward-word()\\n"
      " Ctrl<Key>Left: backward-word()\\n"
      " <Key>: insert-char() report(text-fallback)\\n"
#endif
      " Shift<Btn4Down>,<Btn4Up>: scroll-one-line-down()\\n"
      " Shift<Btn5Down>,<Btn5Up>: scroll-one-line-up()\\n"
      " Ctrl<Btn4Down>,<Btn4Up>: previous-page()\\n"
      " Ctrl<Btn5Down>,<Btn5Up>: next-page()\\n"
      " <Btn4Down>,<Btn4Up>:scroll-one-line-down()scroll-one-line-down()scroll-one-line-down()"
        "scroll-one-line-down()scroll-one-line-down()\\n"
      " <Btn5Down>,<Btn5Up>:scroll-one-line-up()scroll-one-line-up()scroll-one-line-up()"
        "scroll-one-line-up()scroll-one-line-up()\\n"
      " Ctrl<Btn1Down>: XawPositionSimpleMenu(text-shortcut) XtMenuPopup(text-shortcut)\\n"
      " Ctrl<Btn3Down>: XawPositionSimpleMenu(text-shortcut) XtMenuPopup(text-shortcut)\\n"
      ,

#if 0
      "*menu_bar.translations: #override "
      " <Btn3Down>: XawPositionSimpleMenu(text-shortcut) MenuPopup(text-shortcut)\\n"
      " !Ctrl <Btn3Down>: XawPositionSimpleMenu(text-shortcut) MenuPopup(text-shortcut)\\n"
      " !Lock Ctrl <Btn3Down>: popup-shortcut(text-shortcut)\\n"
#else
      "*menu_bar.translations: #override "
      " <Btn3Down>: popup-shortcut(text-shortcut)\\n"
      " Ctrl <Btn1Down>: XawPositionSimpleMenu(text-shortcut) XtMenuPopup(text-shortcut)\\n"
      " Ctrl <Btn3Down>: popup-shortcut(text-shortcut)\\n"
      " !Ctrl <Btn3Down>: popup-shortcut(text-shortcut)\\n"
      " !Lock Ctrl <Btn3Down>: popup-shortcut(text-shortcut)\\n"
#endif
      ,
      "*.jump.accelerators: <KeyPress>Return: set() notify() unset()\\n",
      "*.cancel.accelerators: <KeyPress>Escape: set() notify() unset()\\n",
      "*.close.accelerators: #override Ctrl<KeyPress>w: set() notify() unset()\\n",
      0,
    };
    /*
      fallbackのアドレスは、スタック上に作成してはいけないため、
      グローバル変数とするか、static　修飾を指定すること。
    */

    XtAppContext context;
    Widget top_level_shell;

    XtSetLanguageProc(NULL, NULL, NULL);

    // アプリケーション追加オプションの様式定義
    static XrmOptionDescRec options[] = {
      { "-msg", ".Message", XrmoptionSepArg, NULL, },
    };

    static XtActionsRec actions[] = {
      { "change-item", change_item_action, },
      { "popup-shortcut", popup_action, },
      { "report", report_action, },
    };

    {
      const char *cname = ff->get_class_name();
      if (!cname) {
	cname = demangle(typeid(*ff).name());
	const char *p;
	while ((p = strstr(cname,"::"))) { cname = p + 2; }
      }
      app_class = strdup(cname);
    }

    top_level_shell =
      XtVaAppInitialize(&context, app_class, options, XtNumber(options),
			&argc, argv, fallback_resouces, NULL);

    XawInitializeWidgetSet();

    XawSimpleMenuAddGlobalActions(context);

    XtRegisterGrabAction(popup_action, True,
			 ButtonPressMask | ButtonReleaseMask ,
			 GrabModeAsync, GrabModeAsync);

    XtAppAddActions(context, actions, XtNumber(actions));

    Frame *fr = ff->get_instance();
    printf("frame: %p: %s\n", fr, demangle(typeid(*fr).name()));

    Server_Resource_Impl *sr = new Server_Resource_Impl(top_level_shell);
    sr->open_frame(fr) ;

    XtAppMainLoop(context);
    XtDestroyApplicationContext(context);
    cerr << "TRACE: context destroyed." << endl;

    {
      vector<Frame_Factory *>::iterator it = frame_factories.begin();
      for (; it != frame_factories.end();it++) { delete *it; }
    }
    return 0;
  }

  // --------------------------------------------------------------------------------
  
  static Atom WM_PROTOCOLS, WM_DELETE_WINDOW, COMPOUND_TEXT, MULTIPLE;

  /// アトムを入手する
  static void xatom_initialize(Display *display) {
    struct {
      Atom *atom_ptr;
      const char *name;
    } ptr_list[] = {
      { &WM_PROTOCOLS, "WM_PROTOCOLS", },
      { &WM_DELETE_WINDOW, "WM_DELETE_WINDOW", },
      { &COMPOUND_TEXT, "COMPOUND_TEXT", },
      { &MULTIPLE, "MULTIPLE", },
    }, *i;
    
    for (i = ptr_list; i < ptr_list + XtNumber(ptr_list); i++)
      *i->atom_ptr = XInternAtom(display, i->name, True );
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

namespace {

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

extern "C" {
#include "xev.c"
};

// --------------------------------------------------------------------------------

#include "subcmd.h"

subcmd awt_cmap[] = {
  { "top", awt_app01, },
  { "multi", awt_multi_apps, },
  { "message", awt_app01, },
  { "dialog", awt_app01, },
  { "dialog02", awt_dialog02, },
  { "ctree", awt_app01, },
  { "folder", awt_app01, },
  { "memo", awt_app01, },
  { "awt", awt_app01, },
  { "event", awt_app01, },
  { 0 },
};

