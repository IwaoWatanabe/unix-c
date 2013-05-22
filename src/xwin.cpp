


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


subcmd xwin_cmap[] = {
  { "win", simple_window },
  { 0 },
};

