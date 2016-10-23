
/*! \file
 * \brief Xlibを利用したGUIサンプル・コード
 */

#include <csignal>
#include <X11/Xlib.h>
#include "uc/elog.hpp"

extern "C" {

  static int exit_flag = 0;

  static void interrupt_handler(int sig) { exit_flag = 1; }

  static void setup_interrupt_handler() {
    if (SIG_ERR == signal(SIGINT, interrupt_handler))
      perror("ERROR: sinale SIGINT");
  }

  enum { FATAL, ERROR, WARN, NOTICE, INFO, AUDIT, DEBUG, TRACE };

  /// ウィンドウだけ表示する簡単な例
  static int simple_window(int argc,char **argv) {

    const char *display_name = "";
    Display *display = XOpenDisplay(display_name);
    /*
      Xサーバと接続する。接続できなければ NULLが返る。
      接続名が空テキストであれば、環境変数 $DISPLAYに設定されている値を利用する。
    */
    if (!display) {
      elog(ERROR, "can not connect xserver.");
      return 1;
    }
  
    Window parent = DefaultRootWindow(display);
    int screen = DefaultScreen(display);
    unsigned long background = WhitePixel(display, screen), 
      border_color = BlackPixel(display, screen);
    int x = 0, y = 0, width = 300, height = 100, border_width = 2;

    Window window = 
      XCreateSimpleWindow(display, parent, x, y, width, height, 
			border_width, border_color, background);
    /*
      ウィンドウを作成する。
      指定していない、その他のウィンドウ属性は親から引き継ぐ。
      ルートの配置するトップレベルウィンドウの場合は、
      ウィンドウマネージャの介入が入って、
      配置位置や大きさはリクエスト通りにはならないこともある。
      生成した直後は非表示状態なので、あとで XMapWindow　を使って表示状態にする。
    */

    unsigned long event_mask = ButtonPressMask|KeyPressMask;
    XSelectInput(display, window, event_mask);
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

    setup_interrupt_handler();

    while (!exit_flag) {
      XNextEvent(display, &event);
      /*
	Xサーバから送付されてくるイベントを入手する。
	イベントが到達するまでブロックしている。
      */

      switch (event.type) {
      case KeyPress: case ButtonPress:
	exit_flag = 1;
	/*
	  キーが押下されるか、ポインティング・デバイス（マウス等）の
	  ボタンが押下されたら終了する
	*/
      }
    }

    XDestroyWindow(display, window);
    XCloseDisplay(display);
    /*
      サーバにリソースを開放のリクエストを送っている。
      プロセスが終了するなら、これらを呼び出さずとも自動的に開放される。
    */

    elog(TRACE, "display closed.");

    return 0;
  }

}


// --------------------------------------------------------------------------------

#ifdef USE_SUBCMD

#include "subcmd.h"

subcmd dxwin_cmap[] = {
  { "d:win", simple_window, },
  { 0 },
};

#endif


