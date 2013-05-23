


#include <clocale>
#include <iostream>

#include <X11/Xlib.h>
#include <X11/Xlocale.h>
#include <X11/Xresource.h>
#include <X11/Xutil.h>

#include "subcmd.h"

using namespace std;

/// ウィンドウだけ表示する簡単な例
static int simple_window(int argc,char **argv) {

  char *display_name = "";
  Display *display = XOpenDisplay(display_name);
  /*
    Xサーバと接続する。接続できなければ NULLが返る。
    接続名が空テキストであれば、環境変数 $DISPLAYに設定されている値を利用する。
   */
  if (!display) {
    cerr << "ERROR: can not connect xserver." << endl;
    return 1;
  }
  
  Window parent = DefaultRootWindow(display);
  int screen = DefaultScreen(display);
  unsigned long background = WhitePixel(display, screen), 
    border_color = BlackPixel(display, screen);
  int x = 0, y = 0, width = 100, height = 100, border_width = 2;

  Window window = 
    XCreateSimpleWindow(display, parent, x, y, width, height, 
			border_width, border_color, background);
  /*
    ウィンドウを作成する。
    これで指定していない、その他のウィンドウ属性は親から引き継ぐ。
    ルートの配置するトップレベルウィンドウの場合は、
    ウィンドウマネージャの介入が入って、
    配置位置や大きさはリクエスト通りにはならないこともある。
    生成した直後は非表示状態なので、あとで XMapWindow　を使って表示状態にする。
   */
  
  XSelectInput(display, window, ButtonPressMask|KeyPressMask);
  /*
    受け入れるイベントを設定する。
    この例では、マウスのボタンとキーボードの押下を検出する。
   */

  XMapWindow(display, window);
  /*
    表示状態にする。
    このタイミングでウィンドウマネージャの介入が入る。
   */

  XEvent event;
  int exit_flag = 0;

  while (!exit_flag) {
    XNextEvent(display, &event);
    /*
      Xサーバから送付されてくるイベントを入手する。
      イベントが到達するまでブロックしている。
    */

    switch (event.type) {
    case KeyPress: case ButtonPress:
      exit_flag = 1;
    }
  }

  XDestroyWindow(display, window);
  XCloseDisplay(display);

  return 0;
}

namespace xwin {

  /// Xアプリケーションの雛形
  struct xlib01 {
    /// Xサーバ接続情報
    Display *display;
    /// 作業ウィンドウ
    Window window;
    /// サーバに接続
    bool connect_server(char *display_name = "");
    /// アプリケーション・ウィンドウを作成
    void create_application_window();
    /// イベントループ
    void event_loop();
    /// リソースの開放
    void dispose();
  };

  /// Xサーバに接続する
  bool 
  xlib01::connect_server(char *display_name)
  {
    display = XOpenDisplay(display_name);
    /*
      Xサーバと接続する。接続できなければ NULLが返る。
      接続名が空テキストであれば、環境変数 $DISPLAYに設定されている値を利用する。
    */
    if (!display) {
      cerr << "ERROR: can not connect xserver." << endl;
      return false;
    }
    return true;
  }

  /// ウィンドウを作成する
  void 
  xlib01::create_application_window()
  {
    Window parent = DefaultRootWindow(display);
    int screen = DefaultScreen(display);
    unsigned long background = WhitePixel(display, screen),
      border_color = BlackPixel(display, screen);
    int x = 0, y = 0, width = 400, height = 200, border_width = 2;
    
    window = 
      XCreateSimpleWindow(display, parent, x, y, width, height, 
			  border_width, border_color, background);

    unsigned long event_mask = ButtonPressMask|KeyPressMask;
    XSelectInput(display, window, event_mask);
  }

  /// イベントループ
  void 
  xlib01::event_loop()
  {
    XMapWindow(display, window);

    XEvent event;
    int exit_flag = 0;
    while (!exit_flag) {
      XNextEvent(display, &event);
      
      switch (event.type) {
      case KeyPress: case ButtonPress:
	exit_flag = 1;
      }
    }
  }

  /// リソースを破棄する
  void
  xlib01::dispose()
  {
    XDestroyWindow(display, window);
    XCloseDisplay(display);
  }

};

/// C++ で作成した Simple Window
static int
simple_window02(int argc,char **argv) {
  xwin::xlib01 *app = new xwin::xlib01;
  
  if (!app->connect_server()) return 1;
  app->create_application_window();
  app->event_loop();
  app->dispose();
  return 0;
}


subcmd xwin_cmap[] = {
  { "win", simple_window, },
  { "win2", simple_window02, },
  { 0 },
};

