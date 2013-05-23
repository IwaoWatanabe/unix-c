/*! \file
 * \brief Xlibを利用したGUIサンプル・コード
 */

#include <cerrno>
#include <clocale>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <iostream>
#include <vector>

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

    virtual ~xlib01() {}
    /// サーバに接続
    virtual bool connect_server(char *display_name = "");
    /// アプリケーション・ウィンドウを作成
    virtual void create_application_window();
    /// イベントループ
    virtual void event_loop();
    /// リソースの開放
    virtual void dispose();
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

namespace xwin {

  /// 行単位でテキストを操作するクラス
  class line_text {
    vector<wchar_t *>lines;
  public:
    line_text();
    ~line_text();
    int get_text_lines();
    wchar_t *get_text(int idx);
    int add_text(const wchar_t *t);
    int insert_text(int idx, const wchar_t *t);
    int replace_text(int idx, int column, const wchar_t *t);
    void delete_text(int idx);
    void clear();
  };

  line_text::line_text() { }
  line_text::~line_text() { clear(); }

  /// 保持している行数を返す
  int line_text::get_text_lines() { return lines.size(); }

  /// 指定する行位置のテキストを返す
  wchar_t *line_text::get_text(int idx) { return lines[idx]; }

  /// テキストを末尾に追加する
  int line_text::add_text(const wchar_t *t)  { 
    wchar_t *tt = wcsdup(t);
    if (!tt) return 0;
    lines.push_back(tt);
    return 1;
  }

  /// テキストを指定する行位置に挿入する
  int line_text::insert_text(int pos, const wchar_t *t) {
    wchar_t *tt = wcsdup(t);
    if (!tt) return 0;
    vector<wchar_t *>::iterator it = lines.begin();
    while (pos-- > 0 && it != lines.end()) { it++; }
    if (pos <= 0) {
      lines.insert(it, tt);
      return 1;
    }
    free(tt);
    return 0;
  }

  /// テキストの指定桁位置のテキストを置き換える
  int line_text::replace_text(int pos, int col, const wchar_t *t) {

    if (pos < 0 || (size_t)pos > lines.size()) return 0;

    wchar_t *cur = lines[pos];
    
    size_t len = wcslen(cur), len2 = wcslen(t);
    if (col < len && col + len2 < len) {
      // これまで確保していた範囲に収まる場合
      wcscpy(cur + col, t);
      return 1;
    }
    
    wchar_t *tt = (wchar_t *)realloc(cur, sizeof(wchar_t) * (col + len2 + 1));
    if (tt == NULL) {
      cerr << "realloc failed: " << (col + len2) << endl;
      return 0;
    }
    
    lines[pos] = cur = tt;
    
    if (col < len) 
      tt += col;
    else {
      tt += len;
      while(tt < cur + col) { *tt++ =  L' '; }
    }
    wcscpy(tt,t);
    return 0;
  }

  /// 指定する行位置のテキストを削除する
  void line_text::delete_text(int pos) {
    vector<wchar_t *>::iterator it = lines.begin();
    while (pos-- > 0 && it != lines.end()) { it++; }
    free(*it);
    lines.erase(it);
  }

  /// 保持するデータを破棄する
  void line_text::clear() {
    for (vector<wchar_t *>::iterator it = lines.begin(); it != lines.end(); it++)
      free(*it);
    lines.clear();
  }

  /// テキストを表示するアプリケーション
  struct xlib02 : xlib01 {
    GC gc;
    XFontSet font;
    line_text data;
    virtual bool load_display_text(const char *filename = __FILE__ );
    virtual bool locale_initialize(const char *lang = "");
    virtual XFontSet load_font(const char *font_name = "-*--14-*,-*--24-*");
    virtual void create_application_window();
    virtual void event_loop();
    virtual void process_expose(XEvent *event);
    virtual void dispose();
  };

  /// ロケールの初期化
  bool xlib02::locale_initialize(const char *lang) {
    if (!setlocale(LC_CTYPE, lang)) {
      cerr << "ERROR: invalid locale:" << lang << endl;
      return false;
    }
    if (!XSupportsLocale()) {
      cerr << "ERROR: unsupported locale: " << setlocale(LC_CTYPE, NULL) << endl;
      return false;
    }
    return true;
  }

  /// フォントの読み込み
  XFontSet xlib02::load_font(const char *font_name) {
    int missing_count = 0;
    char **missing_list = NULL, *default_string;
    
    XFontSet font = 
      XCreateFontSet(display, font_name,
		     &missing_list, &missing_count, &default_string );

    if (font == NULL) {
      cerr << "ERROR: failed to create fontset:" << font_name << endl;
      return font;
    }

    if (missing_count) {
      // 読み込めなかったフォントがあった
      cerr << "WARNING: font list missing: " << font_name << endl;
      for (int i = 0; i < missing_count; i++)
	cerr << "\t" << missing_list[i] << endl;
      
      cerr << "default string: " << default_string << endl;
      /*
	表示できない文字の代替文字
      */
      XFreeStringList(missing_list);
    }
    return font;
  }

  /// アプリケーションのウインドウを作成する
  void xlib02::create_application_window() {

    xlib01::create_application_window();

    XGCValues values;
    values.foreground = BlackPixel(display, DefaultScreen(display));
    gc = XCreateGC( display, window, GCForeground, &values);

    unsigned long event_mask = ButtonPressMask|KeyPressMask|ExposureMask;
    XSelectInput(display, window, event_mask);
  }

  /// dequee の内容を配列に置き換え
  wchar_t **as_wchar_array(deque<wchar_t *> &deq, size_t *size_return = NULL) {

    wchar_t **aa = (wchar_t **)malloc(sizeof (wchar_t *) * (deq.size() + 1)), **p = aa;
    if (aa) {
      for (deque<wchar_t *>::iterator it = deq.begin(); it != deq.end(); it++)
	*p++ = *it;
      *p++ = NULL;
      if (size_return) *size_return = deq.size();
    }
    return aa;
  }

  /// テキストを指定するファイルから読み込む
  bool xlib02::load_display_text(const char *filename) {
    FILE *fp;
    
    if ((fp = fopen(filename, "r")) == NULL) {
      cerr << "ERROR: file open failed: " << filename << strerror(errno) << endl;
      return false;
    }

    char buf[1024];
    wchar_t wbuf[1024];
    int ct = 0;

    while (fgets(buf,sizeof buf, fp)) {
      char *p = strchr(buf,'\n');
      if (p) { ct++; *p = 0; }
      
      size_t len = mbstowcs(wbuf, buf, sizeof wbuf/sizeof wbuf[0]);
      if (len == (size_t)-1) {
	cerr << "WARNING: invalid state mbstring on " << ct << " (ignored)" << endl;
	continue;
      }
    
      if (!data.add_text(wbuf)) {
	cerr << "ERROR: out of memory(wcsdup)" << endl;
	break;
      }
    }

    if (fclose(fp) != 0) {
      cerr << "WARNING: file close failed: " << filename << strerror(errno) << endl;
    }
    cerr << "INFO: read " << data.get_text_lines() << " lines:" << filename << endl;

    return true;
  }

  /// Expose イベントを処理する
  void xlib02::process_expose(XEvent *event) {
    if (event->xexpose.window != window) return;

    int n = 0, x = 0, y = 0;
    XRectangle overall_ink, overall_logical;

    size_t len;
    wchar_t *ws;

    for (n = 0; (ws = data.get_text(n)); n++) {
      len = wcslen(ws);
      if (len == 0) { ws = L" "; len = 1; }
      
      XwcTextExtents(font, ws, len, &overall_ink, &overall_logical);

      XwcDrawString(display, window, font, gc,
		    x, y - overall_logical.y, ws, len);

      y += overall_logical.height;
      if (y >= event->xexpose.y + event->xexpose.height) break;
    }
  }

  /// イベントループ
  void xlib02::event_loop() {
    XMapWindow(display, window);

    XEvent event;
    int exit_flag = 0;

    while (!exit_flag) {
      XNextEvent(display, &event);

      switch (event.type) {
      case KeyPress: case ButtonPress:
	exit_flag = 1;
	break;
      case Expose:
	process_expose(&event);
	break;
      }
    }
  }

  void xlib02::dispose() {
    XFreeGC(display, gc);
    XFreeFontSet(display, font);
    xlib01::dispose();
  }
};

/// テキストを表示するアプリケーション
static int text_list(int argc,char **argv) {
  xwin::xlib02 *app = new xwin::xlib02;

  const char *text_file = getenv("TEXT");
  if (!text_file) text_file = __FILE__;

  const char *font_name = getenv("FONT_NAME");
  if (!font_name) font_name = "-*--14-*,-*--24-*";

  if (!app->locale_initialize()) return 1;
  if (!app->load_display_text(text_file)) return 1;
  
  if (!app->connect_server()) return 1;
  if (!(app->font = app->load_font(font_name))) return 1;
  
  app->create_application_window();
  app->event_loop();
  app->dispose();
  return 0;
}

subcmd xwin_cmap[] = {
  { "win", simple_window, },
  { "win2", simple_window02, },
  { "text", text_list, },
  { 0 },
};

