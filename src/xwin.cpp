/*! \file
 * \brief Xlibを利用したGUIサンプル・コード
 */

#include <X11/Xlib.h>
#include <X11/Xresource.h>

#include <libgen.h>

namespace xwin {

  /// Xの資源を利用するコードが実装するインタフェース
  class X_resoruce {
  public:
    virtual ~X_resoruce() { }
    /// このメソッドが呼ばれたらリソースを開放する

    /// このメソッドを呼び出されたらリソースを確保する準備ができている
    virtual void realize() { }
    /*
      何度呼ばれても大丈夫なようにすること
     */
    virtual void dispose() { }
  };

  class X_frame;

  /// Xサーバの接続情報を管理するクラス
  /*
    実装クラスはコンテナによって提供される。
    ユーザ・コードが継承することはない。
  */
  class X_server {
  public:
    /// サーバ接続の参照
    Display *ref;
    /// ウィンドウ・マネージャとの通信用
    Atom WM_PROTOCOLS, WM_DELETE_WINDOW;

    virtual ~X_server() { }
    /// リソース・データベースを入手する
    virtual XrmDatabase get_resource_database();
    /// Atomに割りあたっているテキストを入手する
    virtual const char *get_atom_name(Atom atom);
    /// Xの接続時にAtomを設定するアドレスを定義
    virtual void store_atom(const char *name, Atom *atom) = 0;
    /// Xの接続切断前に解放するリソースを登録する
    virtual void add_resoruce(X_resoruce *res) = 0;
    /// リソースの登録を解除する
    virtual void remove_resoruce(X_resoruce *res) = 0;
    /// サーバに接続する
    virtual bool connect(const char *display_name = "") = 0;
    /// サーバへの接続を解除する
    virtual void disconnect() = 0;
    /// フレームを登録する
    virtual void add_frame(X_frame *frame) = 0;
    /// フレームの登録を解除する
    virtual void remove_frame(X_frame *frame) = 0;

    virtual XFontSet load_font(const char *font_name);
    /// このメソッドを呼び出されたらリソースを確保する準備ができている
    virtual void realize() { }

  protected:
    X_server();
    virtual bool dispach() = 0;
  };

  /// トップレベルのウィンドウを管理するクラス
  /*
    実装クラスはライブラリとして提供される。
    ユーザ・コードが継承して利用する。
  */
  class X_frame : public X_resoruce {
  protected:
    /// サーバの接続情報を記録
    X_server *server;
    /// 作業ウィンドウ
    Window window;
    /// 基本画像操作
    GC gc;

  public:
    X_frame(const char *app_name, const char *app_class);
    virtual ~X_frame() { }
    /// X_server は、このメソッドを利用してwindowを保持しているか診断する
    virtual bool contains(Window window);
    /// イベントを処理する
    virtual void dispatch(XEvent &event) { }
    /// ウインドウのタイトルを設定する
    virtual void set_title(const char *title);
  };
};


#include <algorithm>
#include <cerrno>
#include <clocale>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <csignal>
#include <deque>
#include <iostream>
#include <map>
#include <pwd.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

#include <X11/Xlocale.h>
#include <X11/Xutil.h>

using namespace std;

static int exit_flag = 0;

static void interrupt_handler(int sig) { exit_flag = 1; }

static void setup_interrupt_handler() {
  if (SIG_ERR == signal(SIGINT, interrupt_handler))
    perror("ERROR: sinale SIGINT");
}

/// ウィンドウだけ表示する簡単な例
static int simple_window(int argc,char **argv) {

  const char *display_name = "";
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

  cerr << "INFO: display closed." << endl;

  return 0;
}

// --------------------------------------------------------------------------------
// 先のコードをC++スタイルで記述したもの

namespace xwin {

  class X_server_XlibImpl : public X_server {
  public:
    X_server_XlibImpl();
    ~X_server_XlibImpl();
    void add_resoruce(X_resoruce *res);
    /// リソースの登録を解除する
    void remove_resoruce(X_resoruce *res);
    /// サーバに接続する
    bool connect(const char *display_name = "");
    /// サーバへの接続を解除する
    void disconnect();
    /// フレームを登録する
    void add_frame(X_frame *frame);
    /// フレームの登録を解除する
    void remove_frame(X_frame *frame);
    /// Xの接続時にAtomを設定するアドレスを定義
    void store_atom(const char *name, Atom *atom);

    bool dispatch();
  private:
    vector<X_frame *> frames;
    vector<X_resoruce *> resources;
    map<string,Atom *> atom_ref;
  };


  X_server::X_server() :
    ref(0), WM_PROTOCOLS(0), WM_DELETE_WINDOW(0) { }

  XrmDatabase X_server::get_resource_database() {
    return 0;
  }

  const char *X_server::get_atom_name(Atom atom) {
    if (!ref || !atom) return "";
    return XGetAtomName(ref, atom);
  }

  /// フォントの読み込み
  XFontSet X_server::load_font(const char *font_name) {
    int missing_count = 0;
    char **missing_list = NULL, *default_string;
    
    XFontSet font = 
      XCreateFontSet(ref, font_name,
		     &missing_list, &missing_count, &default_string );

    if (font == NULL) {
      cerr << "ERROR: failed to create fontset:" << font_name << endl;
      return font;
      /*
	XCreateFontSet は指示するフォントを読込む。
	ただし、サーバに代替フォントすら存在しなければ、NULLが返る。
	例えば日本語を表示させたいのに、日本語フォントを一切インストールしていない場合。
       */
    }

    if (missing_count) {
      /*
	指定したフォントが見つからず、代替フォントが採用された場合にここに処理が移る。
	ここでは代替フォント名をフィードバックしている。
      */
      cerr << "WARNING: font list missing: " << font_name << endl;
      for (int i = 0; i < missing_count; i++)
	cerr << "\t" << missing_list[i] << endl;
      
      cerr << "default string: " << default_string << endl;
      /*
	表示できない文字の代替文字をフィードバックしている。
      */
      XFreeStringList(missing_list);
      /*
	レポートで返されたリストは開放する必要がある。
       */
    }
    return font;
  }

  X_server_XlibImpl::X_server_XlibImpl() {
    store_atom("WM_PROTOCOLS", &WM_PROTOCOLS);
    store_atom("WM_DELETE_WINDOW", &WM_DELETE_WINDOW);
  }

  X_server_XlibImpl::~X_server_XlibImpl() { disconnect(); }

  void X_server_XlibImpl::store_atom(const char *name, Atom *atom) {
    if (!name || !atom) return;
    string atom_name(name);
    atom_ref.insert(map<string,Atom *>::value_type(atom_name, atom));
  }

  void X_server_XlibImpl::add_resoruce(X_resoruce *res) {
    if (!res) return;
    resources.push_back(res);
  }

  void X_server_XlibImpl::remove_resoruce(X_resoruce *res) {
    if (!res) return;
    vector<X_resoruce *>::iterator pos = find(resources.begin(), resources.end(), res);
    if (pos != resources.end()) resources.erase(pos);
  }
  
  void X_server_XlibImpl::add_frame(X_frame *frame) {
    resources.push_back(frame);
  }

  void X_server_XlibImpl::remove_frame(X_frame *frame) {
    if (!frame) return;
    vector<X_frame *>::iterator pos = find(frames.begin(), frames.end(), frame);
    if (pos != frames.end()) frames.erase(pos);
  }

  bool X_server_XlibImpl::connect(const char *display_name) {

    Display *disp = XOpenDisplay(display_name);
    /*
      Xサーバと接続する。一定時間経過しても接続できなければ NULLが返る。
      接続名が空テキストであれば、環境変数 $DISPLAYに設定されている値を利用する。
    */
    if (!disp) {
      cerr << "ERROR: can not connect xserver." << endl;
      return false;
    }
    if (ref) disconnect();
    ref = disp;

    // アトムの値の入手
    map<string,Atom *>::iterator it = atom_ref.begin();
    for (; it != atom_ref.end(); it++) {
      *it->second = XInternAtom(ref, it->first.c_str(), True);
    }

    {
      vector<X_frame *>::iterator it = frames.begin();
      for (; it != frames.end(); it++) { (*it)->realize(); }
    }

    return true;
  }

  void X_server_XlibImpl::disconnect() {
    if (!ref) return;
    {
      vector<X_resoruce *>::iterator it = resources.begin();
      for (; it != resources.end(); it++) { (*it)->dispose(); }
    }
    {
      vector<X_frame *>::iterator it = frames.begin();
      for (; it != frames.end(); it++) { (*it)->dispose(); }
    }

    XCloseDisplay(ref);
    cerr << "#disconnect called." << endl;
    /*
      サーバにリソースを開放のリクエストを送っている。
      プロセスが終了するなら、これらを呼び出さずとも自動的に開放される。
      このメソッドは、処理がここまで到達したことを認識できるように用意した。
    */
  }

  bool X_server_XlibImpl::dispatch() {
    while (!exit_flag || frames.empty()) return false;

    XEvent event;
    XNextEvent(ref, &event);

    if (XFilterEvent(&event,None)) return true;

    vector<X_frame *>::iterator it = frames.begin();
    for (; it != frames.end(); it++) {
      if (!(*it)->contains(event.xany.window)) continue;
      (*it)->dispatch(event);
    }

    return true;
  }

  // --------------------------------------------------------------------------------

  X_frame::X_frame(const char *app_name, const char *app_class)
    : server(0), window(0), gc(0) { }

  bool X_frame::contains(Window target) {
    return window == target;
  }

  void X_frame::set_title(const char *title) {
    char *ttitle = (char *)title;
    Display *const display = server->ref;
    XTextProperty prop;
    int rc = XmbTextListToTextProperty(display,&ttitle,1,XStdICCTextStyle,&prop);
    if (rc != 0)
      cerr << "WARNING: XmbTextListToTextProperty failed: " << rc <<endl;
    else {
      XSetWMName(display,window,&prop);
      XFree(prop.value);
    }
    cerr << "INFO: window title: " << title <<endl;
  }

  // --------------------------------------------------------------------------------

  class X_frame_XlibImpl : public X_frame {
  public:
    X_frame_XlibImpl(const char *app_name, const char *app_class);
    ~X_frame_XlibImpl();
    void realize();
    void dispose();
  };

  X_frame_XlibImpl::X_frame_XlibImpl(const char *app_name, const char *app_class)
    : X_frame(app_name, app_class) { }
    
  X_frame_XlibImpl::~X_frame_XlibImpl() { }
  
  void X_frame_XlibImpl::realize() {
    Display *const display = server->ref;
    const Window parent = DefaultRootWindow(display);
    const int screen = DefaultScreen(display);
    unsigned long background = WhitePixel(display, screen), 
      border_color = BlackPixel(display, screen);
    int x = 0, y = 0, width = 100, height = 100, border_width = 2;

    window = 
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

    gc = DefaultGC(display, screen);
  }

  void X_frame_XlibImpl::dispose() {
    if (!window) return;

    Display *const display = server->ref;
    if (gc && gc != DefaultGC(display, DefaultScreen(display)))
      XFreeGC(display, gc);
    XDestroyWindow(display, window);
    window = 0;
  }

};

namespace {

  /// Xアプリケーションの雛形
  struct xlib01 {
    /// Xサーバ接続情報
    Display *display;
    /// 作業ウィンドウ
    Window window;

    virtual ~xlib01() {}
    /// サーバに接続
    virtual bool connect_server(const char *display_name = "");
    /// アプリケーション・ウィンドウを作成
    virtual void create_application_window();
    /// イベントループ
    virtual void event_loop();
    /// リソースの開放
    virtual void dispose();
  };

  /// Xサーバに接続する
  bool 
  xlib01::connect_server(const char *display_name)
  {
    display = XOpenDisplay(display_name);
    /*
      Xサーバと接続する。一定時間経過しても接続できなければ NULLが返る。
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
    /*
      ウィンドウを作成する。
      これで指定していない、その他のウィンドウ属性は親から引き継ぐ。
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
  }

  /// イベントループ
  void 
  xlib01::event_loop()
  {
    XMapWindow(display, window);
    /*
      表示状態にする。
      このタイミングでウィンドウマネージャの介入が入る。
    */

    XEvent event;
    setup_interrupt_handler();

    while (!exit_flag) {
      XNextEvent(display, &event);
      
      switch (event.type) {
      case KeyPress: case ButtonPress:
	/*
	  キーが押下されるか、ポインティング・デバイス（マウス等）の
	  ボタンが押下されたら終了する
	*/
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
    cerr << "#dispose called." << endl;
    /*
      サーバにリソースを開放のリクエストを送っている。
      プロセスが終了するなら、これらを呼び出さずとも自動的に開放される。
      このメソッドは、処理がここまで到達したことを認識できるように用意した。
    */
  }

  /// C++スタイル で作成した Simple Window
  static int
  simple_window02(int argc,char **argv) {
    xlib01 *app = new xlib01;
  
    if (!app->connect_server()) return 1;
    app->create_application_window();
    app->event_loop();
    app->dispose();
    return 0;
  }
};

// --------------------------------------------------------------------------------

namespace {

  /// 行単位でテキストを操作するクラス
  class line_text {
    // 行単位でテキストを保持する
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
    /// グラフィックス描画のための様々な属性パラメータを保持する
    GC gc;
    /// 複数のフォントを取りまとめて操作するための情報を保持する
    XFontSet font;
    /// 表示するテキストを保持する
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
      /*
	設置されたロケールについて、Xlibがサポートできないものであると診断した。
	ここでは利用者にその旨をフィードバックしている。
       */
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
      /*
	XCreateFontSet は指示するフォントを読込む。
	ただし、サーバに代替フォントすら存在しなければ、NULLが返る。
	例えば日本語を表示させたいのに、日本語フォントを一切インストールしていない場合。
       */
    }

    if (missing_count) {
      /*
	指定したフォントが見つからず、代替フォントが採用された場合にここに処理が移る。
	ここでは代替フォント名をフィードバックしている。
      */
      cerr << "WARNING: font list missing: " << font_name << endl;
      for (int i = 0; i < missing_count; i++)
	cerr << "\t" << missing_list[i] << endl;
      
      cerr << "default string: " << default_string << endl;
      /*
	表示できない文字の代替文字をフィードバックしている。
      */
      XFreeStringList(missing_list);
      /*
	レポートで返されたリストは開放する必要がある。
       */
    }
    return font;
  }

  /// アプリケーションのウインドウを作成する
  void xlib02::create_application_window() {

    xlib01::create_application_window();

    XGCValues values;
    values.foreground = BlackPixel(display, DefaultScreen(display));
    gc = XCreateGC( display, window, GCForeground, &values);
    /*
      GCはWindowsの描画属性を元に作成される。
      それと異なる値としたい場合は、作成時に調整する。
      構造体のフィールドに値を設定して、
      扱うフィールドをビットマスクで指示する。
     */

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
  /**
    このメソッドは、ウィンドウの露出によって呼び出される。
    このタイミングで、保持しているテキストを描画する。
  */
  void xlib02::process_expose(XEvent *event) {
    if (event->xexpose.window != window) return;

    int n = 0, x = 0, y = 0;
    XRectangle overall_ink, overall_logical;

    size_t len;
    const wchar_t *ws;

    for (n = 0; (ws = data.get_text(n)); n++) {
      len = wcslen(ws);
      if (len == 0) { ws = L" "; len = 1; }
      /*
	 event->expose には、露出した領域の情報の詳細が格納されているが、
	 この実装はそれを意識せず、保持しているテキストをウィンドウ全体ついて描画している。
	 そのため、再描画効率はよくない。
      */
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

  /// テキストを表示するアプリケーション
  static int text_list(int argc,char **argv) {
    xlib02 *app = new xlib02;

    const char *text_file = getenv("TEXT");
    if (!text_file) text_file = __FILE__;
    /*
      いくつかのパラメータはこの $TEXTのように
      環境変数で調整できるようにしている。
    */

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

};

// --------------------------------------------------------------------------------

namespace {

  /// テキストを表示/入力するアプリケーション
  struct xlib03 : xlib02 {
    /// 入力メソッドの制御用
    XIM input_method;
    /// 入力コンテキストの制御用
    XIC input_context;
    /// ウィンドウ・マネージャとの通信用
    Atom WM_PROTOCOLS, WM_DELETE_WINDOW;
    /// Xリソース・データベースの照会用
    XrmDatabase rmdb;
    /// 取り込み桁位置を保持
    int data_insert_pos;
    
    xlib03() : input_method(0),input_context(0),rmdb(0),data_insert_pos(0) { }
    virtual ~xlib03() {}
    
    virtual bool create_input_context(const char *style = "root");
    virtual void set_window_title(const char *title);
    virtual void insert_text(const wchar_t *wbuf);
    
    virtual void set_application_name(char *name, char *class_name);
    virtual void create_application_window();
    virtual void event_loop();
    virtual void dispose();
  };

  /// ユーザのホームディレクトリを入手する
  string get_home_dir() {
    string res;
    struct passwd *pw;
    char *ptr;

    ptr = getenv("HOME");
    if (ptr) {
      res = ptr;
      return res;
    }

    ptr = getenv("USER");
    pw = ptr ? ::getpwnam(ptr) : ::getpwuid(getuid());
    if (pw) {
      res = pw->pw_dir;
      return res;
    }
    return res;
  }

#ifndef XAPPDEFAULTS_PATH
#define XAPPDEFAULTS_PATH "/usr/share/X11/app-defaults"
#endif

  /// アプリケーション名を設定し、入力サーバとの通信を確立する
  void xlib03::set_application_name(char *res_name, char *res_class) {

    const char *modifier = "";
    if (!XSetLocaleModifiers(modifier))
      cerr << "WARNING: can not set local modifiders." << endl;

    // ここからXリソース・データベースの検索処理
    XrmDatabase rdb, rdbbase;
    XrmInitialize();
  
    const char *appresdir = getenv("XAPPLRESDIR");
    if (!appresdir) appresdir = getenv("XUSERFILESEARCHPATH");
    if (!appresdir) appresdir = XAPPDEFAULTS_PATH;

    rdbbase = XrmGetStringDatabase(appresdir);

    string home_res = get_home_dir();
    char *server_db = XResourceManagerString(display);
    if (server_db)
      rdb = XrmGetStringDatabase(server_db);
    else {
      home_res += "/.Xdefaults";
      rdb = XrmGetFileDatabase(home_res.c_str());
    }
    if (rdb) {
      if (!rdbbase) rdbbase = rdb;
      else XrmMergeDatabases(rdb,&rdbbase);
    }
  
    home_res = get_home_dir();
    const char *environment = getenv("XENVIRONMENT");
    if (environment == NULL) {
      char hostname[100];
      gethostname(hostname, sizeof hostname);
      home_res += "/.Xdefaults-";
      home_res += hostname;
      environment = home_res.c_str();
    }
    rdb = XrmGetFileDatabase(environment);
    if (rdb) {
      if (!rdbbase) rdbbase = rdb;
      else XrmMergeDatabases(rdb,&rdbbase);
    }

    rmdb = rdbbase;
    if (!rmdb) res_name = res_class = NULL;

    // ここからXIMの接続

    input_method = XOpenIM(display, rmdb, res_name, res_class);
    if (!input_method) {
      cerr << "WARNING: cann not open input method." << endl;
      return;
    }
  
    XIMStyles *im_styles;

    XGetIMValues(input_method,
		 XNQueryInputStyle, &im_styles, NULL);
  
    struct { unsigned long style; const char *name; }
    stlist[] = {
      { XIMPreeditCallbacks, "preedit-callbacks", },
      { XIMPreeditPosition, "preedit-position", },
      { XIMPreeditArea, "preedit-area", },
      { XIMPreeditNothing, "preedit-nothing", },
      { XIMPreeditNone, "preedit-none", },
      { XIMStatusCallbacks, "status-callbacks", },
      { XIMStatusArea, "status-area", },
      { XIMStatusNothing, "status-nothing", },
      { XIMStatusNone, "status-none", },
    };

    /// ここから接続したXIMのサポート・レベルをフィードバックしている

    cerr << "INFO: supported im style below." << endl;

    string st;

    for (int i = 0; i < im_styles->count_styles; i++) {
      st = "\t";
      const char *sp = "";

      for (size_t j = 0 ; j < sizeof stlist/sizeof stlist[0]; j++) {
	if ((im_styles->supported_styles[i] & stlist[j].style) == stlist[j].style) {
	  st += sp;
	  st += stlist[j].name;
	  sp = ", ";
	}
      }
      cerr  << st << endl;      
    }
    cerr  << im_styles->count_styles << " support styles found." << endl;

    XFree(im_styles);
  }

  /// 入力コンテキストの作成
  bool xlib03::create_input_context(const char *style) {
    if (!input_method) return false;

    input_context = 
      XCreateIC( input_method,
		 XNInputStyle, (XIMPreeditNothing | XIMStatusNothing),
		 XNClientWindow, window, NULL);
    /*
      入力コンテキストは通常、トップ・レベル毎に作成する。
      作成できないからといって、アプリ全体としては動作しないようにはしない。
      （従来の英数字の入力はできるため）
    */
    
    if (!input_context) {
      cerr << "WARNING: cann not create input context.";
      return false;
    }
    return true;
  }

  /// ウィンドウ・マネージャのタイトルの設定
  void xlib03::set_window_title(const char *title) {
    XTextProperty prop;
    int rc = XmbTextListToTextProperty(display,(char **)&title,1,XStdICCTextStyle,&prop);
    if (rc != 0)
      cerr << "WARNING: XmbTextListToTextProperty failed: " << rc <<endl;
    else {
      XSetWMName(display,window,&prop);
      XFree(prop.value);
    }
    cerr << "INFO: window title: " << title <<endl;
  }

  void xlib03::create_application_window() {
    xlib02::create_application_window();

    XWMHints hints;
    hints.input = True;
    hints.flags = InputHint;

    XSetWMHints(display, window, &hints);
    /*
      キーボード入力を必要とすることをウィンドウ・マネージャにに伝えている
    */

    WM_PROTOCOLS = XInternAtom(display, "WM_PROTOCOLS", True );
    WM_DELETE_WINDOW = XInternAtom(display, "WM_DELETE_WINDOW", True );
    XSetWMProtocols(display, window, &WM_DELETE_WINDOW, 1 );
    /*
      ウィンドウ・マネージャに強制停止させられるのではなく、
      閉じるタイミングを通知してもらうよう要求している。
    */
    set_window_title("wlib03");

    unsigned long event_mask = 0;
    if (create_input_context()) {
      XGetICValues( input_context, XNFilterEvents, &event_mask, NULL );
      /*
	インプットメソッドが必要とするイベントマスクを入手する
      */
    }

    event_mask |= ButtonPressMask | ExposureMask | KeyPressMask | FocusChangeMask;
    XSelectInput( display, window, event_mask);
  }

  /// 入力したテキストをテキストバッファに登録する
  void xlib03::insert_text(const wchar_t *wbuf) {
    size_t len = wcslen(wbuf);

    data.replace_text(0, data_insert_pos, wbuf);
    data_insert_pos += len;

    if (wbuf[len - 1] == L'\n' || wbuf[len - 1] == L'\r') {
      data.insert_text(0, L"");
      data_insert_pos = 0;
    }
    /*
     * この実装は文書の先頭に差し込むだけの単純なもの。
     * 入力内容を確認するだけの振る舞い
     */

    unsigned width, height;
    width = height = 1000;
    XRectangle overall_ink, overall_logical;

    XwcTextExtents(font, wbuf, len, &overall_ink, &overall_logical);
    if (data_insert_pos != 0)
      height = overall_logical.height;
    XClearArea(display, window, 0, 0, width, height, True);
    /*
      入力された領域だけを消去している。
      消去されたら、イベント通知がある。
      そのタイミングでテキストの再描画を行う。
     */

    wcout << hex;
    while (*wbuf) {
      wcout << (int)*wbuf++ << ' ';
    }
    wcout << dec << endl;
  }

  /// Xlibのイベント処理
  void xlib03::event_loop() {
    XMapWindow(display, window);

    XEvent event;

    int wbuf_len = 128;
    wchar_t *wbuf = (wchar_t *)malloc(wbuf_len * sizeof (*wbuf));

    int buf_len = 64;
    char *buf = (char *)malloc(buf_len * sizeof (*buf));

    int len;
    KeySym keysym;
    Status status;
    XComposeStatus compose_status;

    while (!exit_flag) {
      XNextEvent(display, &event);
      if (XFilterEvent(&event,None)) continue;
      /*
	XIMを利用する場合にXFilterEventを呼び出す必要がある。
	日本語テキストの入力で、まだ入力が確定していない状態においては
	XIMのフロントエンドによりイベントが処理されるので、
	このイベントループでの処理は不要となるため、continue している。
       */

      switch (event.type) {
      case KeyPress: 
	if (!input_context) {
	  /*
	    XIMに接続できなかったときは、
	    従来からのローマ字だけの入力処理とする。
	    ワイド文字に変換して処理を継続する。
	   */
	  len = XLookupString(&event.xkey, buf, buf_len, &keysym, &compose_status);
	  size_t mblen = mbstowcs(wbuf, buf, wbuf_len);
	  if (mblen == (size_t)-1) {
	    cerr << "WARNING: invalid state mbstring(ignored)" << endl;
	    continue;
	  }
	  insert_text(wbuf);
	  continue;
	}

	len = 
	  XwcLookupString(input_context,&event.xkey,
			  wbuf,wbuf_len - 1,&keysym,&status);
	wbuf[len] = 0;
	/*
	  入力確定後のテキストはワイド文字の配列で入手できる。
	 */
	if (status == XBufferOverflow) {
	  /*
	    用意したバッファいに入りきらなかった場合は
	    必要な文字数を返却してくれているので、それで再取得する。
	   */
	  wchar_t *new_wbuf = (wchar_t *)realloc(wbuf, (len + 1) * sizeof (*wbuf));
	  if (new_wbuf) {
	    cerr << "INFO: wbuf expaned to " << (len + 1) << endl;
	    wbuf_len = len;
	    wbuf = new_wbuf;
	    XwcLookupString(input_context,&event.xkey,
			    wbuf,wbuf_len - 1,&keysym,&status);
	    wbuf[len] = 0;
	  }
	}

	// 入力されたテキストの種別による分岐
	switch(status) {
	case XLookupNone: break;
	case XLookupKeySym: case XLookupBoth:
	  // テキスト入力ではなく、操作系のキーが押下された。
	  if (keysym == XK_Delete) {
	    cerr << "delete" << endl;
	  }
	  else if (keysym == XK_BackSpace) {
	    cerr << "back space" << endl;
	  }
	  else if (keysym == XK_Insert && event.xkey.state & ShiftMask) {
	    cerr << "try to get selection text" << endl;
	  }
	  if (status == XLookupKeySym) break;
	  
	case XLookupChars: 
	  // 確定入力テキスト
	  insert_text(wbuf);
	  break;
	default:
	  cerr << "unkown status" << status << endl;
	}

      case ButtonPress:
	break;
      case Expose:
	process_expose(&event);
	break;
      case FocusIn :
	if (input_context) 
	  XSetICFocus(input_context);
	break;
      case FocusOut :
	if (input_context) 
	  XUnsetICFocus(input_context);
	break;
      case MappingNotify:
	XRefreshKeyboardMapping(&event.xmapping);
	break;
      case ClientMessage:
	if (event.xclient.message_type == WM_PROTOCOLS &&
	    (Atom)event.xclient.data.l[0] == WM_DELETE_WINDOW ) {
	  cerr << "INFO: delete window accepted." << endl;
	  exit_flag = 1;
	}
      }
    }

    free(wbuf);
    free(buf);
  }

  void xlib03::dispose() {
    if (input_context) XDestroyIC(input_context);
    if (input_method) XCloseIM(input_method);
    XrmDestroyDatabase(rmdb);
    xlib02::dispose();
  }

  /// テキストを表示するアプリケーション
  static int text_inputs(int argc,char **argv) {
    xlib03 *app = new xlib03;

    const char *text_file = getenv("TEXT");
    if (!text_file) text_file = __FILE__;

    const char *font_name = getenv("FONT_NAME");
    if (!font_name) font_name = "-*--14-*,-*--24-*";

    if (!app->locale_initialize()) return 1;
    if (!app->load_display_text(text_file)) return 1;

    if (!app->connect_server()) return 1;
    if (!(app->font = app->load_font(font_name))) return 1;
  
    char *res_name = basename(argv[0]), *res_class = "Xlib03";
    app->set_application_name(res_name, res_class);

    app->create_application_window();
    app->event_loop();
    app->dispose();
    return 0;
  }
};

// --------------------------------------------------------------------------------

#ifdef USE_SUBCMD

#include "subcmd.h"

subcmd xwin_cmap[] = {
  { "win", simple_window02, },
  { "win01", simple_window, },
  { "win02", simple_window02, },
  { "text", text_inputs, },
  { "text01", text_list, },
  { "text02", text_inputs, },
  { 0 },
};

#endif
