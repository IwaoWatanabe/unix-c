/*! \file
 * \brief Athena Widget Setを使ったサンプルコード

  Xlibがサポートするロケールと libcがサポートするロケールが合致していないと
  エディタはうまく動作しない点に注意すること。
 */

#include "uc/elog.hpp"
#include <string>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <clocale>
#include <iostream>
#include <vector>
#include <map>
#include <time.h>

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
#include "SmeCascade.h"

using namespace std;

enum { FATAL, ERROR, WARN, NOTICE, INFO, AUDIT, DEBUG, TRACE };

extern int getXawListColumns(Widget list);
extern int getXawListRows(Widget list);

namespace xwin {
  /// ディレクトリ・エントリを入手する
  bool load_dirent(const char *dirpath, vector<string> &entries, bool with_hidden_file = false);
  String *as_String_array(vector<string> &entries);
  wstring load_wtext(const char *path, size_t buf_len = 4096);
}


extern "C" {
  extern char *demangle(const char *demangle);

  /// 地域時間を入手する
  static void fetch_localtime(struct tm *local) {
    time_t now;
    if (time(&now) == (time_t)-1) { perror("time"); return; }
    if (!localtime_r(&now, local)) {
      cerr << "unkown localtime." << endl;
    }
  }

  /// メッセージを表示するだけのウィンドウ
  static int msg_window(int argc, char **argv) {

    XtSetLanguageProc(NULL, NULL, NULL);

    static String app_class = "Message", 
      fallback_resouces[] = { 
      "Message.geometry: 300x200",
      "*font: -adobe-helvetica-bold-r-*-*-34-*-*-*-*-*-*-*",
      NULL,
    };

    static XrmOptionDescRec options[] = { };

    XtAppContext context;
    Widget top =
      XtVaOpenApplication(&context, app_class, options, XtNumber(options),
			&argc, argv, fallback_resouces, applicationShellWidgetClass, NULL);

    struct message_res { Boolean help; String message; };

    static XtResource resources[] = {
      { "message", "Message", XtRString, sizeof(String),
	XtOffset(message_res *, message), XtRImmediate, (XtPointer)"Hello,world" },
    };

    Widget shell = top;

    message_res res;
    XtVaGetApplicationResources(shell, &res, resources, XtNumber(resources), NULL);

    Widget label =
      XtVaCreateManagedWidget("message", labelWidgetClass, shell,
			      XtNjustify, XtJustifyCenter,
			      XtNlabel, XtNewString(res.message),
			      NULL);
    XtRealizeWidget(top);
    XtAppMainLoop(context);
    XtDestroyApplicationContext(context);

    elog(TRACE,"context destroyed.");
    return 0;
  }
};



namespace awt {

  class AppContainer;

  struct Simple_Menu_Item {
    /// メニュー名
    String name;
    struct Simple_Menu_Item *sub_menu;
  };

  class Apps {
  protected:
    AppContainer *ac;
    Apps();
    virtual void disposeShell(Widget wi);

  public:
    static void item_selected(Widget widget, XtPointer client_data, XtPointer call_data);
    static void open_dialog(Widget widget, XtPointer client_data, XtPointer call_data);
    static void panner_callback (Widget panner, XtPointer closure, XtPointer data);
    static void porthole_callback(Widget porthole, XtPointer closure, XtPointer data);
    static void viewport_callback(Widget porthole, XtPointer closure, XtPointer data);

    virtual ~Apps() { }
    virtual void setContainer(AppContainer *container) { ac = container; }
    virtual void command_proc(string &cmd, Widget widget);
    virtual void popup_notify(Widget widget) { }
    virtual void popdown_notify(Widget widget) { }

    virtual String getAppClassName() = 0;
    virtual void createWidgets(Widget shell) = 0;
    virtual String *loadFallbackResouces();
    virtual XrmOptionDescRec *getAppOptions(Cardinal &n_options);
    virtual void loadResouces(Widget shell) { }
  };


  class _ServerResouece {
  protected:
    Atom WM_PROTOCOLS, WM_DELETE_WINDOW, COMPOUND_TEXT, MULTIPLE;

    static void dispose_handler(Widget widget, XtPointer closure,
				XEvent *event, Boolean *continue_to_dispatch);
  public:
    void atom_initialize(Display *display);
    void register_wm_hander(Widget shell);
  };

  class AppContainer {
  protected:
    _ServerResouece *sr;

    static bool param_contains(String name, String *params, Cardinal num);
    static void change_item_action(Widget list, XEvent *event, String *params, Cardinal *num);
    static Widget find_menu(Widget widget, String name);
    static void positionMenu(Widget w, XPoint *loc);
    static void popup_action(Widget w, XEvent *event, String *params, Cardinal *num);
    static void menu_popup_proc( Widget widget, XtPointer client_data, XtPointer call_data);
    static void menu_popdown_proc( Widget widget, XtPointer client_data, XtPointer call_data);

    static void close_dialog(Widget widget, XtPointer client_data, XtPointer call_data);
    static void install_accelerators(Widget parent, const char *name);
    static void clear_dialog_text_proc( Widget widget, XtPointer client_data, XtPointer call_data);

  public:
    XtAppContext app_context;
    static void popup_dialog(Widget target, Widget shell, XtGrabKind kind);
    AppContainer();
    virtual Widget create_popup_menu(String menu_name, Apps *app, Widget owner, Simple_Menu_Item *items);
    virtual Widget message_dialog(String prompt, String apply_name, Apps *app, Widget owner);
    virtual Widget input_dialog(String prompt, String defaultValue, String apply_name, Apps *app, Widget owner);

    virtual ~AppContainer();
    virtual int run(int &argc, char **argv, Apps *app);
  };

  Apps::Apps():ac(0) { }

  static String _fallback_resouces[] = {
    "Message.geometry: 300x200",
    "Confirm.geometry: 300x200",
    "FolderList.geometry: 800x400",

    "*font: -adobe-helvetica-bold-r-*-*-34-*-*-*-*-*-*-*",
    "*.close.accelerators: #override Ctrl<KeyPress>w: set() notify() unset()\\n",

    "*international: True",
    "*fontSet: -*-*-*-*-*--24-*",
    "*.ok.accelerators: <KeyPress>Return: set() notify() unset()\\n"
    " <KeyPress>space: set() notify() unset()",

    "*.press.label: Press Me!",
    "*.press.accelerators: Meta<KeyPress>p: set() notify() unset()\\n"
    " <KeyPress>space: set() notify() unset()",

    "*.dialog.label: Do You want to quit ?",
    "*.yes.accelerators: <KeyPress>y: set() notify() unset() \\n"
    " <KeyPress>space: set() notify() unset()",

    "*.no.accelerators: <KeyPress>n: set() notify() unset()\\n"
    " <KeyPress>Escape: set() notify() unset()",


      "*Viewport.vertical.accelerators: #override "
      " Ctrl<KeyPress>Next: StartScroll(Forward) NotifyScroll(Proportional) EndScroll()\\n"
      " Ctrl<KeyPress>Prior: StartScroll(Backward) NotifyScroll(Proportional) EndScroll()\\n"
      " <Btn4Down>: StartScroll(Backward)\\n"
      " <Btn5Down>: StartScroll(Forward)\\n"
      " <BtnUp>:NotifyScroll(Proportional) EndScroll()\n",

      "*Viewport.vertical.translations: #override "
      " <Btn4Down>: StartScroll(Backward)\\n"
      " <Btn5Down>: StartScroll(Forward)\\n",

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
      "*preeditType: OverTheSpot,OffTheSpot,Root",
      "*cursorColor: red",

      "*textbuf.width: 500",
      "*textbuf.height: 300",
      "*textbuf.textSource.enableUndo: True",
      "*sub-menu.menuName: memo-submenu",

      "*textbuf.translations: #override "
      " Ctrl<Key>1: XtDisplayTranslations()\\n"
      " Ctrl<Key>2: XtDisplayInstalledAccelerators()\\n"
      " Ctrl<Key>F2: XawPositionSimpleMenu(text-shortcut) XtMenuPopup(text-shortcut)\\n"

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

      "*menu_bar.translations: #override "
      " <Btn3Down>: popup-shortcut(text-shortcut)\\n"
      " Ctrl <Btn1Down>: XawPositionSimpleMenu(text-shortcut) XtMenuPopup(text-shortcut)\\n"
      " Ctrl <Btn3Down>: popup-shortcut(text-shortcut)\\n"
      " !Ctrl <Btn3Down>: popup-shortcut(text-shortcut)\\n"
      " !Lock Ctrl <Btn3Down>: popup-shortcut(text-shortcut)\\n"
    ,

      "*.jump.accelerators: <KeyPress>Return: set() notify() unset()\\n",
      "*.cancel.accelerators: <KeyPress>Escape: set() notify() unset()\\n",
      "*.close.accelerators: #override Ctrl<KeyPress>w: set() notify() unset()\\n",
      0,

  };

  /*
    fallbackのアドレスは、スタック上に作成してはいけないため、
    グローバル変数とするか、static　修飾を指定する。
  */

  String *Apps::loadFallbackResouces() {
    return _fallback_resouces;
  }

  XrmOptionDescRec *Apps::getAppOptions(Cardinal &n_options) {
    static XrmOptionDescRec options[] = { };
    n_options = XtNumber(options);
    return options;
  }

  /// トップレベル・シェルを破棄する
  void Apps::disposeShell(Widget wi) {
    dispose_shell(wi);
  }

  void Apps::command_proc(string &cmd, Widget widget) {
    if (cmd == "close")
      disposeShell(widget);
  }

  void Apps::item_selected(Widget widget, XtPointer client_data, XtPointer call_data) {
    string cmd = XtName(widget);
    ((Apps *)client_data)->command_proc(cmd, widget);
  }

  /// pannaer が操作されたタイミングで portholeのウィジェットの位置を変えるためのコールバック
  void Apps::panner_callback (Widget panner,XtPointer closure,XtPointer data) {
    XawPannerReport *rep = (XawPannerReport *)data;
    Widget target = (Widget)closure;
    Arg args[2];
    if (!target) return;
    XtSetArg (args[0], XtNx, -rep->slider_x);
    XtSetArg (args[1], XtNy, -rep->slider_y);
    XtSetValues (target, args, 2);
  }

  /// portholeの大きさが変わったときに、pannerの大きさを調整するためのコールバック
  void Apps::porthole_callback(Widget porthole, XtPointer closure, XtPointer data) {
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

  /// portholeの大きさが変わったときに、pannerの大きさを調整するためのコールバック
  void Apps::viewport_callback(Widget porthole, XtPointer closure, XtPointer data) {
    XawPannerReport *rep = (XawPannerReport *)data;

    cerr << "TRACE: y:" << rep->slider_y << " x:" << rep->slider_x
	 << " slider width:" << rep->slider_width << " height:" << rep->slider_height
	 << " canvas width:" << rep->canvas_width << " height:" << rep->canvas_height
	 << " changed:" << rep->changed << endl;
  }

  /// -------------- ここからコンテナのコード


  void _ServerResouece::atom_initialize(Display *display) {
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
  void _ServerResouece::dispose_handler(Widget widget, XtPointer closure,
					XEvent *event, Boolean *continue_to_dispatch) {
    switch(event->type) {
    default: return;
    case ClientMessage:
      _ServerResouece *sr = (_ServerResouece *)closure;
      if (event->xclient.message_type == sr->WM_PROTOCOLS &&
	  event->xclient.format == 32 &&
	  (Atom)*event->xclient.data.l == sr->WM_DELETE_WINDOW) {
	dispose_shell(widget);
      }
    }
  }

  void _ServerResouece::register_wm_hander(Widget shell) {
    XtAddEventHandler(shell, NoEventMask, True, dispose_handler, this);
    XSetWMProtocols(XtDisplay(shell), XtWindow(shell), &WM_DELETE_WINDOW, 1 );
    /*
      DELETEメッセージを送信してもらうように
      ウィンドウ・マネージャに依頼している
     */
  }

  AppContainer::AppContainer():app_context(0),sr(0) { }

  AppContainer::~AppContainer() {
    if (sr) delete sr;
  }

  int AppContainer::run(int &argc, char **argv, Apps *app) {

    XtSetLanguageProc(NULL, NULL, NULL);
    {
      app->setContainer(this);

      String app_class = app->getAppClassName(), *fallback_resouces = app->loadFallbackResouces();
      Cardinal n_options = 0;
      XrmOptionDescRec *options = app->getAppOptions(n_options);

      Widget top =
	XtVaOpenApplication(&app_context, app_class, options, n_options,
			    &argc, argv, fallback_resouces, applicationShellWidgetClass, NULL);

      XawSimpleMenuAddGlobalActions(app_context);

      XtRegisterGrabAction(popup_action, True,
			   ButtonPressMask | ButtonReleaseMask ,
			   GrabModeAsync, GrabModeAsync);

      static XtActionsRec actions[] = {
	{ "change-item", change_item_action, },
	{ "popup-shortcut", popup_action, },
	//	{ "report", report_action, },
      };

      XtAppAddActions(app_context, actions, XtNumber(actions));

      app->loadResouces(top);
      app->createWidgets(top);
      XtRealizeWidget(top);

      _ServerResouece *sr = new _ServerResouece();
      sr->atom_initialize(XtDisplay(top));
      sr->register_wm_hander(top);
    }

    XtAppMainLoop(app_context);

    delete app;

    return 0;
  }

  /// パラメータにキーが含まれているか診断する
  bool AppContainer::param_contains(String name, String *params, Cardinal num) {
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
  void AppContainer::change_item_action(Widget list, XEvent *event, String *params, Cardinal *num) {
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
  Widget AppContainer::find_menu(Widget widget, String name) {
    Widget w, menu;

    for (w = widget; w != NULL; w = XtParent(w))
      if ((menu = XtNameToWidget(w, name)) != NULL) return menu;
    return NULL;
  }

  /// メニューの表示位置を調整する
  void AppContainer::positionMenu(Widget w, XPoint *loc) {
    Arg arglist[2];
    Cardinal num_args = 0;

    XtRealizeWidget(w);
    XtSetArg(arglist[num_args], XtNx, loc->x); num_args++;
    XtSetArg(arglist[num_args], XtNy, loc->y); num_args++;
    XtSetValues(w, arglist, num_args);
  }

  /// ポップアップ・メニューの表示
  void AppContainer::popup_action(Widget w, XEvent *event, String *params, Cardinal *num) {
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
      positionMenu(menu, &loc);
      break;
    case EnterNotify:
    case LeaveNotify:
      loc.x = event->xcrossing.x_root;
      loc.y = event->xcrossing.y_root;
      positionMenu(menu, &loc);
      break;
    case MotionNotify:
      loc.x = event->xmotion.x_root;
      loc.y = event->xmotion.y_root;
      positionMenu(menu, &loc);
      break;
    default:
      positionMenu(menu, NULL);
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

  void AppContainer::menu_popup_proc( Widget item, XtPointer client_data, XtPointer call_data) {
    cout << "TRACE: " << XtName(item) << " popup" << endl;
    ((Apps *)client_data)->popup_notify(item);
  }

  void AppContainer::menu_popdown_proc( Widget item, XtPointer client_data, XtPointer call_data) {
    cout << "TRACE: " << XtName(item) << " popdown" << endl;
    ((Apps *)client_data)->popdown_notify(item);
  }

  /** AWのポップアップメニューを作成する
   */
  Widget AppContainer::create_popup_menu(String menu_name, Apps *app, Widget shell, Simple_Menu_Item *items) {

    Widget menu =
      XtVaCreatePopupShell( menu_name, simpleMenuWidgetClass, shell, NULL );

    XtAddCallback(menu, XtNpopupCallback, menu_popup_proc, app);
    XtAddCallback(menu, XtNpopdownCallback, menu_popdown_proc, app);

    Widget item;

    for (int i = 0; items[i].name; i++) {
      if (strcmp("-", items[i].name) == 0) {
	XtVaCreateManagedWidget("-", smeLineObjectClass, menu, NULL );
	continue;
      }

      if (items[i].sub_menu) {
	Widget sub = create_popup_menu(items[i].name, app, shell, items[i].sub_menu);
	item = XtVaCreateManagedWidget(items[i].name, smeCascadeObjectClass, menu,
				       XtNsubMenu, sub, NULL );
	continue;
      }

      item = XtVaCreateManagedWidget(items[i].name, smeBSBObjectClass, menu, NULL );
      XtAddCallback(item, XtNcallback, app->item_selected, app);
    }
    return menu;
  }

  /// ダイアログをウィジェット相対で表示する
  void AppContainer::popup_dialog(Widget target, Widget shell, XtGrabKind kind) {
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

  // モーダルダイアログとして表示する（ただし無関係な他のウィンドウとは非排他的)
  void Apps::open_dialog(Widget widget, XtPointer client_data, XtPointer call_data) {
    Widget shell = (Widget)client_data;
    AppContainer::popup_dialog(widget, shell, XtGrabNonexclusive);
  }

  // ダイアログのボタンが押されたタイミングでポップダウンさせるためのコールバック
  void AppContainer::close_dialog(Widget widget, XtPointer client_data, XtPointer call_data) {
    Widget dialog = (Widget)client_data;
    XtPopdown(dialog);
  }

  // 子ウィジェットを探して、アクセラレータを登録する
  void AppContainer::install_accelerators(Widget parent, const char *name) {
    Widget wi = XtNameToWidget(parent, name);
    if (wi) {
      XtAccelerators accelerators;
      XtVaGetValues(wi, XtNaccelerators, &accelerators, NULL);
      XtInstallAccelerators(parent, wi);
      cerr << "TRACE: widget " << name << ":" << getXtName(wi) << endl;
      cerr << "TRACE: accelerators: " << accelerators << endl;
    }
  }

  // メッセージ表示のダイアログを作成する
  Widget AppContainer::message_dialog(String prompt, String apply_name, Apps *app, Widget owner) {

    Widget shell = XtVaCreatePopupShell("confirm", transientShellWidgetClass, owner, NULL );
    Widget dialog = XtVaCreateManagedWidget("dialog", dialogWidgetClass, shell, NULL );
    XtVaSetValues(dialog, XtNlabel, XtNewString(prompt), NULL);

    /* ウィジット関数によってダイアログ内部のボタンをセットする */
    if (apply_name && apply_name[0]) {
      XawDialogAddButton(dialog, "yes", app->item_selected, app);
      XawDialogAddButton(dialog, "no", close_dialog, shell );
      install_accelerators(dialog, "yes");
      install_accelerators(dialog, "no");
    }
    else {
      XawDialogAddButton(dialog, "ok", close_dialog, shell );
      install_accelerators(dialog, "ok");
    }
    return shell;
  }

  // ダイアログテキストを初期化する
  void AppContainer::clear_dialog_text_proc( Widget widget, XtPointer client_data, XtPointer call_data) {
    Widget dialog = (Widget )client_data;
    XtVaSetValues(dialog, XtNvalue, XtNewString(""), NULL);
  }

  // 入力ダイアログを作成する
  Widget AppContainer::input_dialog(String prompt, String defaultValue, String apply_name, Apps *app, Widget owner) {

    Widget shell = XtVaCreatePopupShell("confirm", transientShellWidgetClass, owner, NULL );
    Widget dialog = XtVaCreateManagedWidget("dialog", dialogWidgetClass, shell, NULL );
    XtVaSetValues(dialog, XtNlabel, XtNewString(prompt), XtNvalue, XtNewString(""), NULL);

    XawDialogAddButton(dialog, apply_name, app->item_selected, app);
    XawDialogAddButton(dialog, "clear", clear_dialog_text_proc, dialog );
    XawDialogAddButton(dialog, "cancel", close_dialog, shell );

    install_accelerators(dialog, apply_name);
    install_accelerators(dialog, "cancel");

    Widget value = XtNameToWidget(dialog, "value");
    install_accelerators(value, apply_name);
    install_accelerators(value, "cancel");

    return shell;
  }


  /// -------- ここまでコンテナのコード


  class MessageApps : public Apps {
    String getAppClassName();
    void loadResouces(Widget shell);
    void createWidgets(Widget shell); 
  private:
    struct message_res { Boolean help; String message; };
    message_res res;
  };

  String MessageApps::getAppClassName() {
    static String name = "Message";
    return name;
  }

  void MessageApps::loadResouces(Widget shell) {
    static XtResource resources[] = {
      { "message", "Message", XtRString, sizeof(String),
	XtOffset(message_res *, message), XtRImmediate, (XtPointer)"Hello,world" },
    };
    XtVaGetApplicationResources(shell, &res, resources, XtNumber(resources), NULL);
  }

  void MessageApps::createWidgets(Widget shell) {
    Widget label =
      XtVaCreateManagedWidget("message", labelWidgetClass, shell,
			      XtNjustify, XtJustifyCenter,
			      XtNlabel, XtNewString(res.message),
			      NULL);
  }

  // --------------------------------------------------------------------------------

  class ButtonApps : public Apps {
    String getAppClassName();
    void createWidgets(Widget shell); 
  };

  String ButtonApps::getAppClassName() {
    static String name = "ButtonOnly";
    return name;
  }

  void ButtonApps::createWidgets(Widget shell) {
      Widget panel =
	XtVaCreateManagedWidget("panel", boxWidgetClass, shell, NULL);
      Widget close =
	XtVaCreateManagedWidget("close", commandWidgetClass, panel, NULL);

      XtAddCallback(close, XtNcallback, item_selected, this);
  }

  // --------------------------------------------------------------------------------

  class ConfirmApps : public Apps {
    String getAppClassName();
    void createWidgets(Widget shell); 
    void command_proc(string &cmd, Widget widget);
  };

  String ConfirmApps::getAppClassName() {
    static String name = "Confirm";
    return name;
  }

  void ConfirmApps::command_proc(string &cmd, Widget widget) {
    if (cmd == "close" || cmd == "yes")
      disposeShell(widget);
  }

  void ConfirmApps::createWidgets(Widget shell) {
    Widget box = XtVaCreateManagedWidget("box", boxWidgetClass, shell, NULL);
    Widget press = XtVaCreateManagedWidget("press", commandWidgetClass, box, NULL);

    Widget dialog = ac->message_dialog("aaaa", "yes", this, shell);
    XtAddCallback(press, XtNcallback, open_dialog, dialog);
  }

  /// --------------------------------------------------------------------------------

  // AWのクラスの継承ツリーを出力する
  class ClassTreeApps : public Apps {
    // クラス名とそれを表示するウィジェットの組を保持する
    map<string, Widget> nodes;
    Widget find_node(WidgetClass wc);
    Widget add_node(WidgetClass wc, Widget tree);

    String getAppClassName();
    void createWidgets(Widget shell); 
  };

  String ClassTreeApps::getAppClassName() {
    static String name = "CTree";
    return name;
  }

  // クラスに対応するwidgetを入手する
  Widget ClassTreeApps::find_node(WidgetClass wc) {
    const char *cn = getXtClassName(wc);
    map<string, Widget>::iterator it = nodes.find(cn);
    // クラス名でウィジェットを引き出す
    if (it == nodes.end()) return None;
    return (*it).second;
  }

  void ClassTreeApps::createWidgets(Widget top) {

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
  }

  // クラスに対応するwidgetを追加する
  Widget ClassTreeApps::add_node(WidgetClass wc, Widget tree) {
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

  /// --------------------------------------------------------------------------------
  // フォルダの一覧

  class FolderListApps : public Apps {
    Widget list;
    vector<string> entries;

    XawListReturnStruct last_selected;
    XtIntervalId timer;
    long interval; // ms

    static void item_selected_timer(XtPointer closure, XtIntervalId *id);
    static void list_item_selected( Widget widget, XtPointer client_data, XtPointer call_data);

  public:
    FolderListApps() : list(0), timer(0), interval(400) { }
    String getAppClassName();
    void createWidgets(Widget shell);
    void command_proc(string &cmd, Widget widget);
  };

  String FolderListApps::getAppClassName() {
    static String name = "FolderList";
    return name;
  }


  // リストアイテムを選択した時の処理
  void FolderListApps::item_selected_timer(XtPointer closure, XtIntervalId *id) {
    FolderListApps *app = (FolderListApps *)closure;
    app->timer = 0;
    cerr << "TRACE: item selected:" << app->last_selected.list_index
	 << ":" << app->last_selected.string << endl;
  }

  // リストアイテムを選択した時の処理
  void FolderListApps::list_item_selected( Widget widget, XtPointer client_data, XtPointer call_data) {
    XawListReturnStruct *rs = (XawListReturnStruct *)call_data;
    FolderListApps *app = (FolderListApps *)client_data;

    app->last_selected = *rs;
    // このタイミングではタイマーを設定するだけで、主要な処理は遅延実行する

    if (app->timer) XtRemoveTimeOut(app->timer);
    app->timer =
      XtAppAddTimeOut(XtWidgetToApplicationContext(widget),
		      app->interval, item_selected_timer, app);
  }

  void FolderListApps::command_proc(string &cmd, Widget widget) {
    if (cmd == "close" || cmd == "yes") {
      disposeShell(widget);
      return;
    }

    cerr << "TRACE: " << cmd << " selected." << endl;
  }

  void FolderListApps::createWidgets(Widget top) {

    xwin::load_dirent(".", entries, false);
    String *slist = xwin::as_String_array(entries);

    Widget pane =
      XtVaCreateManagedWidget("pane", panedWidgetClass, top, NULL );
    Widget menu_bar =
      XtVaCreateManagedWidget("menu_bar", boxWidgetClass, pane, NULL);

    XtVaCreateManagedWidget("list", menuButtonWidgetClass, menu_bar,
			    XtNmenuName, "list-menu",
			    NULL);

    struct Simple_Menu_Item sub_items[] = {
      { "sub-item1", },
      { "sub-item2", },
      { "sub-item3", },
      { 0, },
    };

    struct Simple_Menu_Item items[] = {
      { "item1", },
      { "item2", },
      { "menu-item2", sub_items },
      { "-", },
      { "close", },
      { 0, },
    };

    ac->create_popup_menu("list-menu", this, pane, items);

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
    XtAddCallback(list, XtNcallback, list_item_selected, this);
    XtSetKeyboardFocus(pane, list);

    Widget close = 
      XtVaCreateManagedWidget("close", commandWidgetClass, menu_bar, NULL);
    XtAddCallback(close, XtNcallback, item_selected, this);

    XtInstallAccelerators(list, close);
    XtRealizeWidget(top);

    // 初期状態で選択されているようにする
    XawListHighlight(list, 0);

    Widget vertical = XtNameToWidget(viewport, "vertical");
    if (vertical) XtInstallAccelerators(list, vertical);
  }

  /// --------------------------------------------------------------------------------

  struct TextBuffer {
    Widget buf;
    wstring fetch_selected_text();
    void select_all();
    void append_text(const wchar_t *buf, size_t len);
    void replace_text(const wchar_t *buf, size_t len);
    void insert_text(const wchar_t *buf, size_t len);
  };

  /// 選択された領域のテキストを入手する
  wstring TextBuffer::fetch_selected_text() {
    XawTextPosition begin_pos, end_pos;

    XawTextGetSelectionPos(buf, &begin_pos, &end_pos);
    if (begin_pos == end_pos) return L"";

    XawTextBlock text;
    XawTextPosition pos = XawTextSourceRead(XawTextGetSource(buf), begin_pos, &text, end_pos - begin_pos);

#if 0
    cerr << "begin:" << begin_pos << endl;
    cerr << "end:" << end_pos << endl << endl;
    cerr << "pos:" << pos << endl << endl;
    cerr << "first:" << text.firstPos << endl;
    cerr << "length:" << text.length << endl;
    cerr << "format:" << text.format << endl;
#endif

    if (text.format == XawFmtWide) {
      // ワイド文字で返って来るようだ
      wstring wstr(((wchar_t *)text.ptr), text.length);
      return wstr;
    }
    return L"";
  }

  void TextBuffer::select_all() {
    XawTextPosition last = XawTextLastPosition(buf);
    XawTextSetSelection(buf,0,last);
  }

  /// テキストを末尾に追加する
  void TextBuffer::append_text(const wchar_t *wbuf, size_t len) {
    XawTextPosition last = XawTextLastPosition(buf);
    XawTextBlock block;
    block.firstPos = 0;
    block.length = len;
    block.ptr = (char *)wbuf;
    block.format = XawFmtWide;
    int rc = XawTextReplace(buf, last, last, &block);
    cerr << "TRACE: aw append: " << rc << endl;
    XawTextSetInsertionPoint(buf, last + len);
  }

  /// 選択テキストを置き換える
  void TextBuffer::replace_text(const wchar_t *wbuf, size_t len) {
    XawTextPosition begin_pos, end_pos;
    XawTextGetSelectionPos(buf, &begin_pos, &end_pos);
    XawTextBlock block;
    block.firstPos = 0;
    block.length = len;
    block.ptr = (char *)wbuf;
    block.format = XawFmtWide;
    int rc = XawTextReplace(buf, begin_pos, end_pos, &block);
    cerr << "TRACE: aw replace selection: " << rc << endl;
  }

  /// テキストをカレット位置に追加する
  void TextBuffer::insert_text(const wchar_t *wbuf, size_t len) {
    XawTextPosition pos = XawTextGetInsertionPoint(buf);
    XawTextBlock block;
    block.firstPos = 0;
    block.length = len;
    block.ptr = (char *)wbuf;
    block.format = XawFmtWide;
    int rc = XawTextReplace(buf, pos, pos, &block);
    cerr << "TRACE: aw insert: " << rc << endl;
    XawTextSetInsertionPoint(buf, pos + len);
  }

  class EditorApps : public Apps {
    TextBuffer tbuf;
    Widget status, goto_line_dialog;
    /// 時刻表示のタイマー
    XtIntervalId timer;
    long interval; // 時刻表示のインターバル

    XtIntervalId position_timer;
    long position_interval; // ms
    XawTextPositionInfo save_pos;

    static Boolean text_ready(XtPointer closure);
    static void show_time_proc(XtPointer closure, XtIntervalId *id);
    static void show_potision_proc(XtPointer closure, XtIntervalId *id);
    static void position_callback(Widget widget, XtPointer client_data, XtPointer call_data);

  public:
    EditorApps() : timer(0), interval(400) { }
    String getAppClassName();
    void createWidgets(Widget shell);
    void command_proc(string &cmd, Widget widget);
  };

  String EditorApps::getAppClassName() {
    static String name = "Memo";
    return name;
  }

  void EditorApps::command_proc(string &cmd, Widget widget) {

    if (cmd == "select-all") {
      tbuf.select_all();
      return;
    }

    if (cmd == "goto-line") {
      if (!goto_line_dialog)
	goto_line_dialog = ac->input_dialog("Line number:", "", "jump", this, tbuf.buf);
      
      XtVaSetValues(goto_line_dialog, XtNvalue, XtNewString(""), NULL);
      // モーダルダイアログ（ただし他のウィンドウとは非排他的)
      ac->popup_dialog(widget, find_shell(goto_line_dialog, 0), XtGrabNonexclusive);
      return;
    }

    if (cmd == "now") {
      struct tm local;
      fetch_localtime(&local);
      char buf[200];
      strftime(buf, sizeof buf, "%Y, %B, %d, %A %p%I:%M:%S", &local);
      size_t len = strlen(buf);
      wchar_t wcs[len + 1];
      len = mbstowcs(wcs, buf, len);
      if (len != (size_t)-1) tbuf.insert_text(wcs, len);
    }

    if (cmd == "delete-selection") {
      tbuf.select_all();
      tbuf.replace_text(L"",0);
      return;
    }

    if (cmd == "hello") {
      wchar_t *ws = L"hello";
      tbuf.append_text(ws, wcslen(ws));
      return;
    }

    if (cmd == "fetch-text") {
      wstring ws = tbuf.fetch_selected_text();
      cerr << ws.c_str() << endl;
      return;
    }

    if (cmd == "close") {
      disposeShell(widget);
      return;
    }

    cerr << "TRACE: " << cmd << " selected." << endl;
  }

  void EditorApps::createWidgets(Widget top) {
    Widget pane =
      XtVaCreateManagedWidget("pane", panedWidgetClass, top, NULL );
    Widget menu_bar =
      XtVaCreateManagedWidget("menu_bar", boxWidgetClass, pane, NULL);

    XtVaCreateManagedWidget("memo", menuButtonWidgetClass, menu_bar,
			    XtNmenuName, "memo-menu",
			    NULL);
    Widget buf =
      XtVaCreateManagedWidget("textbuf", asciiTextWidgetClass, pane,
			      XtNeditType, XawtextEdit,
			      XtNtype, XawAsciiString,
			      XtNwrap, XawtextWrapWord,
			      XtNscrollVertical, XawtextScrollWhenNeeded, // or XawtextScrollAlways,
			      NULL);

    //XtAddCallback(buf, XtNpositionCallback, position_callback, this);
    tbuf.buf = buf;

    Widget close =
      XtVaCreateManagedWidget("close", commandWidgetClass, menu_bar, NULL);
    XtAddCallback(close, XtNcallback, item_selected, this);
    XtInstallAccelerators(buf, close);

    struct Simple_Menu_Item submenu[] = {
      { "item1", },
      { "item2", },
      { "item3", },
      { 0, },
    };

    struct Simple_Menu_Item items[] = {
      { "select-all", },
      { "delete-selection", },
      { "hello", },
      { "fetch-text", },
      { "now", },
      { "goto-line", },
      { "sub-menu", submenu },
      { "-", },
      { "close", },
      { 0, },
    };

    ac->create_popup_menu("memo-menu", this, top, items);

    struct Simple_Menu_Item shortcut_item[] = {
      { "aaa", },
      { "bbb", },
      { "fetch-text", },
      { "sub-menu", submenu },
      { "-", },
      { "close", },
      { 0, },
    };

    ac->create_popup_menu("text-shortcut", this, top, shortcut_item);

#if 1
    status =
      XtVaCreateManagedWidget("status", asciiTextWidgetClass, pane,
			      XtNscrollHorizontal, XawtextScrollNever,
			      XtNdisplayCaret, False,
			      XtNskipAdjust, True, NULL);
#endif

    XtAddEventHandler(top, NoEventMask, True, _XEditResCheckMessages, NULL);

#if 1
    XtAppContext context = XtWidgetToApplicationContext(top);
    XtWorkProcId work = XtAppAddWorkProc(context, text_ready, this);
    cerr << "TRACE: ready proc: " << this << ":" << work << endl;
    cerr << "TRACE: now locale C_TYPE: " << setlocale (LC_CTYPE, 0) << endl;
#endif

    XtSetKeyboardFocus(pane, buf);
  }


  /// 処理するイベントがなくなった時に呼び出される
  Boolean EditorApps::text_ready(XtPointer closure) {
    EditorApps *app = (EditorApps *)closure;

    cerr << "TRACE: work proc called" << endl;
    wstring wtext = xwin::load_wtext(__FILE__);

    app->tbuf.append_text(wtext.c_str(), wtext.size());
    XawTextSetInsertionPoint(app->tbuf.buf, 0);

    // 時刻表示のタイマーを登録
    app->timer =
      XtAppAddTimeOut(XtWidgetToApplicationContext(app->status),
		      app->interval, show_time_proc, app);
    return True;
  }

  /// 現在時刻の表示
  void EditorApps::show_time_proc(XtPointer closure, XtIntervalId *id) {
    EditorApps *app = (EditorApps *)closure;
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

  /// 現在カーソル位置の表示
  void EditorApps::show_potision_proc(XtPointer closure, XtIntervalId *id) {
    EditorApps *app = (EditorApps *)closure;
    app->position_timer = 0;
    cerr << "Line: " << app->save_pos.line_number <<
      " Column: " << (app->save_pos.column_number + 1) << "     \r";
  }

  /// テキスト位置が変化したタイミングで呼び出される
  void EditorApps::position_callback(Widget widget, XtPointer client_data, XtPointer call_data) {
    XawTextPositionInfo *pos = (XawTextPositionInfo *)call_data;
    EditorApps *app = (EditorApps *)client_data;
    if (0)
      cerr << XtName(widget) << " position callback" << endl;
    app->save_pos = *pos;
    if (app->position_timer)
      XtRemoveTimeOut(app->position_timer);
    app->position_timer =
      XtAppAddTimeOut(XtWidgetToApplicationContext(app->status),
		      app->position_interval, show_potision_proc, app);
  }


};

extern "C"{
  static int msg_window2(int argc, char **argv) {
    awt::AppContainer ac;
    return ac.run(argc, argv, new awt::MessageApps());
  }

  static int button_window(int argc, char **argv) {
    awt::AppContainer ac;
    return ac.run(argc, argv, new awt::ButtonApps());
  }

  static int confirm_window(int argc, char **argv) {
    awt::AppContainer ac;
    return ac.run(argc, argv, new awt::ConfirmApps());
  }

  static int ctree_window(int argc, char **argv) {
    awt::AppContainer ac;
    return ac.run(argc, argv, new awt::ClassTreeApps());
  }

  static int folder_window(int argc, char **argv) {
    awt::AppContainer ac;
    return ac.run(argc, argv, new awt::FolderListApps());
  }

  static int memo_window(int argc, char **argv) {
    awt::AppContainer ac;
    return ac.run(argc, argv, new awt::EditorApps());
  }

};


// --------------------------------------------------------------------------------

#ifdef USE_SUBCMD

#include "subcmd.h"

subcmd dawt_cmap[] = {
  { "d:msg", msg_window, },
  { "d:msg2", msg_window2, },
  { "d:btn", button_window, },
  { "d:conf", confirm_window, },
  { "d:tree", ctree_window, },
  { "d:folder", folder_window, },
  { "d:memo", memo_window, },
  { 0 },
};

#endif

