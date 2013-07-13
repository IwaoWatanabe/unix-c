#ifndef uc_xt_proc_h
#define uc_xt_proc_h

#include <X11/Intrinsic.h>

#ifdef __cplusplus
extern "C" {
#endif

extern const char *getXtClassName(WidgetClass klass);
extern WidgetClass getXtSuperClass(WidgetClass klass);
extern String getXtName(Widget widget);

extern WidgetList getXtChildren(Widget widget);
extern Cardinal getXtChildrenCount(Widget widget);

extern int app_shell_count;

extern Boolean dispose_work_proc(XtPointer closure);
extern Widget find_shell(Widget t, Boolean need_top_level);
extern void dispose_shell(Widget w);

#ifdef __cplusplus
extern void show_component_tree(Widget target, int level = 2);
#else
extern void show_component_tree(Widget target, int level);
#endif

#ifdef __cplusplus
};


/// Xで動作するGUIアプリケーション作成をサポートする

namespace xwin {

  /// 基本メニュー項目を定義する構造体
  struct Menu_Item {
    /// メニュー名
    String name;
    /// メニュー固有データへのポインタ
    XtCallbackProc proc;
    /// メニュー管理主体（通常は Frame）へのポインタ
    XtPointer closure;
    /// サブメニュー項目
    struct Menu_Item *sub_menu;
     /// メニューの簡易説明テキスト
    const char *description;
  };

  struct Client_Context;

  /// GUIユーザ・コードが実装するインタフェース
  struct Frame {
    virtual ~Frame() = 0;
    /// コンテナ・リソースを参照するためのコンテキストが渡されてくる
    virtual void set_Client_Context(struct Client_Context *context) { }
    /// アプリケーションのタイトルを返す
    virtual std::string get_title() { return ""; }
    /// 実装クラスはこのタイミングでコンテンツを作る
    virtual Widget create_contents(Widget parent) { return 0; }
    /// 基本メニュー・アイテムを返す
    virtual Menu_Item *get_menu_itmes() { return 0; }
    /// リソースを開放する要求で呼び出される
    virtual void release() { }
  };

  /// Frameインスタンスを生成するクラス
  struct Frame_Factory {
    Frame_Factory();
    virtual ~Frame_Factory() { }
    /// フレームのインスタンスを入手する
    virtual Frame *get_instance() = 0;
    /// アプリケーション・クラス名を入手する
    virtual const char *get_class_name() { return 0; }
    /// バージョン情報を入手する
    virtual const char *get_version() { return "0.1"; }
  };

  /// Xサーバに関連するリソースを保持、管理する
  /**
     アトム、フォント、XIM関連を操作することになる
   */
  class Server_Resource {
  protected:
    /// このリソースを利用するウィジェット
    Widget owner;
  public:
    // 良く利用するアトム
    Atom WM_PROTOCOLS      /// WMとの通信用
      ,WM_DELETE_WINDOW  /// トップレベルウインドウの破棄
      ,COMPOUND_TEXT;   ///< 複合エンコーディングテキスト

    Server_Resource(Widget owner);

    virtual ~Server_Resource() = 0;
    /// アトム値を入手する
    virtual Atom atom_value_of(const char *name) = 0;
    /// アトム・テキストを入手する
    virtual std::string text_of(Atom atom) = 0;
    /// 同じサーバを参照するリソースを探す
    static Server_Resource *find_Server_Resource(Widget shell);
  };

  /// コンテナのリソースにアクセスするためのインタフェース
  struct Client_Context {
    virtual ~Client_Context() = 0;
    /// サーバ情報を入手する
    virtual Server_Resource *get_Server_Resource() = 0;
    /// フレームを閉じる操作のためのメニュー項目を返す
    virtual const Menu_Item *get_close_item() = 0;
    /// フレームを探す。無ければ作成する
    virtual Frame *find_Frame(const char *name, const char *frame_class) = 0;
    /// ダイアログを探す。無ければ作成する
    virtual Frame *find_Dialog(const char *name, const char *frame_class) = 0;
  };

};

#endif

#endif
