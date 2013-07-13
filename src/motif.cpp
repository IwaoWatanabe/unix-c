/*! \File
 * \brief Motifを使ったサンプルコード
 */

#include <iostream>
#include <cstring>
#include <cctype>
#include <string>
#include <time.h>
#include <vector>

#include <X11/Intrinsic.h>
#include <X11/Xmu/Editres.h>

#include "xt-proc.h"
#include "uc/elog.hpp"

#include <Mrm/MrmPublic.h>

#include <Xm/CascadeB.h>
#include <Xm/Command.h>
#include <Xm/Container.h>
#include <Xm/IconG.h>
#include <Xm/DialogS.h>
#include <Xm/FileSB.h>
#include <Xm/List.h>
#include <Xm/MainW.h>
#include <Xm/MessageB.h>
#include <Xm/PushB.h>
#include <Xm/PushBG.h>
#include <Xm/RowColumn.h>
#include <Xm/Separator.h>
#include <Xm/SelectioB.h>
#include <Xm/ScrolledW.h>
#include <Xm/Text.h>

using namespace std;
using namespace xwin;

extern wstring load_wtext(const char *path, const char *encoding = "UTF-8", time_t *lastmod = 0, size_t buf_len = 4096);

extern "C" {
  extern char *trim(char *t);
  extern char *demangle(const char *demangle);
};

namespace xwin {
  extern bool load_dirent(const char *dirpath, vector<string> &entries, bool with_hidden_file = false);

  /// 一連のファクトリを記録する
  extern vector<Frame_Factory *> frame_factories;
};

// --------------------------------------------------------------------------------

namespace {

  /// トップレベル・シェルの破棄する
  static void quit_application(Widget widget, XtPointer client_data, XtPointer call_data) {
    dispose_shell(widget);
    cerr << "TRACE: quit application called." << endl;
  }


  /// ボタンを表示するだけのフレーム
  class Hello_Frame : public Frame {
  public:
    Widget create_contents(Widget shell);
  };

  class Hello_Frame_Factory : public Frame_Factory {
    Frame *get_instance() { return new Hello_Frame(); }
  };

  Widget Hello_Frame::create_contents(Widget top) {
    Widget pb = XmVaCreateManagedPushButton(top,"msg",NULL);
#if 0
    XtVaSetValues(pb, XmNlabelString, XmStringCreateLocalized("日本語テキスト"), NULL);
#endif
    XtAddCallback(pb, XmNactivateCallback, quit_application, NULL);
    return 0;
  }

};


// --------------------------------------------------------------------------------

namespace {

  /// ポップアップ・メニューの作成(motif)
  static Widget create_pulldown_menu(String menu_name, Widget parent, Menu_Item *items) {
    Widget menu = XmCreatePulldownMenu(parent,menu_name,0,0);

    for (int i = 0; items[i].name; i++) {
      if (strcmp("-", items[i].name) == 0) {
	XtManageChild(XmCreateSeparator(menu,"-", 0,0));
	continue;
      }
      Widget item = XmCreatePushButton(menu,items[i].name,0,0);
      XtManageChild(item);
      XtAddCallback(item, XmNactivateCallback, items[i].proc, items[i].closure);
    }
    return menu;
  }

  /// Xm文字列のテーブルに変換
  static XmStringTable asXmStringTable(vector<string> &entries) {
    size_t n = entries.size();
    XmString *tbl = (XmString *)XtMalloc (n * sizeof (XmString));
    for (size_t i = 0; i < n; i++)
      tbl[i] = XmStringCreateLocalized((char *)entries[i].c_str());
    return tbl;
  }

  /// Xm文字列のテーブルを開放
  static void freeXmStringTable(XmStringTable tbl, size_t n) {
    XmString *str = tbl;
    for (size_t i = 0; i < n; i++)
      XmStringFree(str[n]);
    XtFree ((char *)tbl); 
  }

  /// フォルダ一覧を表示するリスト
  static void create_folder_list(Widget top) {
    vector<string> entries;

    xwin::load_dirent(".", entries, false);

    Widget main =
      XmVaCreateManagedMainWindow(top,"list-main", NULL);

    Widget bar =
      XmCreateMenuBar(main,"bar",0,0);
    XtManageChild(bar);

    struct Menu_Item dir_items[] = {
      { "new", quit_application, },
      { "-", },
      { "close", quit_application, },
      { 0, },
    };

    XmVaCreateManagedCascadeButton(bar, "dir",
				   XmNsubMenuId, create_pulldown_menu("dir", bar, dir_items),
				   NULL);

    XmStringTable tbl = asXmStringTable(entries);

    Widget list = XmCreateScrolledList( main, "file-list", 0, 0);
    XtVaSetValues(main, XmNworkWindow, XtParent(list), NULL);
    XtVaSetValues(list,
		  XmNitems, tbl, 
		  XmNitemCount, entries.size(), 
		  XmNvisibleItemCount, 20, 
		  XmNselectionPolicy, XmEXTENDED_SELECT,
		  NULL);

    XtManageChild(list);

    //freeXmStringTable(tbl, n_entries);
    XtFree((char *)tbl);
    /*
     * リストの場合、アイテムを複製していないようで
     * 開放するとSEGVになる
     */
    XtRealizeWidget(top);
  }

  /* reset_field(): used to reset the field context
  ** when a newline is encountered in the input
  */
  XmIncludeStatus 
  reset_field (XtPointer *in_out, XtPointer text_end, XmTextType text_type,
	       XmStringTag locale_tag, XmParseMapping parse_mapping,
	       int pattern_length, XmString *str_include, XtPointer call_data) {
    /* client data is a pointer to the field counter */
    int *field_counter = (int *)call_data;
    /* A newline encountered.
     *
     * Trivial: we reset the field counter for this line
     * and terminate the parse sequence 
     */
    char *cptr = (char *)*in_out;
    cptr++;
    
    *in_out = (XtPointer)cptr;
    *field_counter = 0;
    return XmTERMINATE;
  }

  /* filter_field(): throws away the second (password) field
   * and maps colon characters to tab components.
   */
  XmIncludeStatus 
  filter_field ( XtPointer *in_out, XtPointer text_end, XmTextType text_type,
		 XmStringTag locale_tag, XmParseMapping parse_mapping,
		 int pattern_length, XmString *str_include, XtPointer call_data) {
    /* client data is a pointer to the field counter */
    int *field_counter = (int *)call_data;
    char *cptr = (char *) *in_out;
  
    /* Skip this colon */
    cptr += pattern_length;
  
    /* If this is the first colon
     * then skip the input until after the second.
     * Otherwise, we return a TAB component
     */

    if (*field_counter == 0) {
      /* Skip over the next colon */
      while (*cptr != ':') cptr++;
      cptr++;
    }
  
    *str_include = 
      XmStringComponentCreate(XmSTRING_COMPONENT_TAB, 0, NULL);
    *in_out = (XtPointer) cptr;
    *field_counter = *field_counter + 1;
    return XmINSERT;
  }

  XmString *load_filtered_passwd (char *passwdfile, int *count) {
    XmParseMapping map[2];
    FILE *fptr;
    char buffer[256];
    Arg args[8];
    char *cptr;
    XmString *xms_array = (XmString *)0;
    int xms_count = 0;
    /* Used as client data to the XmParseProc routines */
    int field_count = 0;
    int n;
    *count = 0;
    if ((fptr = fopen(passwdfile, "r")) == NULL) {
      return NULL;
    }

    /* Set up an XmParseProc to handle colons */
    n = 0;
    XtSetArg (args[n], XmNpattern, ":"); n++;
    XtSetArg (args[n], XmNpatternType, XmCHARSET_TEXT); n++;
    XtSetArg (args[n], XmNincludeStatus, XmINVOKE); n++;
    XtSetArg (args[n], XmNinvokeParseProc, filter_field); n++;
    map[0] = XmParseMappingCreate(args, n);

    /* Set up an XmParseProc to handle newlines */
    n = 0;
    XtSetArg (args[n], XmNpattern, "\n"); n++;
    XtSetArg (args[n], XmNpatternType, XmCHARSET_TEXT); n++;
    XtSetArg (args[n], XmNincludeStatus, XmINVOKE); n++;
    XtSetArg (args[n], XmNinvokeParseProc, reset_field); n++;
    map[1] = XmParseMappingCreate(args, n);

    while ((cptr = fgets(buffer, sizeof buffer, fptr)) != NULL) {
      xms_array = (XmString *)
	XtRealloc((char *) xms_array, (xms_count + 1) * sizeof (XmString));
      xms_array[xms_count] = 
	XmStringParseText (cptr, NULL, NULL, XmCHARSET_TEXT,
			 map, 2, &field_count);
      xms_count++;
    }

    fclose(fptr);

    XmParseMappingFree(map[0]);
    XmParseMappingFree(map[1]);

    *count = xms_count;
    return xms_array;
  }


  static void create_passwd_list(Widget top) {
    Widget main =
      XmVaCreateManagedMainWindow(top,"passwd-main", NULL);

    Widget bar =
      XmCreateMenuBar(main,"bar",0,0);
    XtManageChild(bar);

    struct Menu_Item dir_items[] = {
      { "close", quit_application, },
      { 0, },
    };

    XmVaCreateManagedCascadeButton(bar, "dir",
				   XmNsubMenuId, create_pulldown_menu("dir", bar, dir_items),
				   NULL);

    char *passwdfile = "/etc/passwd";
    int tbl_count= 0;
    XmStringTable tbl = load_filtered_passwd(passwdfile, &tbl_count);
  
    XmRendition renditions[] = { 
      XmRenditionCreate (top, "tab-render", 0, 0),
    };

    XmRenderTable rendertable = 
      XmRenderTableAddRenditions (NULL, renditions, XtNumber(renditions), XmMERGE_NEW);

    Widget list = XmCreateScrolledList( main, "passwd-list", 0, 0);
    XtVaSetValues(main, XmNworkWindow, XtParent(list), NULL);
    XtVaSetValues(list,
		  XmNitems, tbl, 
		  XmNitemCount, tbl_count, 
		  XmNvisibleItemCount, 20, 
		  XmNselectionPolicy, XmEXTENDED_SELECT,
		  XmNrenderTable, rendertable,
		  NULL);

    XtManageChild(list);

    //freeXmStringTable(tbl, n_entries);
    XtFree((char *)tbl);
    /*
     * アイテムを開放するとSEGVになる
     */

    if (1) {
      /*
       * レンダをウィジェットに登録したら複製されるので開放する
       */
      for (int i = 0; i < XtNumber (renditions); i++) XmRenditionFree (renditions[i]);
      XmRenderTableFree(rendertable);
    }

    XtRealizeWidget(top);
  }

  /// Motif の各種部品の振る舞いを確認する
  class Parts_Frame : public Frame {
    Widget info, warn, err;
    Widget prompt, command, selection;
    Widget tree;

    static void info_menu_selected(Widget widget, XtPointer client_data, XtPointer call_data);
    static void warn_menu_selected(Widget widget, XtPointer client_data, XtPointer call_data);
    static void err_menu_selected(Widget widget, XtPointer client_data, XtPointer call_data);
    static void prompt_menu_selected(Widget widget, XtPointer client_data, XtPointer call_data);
    static void command_menu_selected(Widget widget, XtPointer client_data, XtPointer call_data);
    static void selection_menu_selected(Widget widget, XtPointer client_data, XtPointer call_data);

    static void tree_menu_selected(Widget widget, XtPointer client_data, XtPointer call_data);
    static void create_tree_test(Widget shell);

  public:
    Widget create_contents(Widget shell);
  };

  class Parts_Frame_Factory : public Frame_Factory {
    Frame *get_instance() { return new Parts_Frame(); }
  };

  /// メニューアイテムを選択した時の処理
  static void menu_selected(Widget widget, XtPointer client_data, XtPointer call_data) {
    cerr << "TRACE: " << XtName(widget) << " selected." << endl;
  }
  
  static void list_menu_selected(Widget widget, XtPointer client_data, XtPointer call_data) {
    cerr << "TRACE: " << XtName(widget) << " selected." << endl;
    String app_class = "FileList";
    Widget top = XtVaAppCreateShell(NULL, app_class, applicationShellWidgetClass, XtDisplay(XtParent(widget)), NULL);
    app_shell_count++;
    create_folder_list(top);
  }

  static void passwdlist_menu_selected(Widget widget, XtPointer client_data, XtPointer call_data) {
    cerr << "TRACE: " << XtName(widget) << " selected." << endl;
    String app_class = "PasswdList";
    Widget top = XtVaAppCreateShell(NULL, app_class, applicationShellWidgetClass, XtDisplay(XtParent(widget)), NULL);
    app_shell_count++;
    create_passwd_list(top);
  }

  static void parts_menu_selected(Widget widget, XtPointer client_data, XtPointer call_data) {
    cerr << "TRACE: " << XtName(widget) << " selected." << endl;
  }

  /// ボタン押下の通知
  static void button_selected(Widget widget, XtPointer client_data, XtPointer call_data) {
    cerr << "TRACE: " << XtName(widget) << " selected." << endl;
    char *label = (char *)client_data;
    cerr << "TRACE: press button:" << label << endl;
  }

  /// 情報メッセージの出力
  void Parts_Frame::info_menu_selected(Widget widget, XtPointer client_data, XtPointer call_data) {
    cerr << "TRACE: " << XtName(widget) << " selected." << endl;
    Parts_Frame *app = (Parts_Frame *)client_data;

    if (!app->info) {
      Widget shell = find_shell(widget, 0);
      Widget info = XmCreateInformationDialog(shell,"info-dialog",0,0);
      XtUnmanageChild(XmMessageBoxGetChild(info,XmDIALOG_HELP_BUTTON));
      XtUnmanageChild(XmMessageBoxGetChild(info,XmDIALOG_CANCEL_BUTTON));
      Widget dialog = XtParent(info);
      show_component_tree(dialog);
      app->info = info;
    }

    XmString msg = XmStringCreateLocalized("information messsage.\n");
    XtVaSetValues(app->info, XmNmessageString, msg, NULL);
    XmStringFree(msg);
    XtManageChild(app->info);
  }

  /// 警告メッセージの出力
  void Parts_Frame::warn_menu_selected(Widget widget, XtPointer client_data, XtPointer call_data) {
    cerr << "TRACE: " << XtName(widget) << " selected." << endl;
    Parts_Frame *app = (Parts_Frame *)client_data;

    if (!app->warn) {
      Widget shell = find_shell(widget, 0);
      Widget warn = XmCreateWarningDialog(shell,"warn-dialog",0,0);

      XtUnmanageChild(XmMessageBoxGetChild(warn,XmDIALOG_HELP_BUTTON));
    
      XmString no = XmStringCreateLocalized("No");

      XtVaSetValues(warn, 
		    XmNdefaultButtonType, XmDIALOG_CANCEL_BUTTON, 
		    XmNcancelLabelString, no,
		    XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL,
		    NULL);
      /*
       * CANCELを[No]の表示にして、デフォルトボタンにする
       */
      XmStringFree(no);
      XtAddCallback (warn, XmNokCallback, button_selected, (XtPointer)"ok");
      XtAddCallback (warn, XmNcancelCallback, button_selected, (XtPointer)"no");

      show_component_tree(XtParent(warn));
      app->warn = warn;
    }

    XmString msg = XmStringCreateLocalized("warning messsage.\n");
    XtVaSetValues(app->warn, XmNmessageString, msg, NULL);
    XmStringFree(msg);
    XtManageChild(app->warn);
  }

  // エラー・メッセージの出力
  void Parts_Frame::err_menu_selected(Widget widget, XtPointer client_data, XtPointer call_data) {
    cerr << "TRACE: " << XtName(widget) << " selected." << endl;
    Parts_Frame *app = (Parts_Frame *)client_data;

    if (!app->err) {
      Widget shell = find_shell(widget, 0);
      Widget err = XmCreateErrorDialog(shell,"err-dialog",0,0);

      XtVaSetValues(err, 
		    XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL,
		    NULL);

      XtUnmanageChild(XmMessageBoxGetChild(err,XmDIALOG_HELP_BUTTON));
      XtAddCallback (err, XmNokCallback, button_selected, (XtPointer)"ok");
      XtAddCallback (err, XmNcancelCallback, button_selected, (XtPointer)"no");
      show_component_tree(XtParent(err));
      app->err = err;
    }

    XmString msg = XmStringCreateLocalized("error messsage.\n");
    XtVaSetValues(app->err, XmNmessageString, msg, NULL);
    XmStringFree(msg);
    XtManageChild(app->err);
  }

  /// プロンプト・ダイアログでの入力通知
  static void prompt_inputs_cb(Widget widget, XtPointer client_data, XtPointer call_data) {
    cerr << "TRACE: " << XtName(widget) << " inputs callback." << endl;
    XmSelectionBoxCallbackStruct *cbs = (XmSelectionBoxCallbackStruct *)call_data;

    char *value = (char *)XmStringUnparse(cbs->value, NULL, XmMULTIBYTE_TEXT, XmMULTIBYTE_TEXT, NULL, 0, XmOUTPUT_ALL);
    /// マルチバイトで取り出す
    cerr << "INFO: " << value << endl;
    XtFree(value);

    wchar_t *wcs = (wchar_t *)XmStringUnparse(cbs->value, NULL, XmWIDECHAR_TEXT, XmWIDECHAR_TEXT, NULL, 0, XmOUTPUT_ALL);
    /// ワイドキャラクタで取り出す
    wcerr << "INFO: " << wcs << "(wcs)" << endl;
    fprintf(stderr, "INFO: %ls(wcs fprintf)\n", wcs);

    XtFree((char *)wcs);
  }

  /// プロンプト・ダイアログ表示機能の選択
  void Parts_Frame::prompt_menu_selected(Widget widget, XtPointer client_data, XtPointer call_data) {
    cerr << "TRACE: " << XtName(widget) << " selected." << endl;
    Parts_Frame *app = (Parts_Frame *)client_data;

    if (!app->prompt) {
      Widget shell = find_shell(widget, 0);
      Widget prompt = XmCreatePromptDialog(shell,"prompt-dialog",0,0);
#if 1
      XtAddCallback(prompt, XmNokCallback, prompt_inputs_cb, (XtPointer)app);
      XtAddCallback(prompt, XmNokCallback, button_selected, (XtPointer)"ok");
      XtAddCallback(prompt, XmNcancelCallback, button_selected, (XtPointer)"cancel");
      XtUnmanageChild(XtNameToWidget(prompt, "Help"));
      /*
       * 「Help」ボタンの表示をやめるための操作
       */
      
      Widget dialog = XtParent(prompt);
      show_component_tree(dialog);
#endif
      app->prompt = prompt;
    }

    XmString msg = XmStringCreateLocalized("Enter Text:");
    XtVaSetValues(app->prompt, XmNselectionLabelString, msg, NULL);
    XmStringFree(msg);
    /*
     * プロンプト・テキストの設定
     */

    XtManageChild(app->prompt);
    /*
     * マネージするとポップアップする
     */
  }

  /// Xm文字列のテーブルに変換
  XmStringTable asXmStringTable(char **item, size_t n) {
    XmString *tbl = (XmString *)XtMalloc (n * sizeof (XmString));
    for (size_t i = 0; i < n; i++)
      tbl[i] = XmStringCreateLocalized(item[i]);
    return tbl;
  }

  /// コマンドの入力通知
  static void command_entered_cb(Widget widget, XtPointer client_data, XtPointer call_data) {
    cerr << "TRACE: " << XtName(widget) << " command callback." << endl;
    XmCommandCallbackStruct *cbs = (XmCommandCallbackStruct *)call_data;

    switch (cbs->reason) {
    case XmCR_COMMAND_ENTERED:
      break;
    case XmCR_COMMAND_CHANGED:
      cerr << "TRACE: CHANGED" << endl;
    }

    wchar_t *wcs = (wchar_t *)XmStringUnparse(cbs->value, NULL, XmWIDECHAR_TEXT, XmWIDECHAR_TEXT, NULL, 0, XmOUTPUT_ALL);
    /// ワイドキャラクタで取り出す
    wcerr << "INFO: " << wcs << "(wcs)" << endl;
    fprintf(stderr, "INFO: %ls(wcs fprintf)\n", wcs);
    XtFree((char *)wcs);
  }

  /// コマンド・ダイアログ表示機能の選択
  void Parts_Frame::command_menu_selected(Widget widget, XtPointer client_data, XtPointer call_data) {
    cerr << "TRACE: " << XtName(widget) << " selected." << endl;
    Parts_Frame *app = (Parts_Frame *)client_data;

    if (!app->command) {
      Widget shell = find_shell(widget, 0);
      Widget cmd = XmCreateCommandDialog(shell,"command-dialog",0,0);

      static char *days[] = {
	"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

      XmStringTable tbl = asXmStringTable(days, XtNumber(days));

      XmString prompt = XmStringCreateLocalized("Command");

      XtVaSetValues(cmd, 
		    XmNhistoryItems, tbl, 
		    XmNhistoryItemCount, XtNumber(days), 
		    XmNpromptString, prompt,
		    NULL);
      XmStringFree(prompt);
      XtAddCallback(cmd, XmNcommandEnteredCallback , command_entered_cb, 0);
      XtAddCallback(cmd, XmNcommandChangedCallback , command_entered_cb, 0);
      app->command = cmd;
    }

    XtManageChild(app->command);
  }

  static void selection_cb(Widget widget, XtPointer client_data, XtPointer call_data) {
    cerr << "TRACE: " << XtName(widget) << " selected." << endl;
    XmSelectionBoxCallbackStruct *cbs =
      (XmSelectionBoxCallbackStruct *)call_data;
    switch (cbs->reason) {
    case XmCR_OK:
      break;
    case XmCR_NO_MATCH:
      break;
    }

    wchar_t *wcs = (wchar_t *)XmStringUnparse(cbs->value, NULL, XmWIDECHAR_TEXT, XmWIDECHAR_TEXT, NULL, 0, XmOUTPUT_ALL);
    wcerr << "INFO: " << wcs << "(wcs)" << endl;
    fprintf(stderr, "INFO: %ls(wcs fprintf)\n", wcs);

    XtFree((char *)wcs);
    /// ワイドキャラクタで取り出す
  }

  void Parts_Frame::selection_menu_selected(Widget widget, XtPointer client_data, XtPointer call_data) {
    cerr << "TRACE: " << XtName(widget) << " selected." << endl;
    Parts_Frame *app = (Parts_Frame *)client_data;

    static char *months[] = {
      "January", "February", "March", "April", "May", "June", 
      "July", "August", "September", "October", "November", "December"};

    struct ListItem {
      char *label, **strings;
      size_t size;
    };

    ListItem month_items = {"Months", months, XtNumber(months), }, *items = &month_items;

    if (!app->selection) {
      Widget shell = find_shell(widget, 0);
      Widget selection = XmCreateSelectionDialog(shell, "selection", NULL, 0);
      XmString label = XmStringCreateLocalized(items->label);
      XmStringTable tbl = asXmStringTable(items->strings, items->size);

      XtVaSetValues (selection, 
		     XmNlistLabelString, label, 
		     XmNlistItems, tbl,
		     XmNlistItemCount, items->size,
		     XmNmustMatch, True,
		     NULL);
      XtAddCallback (selection, XmNokCallback, selection_cb, NULL);
      XtAddCallback (selection, XmNnoMatchCallback, selection_cb, NULL);
      XmStringFree(label);
      freeXmStringTable(tbl, items->size);
      app->selection = selection;
    }

    XtManageChild(app->selection);
  }

  /// ツリー表示のサンプル
  void Parts_Frame::create_tree_test(Widget shell) {

    Widget main, tree;
    main = XtVaCreateManagedWidget("main", xmMainWindowWidgetClass, shell, NULL);

    Widget bar =
      XmCreateMenuBar(main,"bar",0,0);

    struct Menu_Item tree_items[] = {
      { "close", quit_application, },
      { 0, },
    };

    XmVaCreateManagedCascadeButton(bar, "tree",
				   XmNsubMenuId, create_pulldown_menu("tree", bar, tree_items),
				   NULL);
    XtManageChild(bar);

    Arg args[8]; int n = 0;
    XtSetArg(args[n], XmNscrollingPolicy, XmAUTOMATIC); n++;
    XtSetArg(args[n], XmNscrollBarDisplayPolicy, XmAS_NEEDED); n++;
    
    Widget scroll = 
      XmCreateScrolledWindow(main, "scroll-win", args, n);
    XtManageChild(scroll);
    XtVaSetValues(main, XmNworkWindow, scroll, NULL);
  
    tree = XtVaCreateManagedWidget("tree",
				   xmContainerWidgetClass, scroll,
				   XmNlayoutType, XmOUTLINE,
				   XmNautomaticSelection, XmAUTO_SELECT,
				   XmNentryViewType, XmSMALL_ICON,
				   NULL);
    XtVaSetValues(scroll, XmNworkWindow, tree, NULL);
  
    char node_name[128];
    Widget level1, level2, level3;

    for(int i=0; i<2; i++) {
      sprintf(node_name, "Node_%d", i+1);
      level1 = 
	XtVaCreateManagedWidget(node_name,
				xmIconGadgetClass, tree,
				XmNoutlineState, XmEXPANDED,
				XmNshadowThickness, 0,
				NULL);

      for(int j=0; j<2; j++) {
	sprintf(node_name, "Node_%d_%d", j+1, i+1);
	level2 = 
	  XtVaCreateManagedWidget(node_name,
				  xmIconGadgetClass, tree,
				  XmNentryParent, level1,
				  XmNoutlineState, XmEXPANDED,
				  XmNshadowThickness, 0,
				  NULL);

	for(int k=0; k<3; k++) {
	  sprintf(node_name, "Node_%d_%d_%d", k+1, j+1, i+1);
	  level3 = 
	    XtVaCreateManagedWidget(node_name,
				    xmIconGadgetClass, tree,
				    XmNentryParent, level2,
				    XmNshadowThickness, 0,
				    NULL);
	}
      }
    }

    XtRealizeWidget(shell);
  }

  void Parts_Frame::tree_menu_selected(Widget widget, XtPointer client_data, XtPointer call_data) {
    cerr << "TRACE: " << XtName(widget) << " selected." << endl;
    String app_class = "Tree";
    Widget top = XtVaAppCreateShell(NULL, app_class, applicationShellWidgetClass, 
				    XtDisplay(XtParent(widget)), NULL);
    app_shell_count++;
    create_tree_test(top);
  }

  /// パーツの振る舞いの確認
  Widget Parts_Frame::create_contents(Widget shell) {

    Widget row_column = XmVaCreateManagedRowColumn(shell,"row-column", NULL);

    struct Menu_Item items[] = {
      { "none", menu_selected, },
      { "tree", tree_menu_selected, },
      { "list", list_menu_selected, },
      { "password-list", passwdlist_menu_selected, },
      { "info", info_menu_selected, this, },
      { "warning", warn_menu_selected, this, },
      { "error", err_menu_selected, this, },

      { "prompt", prompt_menu_selected, this, },
      { "command", command_menu_selected, this, },
      { "selection", selection_menu_selected, this, },
      { "quit", quit_application, },
    };

    for(int i = 0; i < XtNumber(items); i++) {
      Widget item = 
	XmVaCreateManagedPushButtonGadget(row_column, items[i].name, NULL);
      XtAddCallback(item, XmNactivateCallback, items[i].proc, items[i].closure);
    }

    return 0;
  }

};

// --------------------------------------------------------------------------------

namespace {

  /// 素朴なテキストエディタ(Motif版)
  class Editor_Frame : public Frame {
    Widget buf, dialog;

    static void copy_selected(Widget widget, XtPointer client_data, XtPointer call_data);
    static void cut_selected(Widget widget, XtPointer client_data, XtPointer call_data);
    static void paste_selected(Widget widget, XtPointer client_data, XtPointer call_data);
    static void all_selected(Widget widget, XtPointer client_data, XtPointer call_data);
    static void get_selected(Widget widget, XtPointer client_data, XtPointer call_data);
    static void append_hello_text_proc( Widget widget, XtPointer client_data, XtPointer call_data);
    static void open_file(Widget widget, XtPointer client_data, XtPointer call_data);
    static void open_file_dialog(Widget widget, XtPointer client_data, XtPointer call_data);

  public:
    Widget create_contents(Widget shell);
  };

  class Editor_Frame_Factory : public Frame_Factory {
    Frame *get_instance() { return new Editor_Frame(); }
  };

  /// 選択した範囲をクリップボードに複製
  void Editor_Frame::copy_selected(Widget widget, XtPointer client_data, XtPointer call_data) {
    XmPushButtonCallbackStruct *cs = (XmPushButtonCallbackStruct *)call_data;
    Editor_Frame *app = (Editor_Frame *)client_data;

    XmTextCopy(app->buf, cs->event->xbutton.time);
    cerr << "TRACE: " << XtName(widget) << " copy selected." << endl;
  }

  /// 選択した範囲を削除して、クリップボードに複製
  void Editor_Frame::cut_selected(Widget widget, XtPointer client_data, XtPointer call_data) {
    XmPushButtonCallbackStruct *cs = (XmPushButtonCallbackStruct *)call_data;
    Editor_Frame *app = (Editor_Frame *)client_data;
    XmTextCut(app->buf, cs->event->xbutton.time);
    cerr << "TRACE: " << XtName(widget) << " cut selected." << endl;
  }

  /// カーソル位置にクリップボードの内容を複製
  void Editor_Frame::paste_selected(Widget widget, XtPointer client_data, XtPointer call_data) {
    Editor_Frame *app = (Editor_Frame *)client_data;
    XmTextPaste(app->buf);
    cerr << "TRACE: " << XtName(widget) << " paste selected." << endl;
  }

  /// すべての要素を選択した状態にする
  void Editor_Frame::all_selected(Widget widget, XtPointer client_data, XtPointer call_data) {
    XmAnyCallbackStruct *cs = (XmAnyCallbackStruct *)call_data;
    Editor_Frame *app = (Editor_Frame *)client_data;
    XmTextPosition start = 0;
    XmTextPosition last = XmTextGetLastPosition(app->buf);
    XmTextSetSelection(app->buf, start, last, cs->event->xbutton.time);
    cerr << "TRACE: " << XtName(widget) << " all selected." << endl;
  }

  /// 選択された領域を入手する
  void Editor_Frame::get_selected(Widget widget, XtPointer client_data, XtPointer call_data) {
    XmPushButtonCallbackStruct *cs = (XmPushButtonCallbackStruct *)call_data;
    Editor_Frame *app = (Editor_Frame *)client_data;

    wchar_t *wcs = XmTextGetSelectionWcs(app->buf);
    if (!wcs) {
      //選択されている領域がない場合は全テキストを対象とする
      wcs =  XmTextGetStringWcs(app->buf);
    }

    if (wcs) 
      printf("%ls\n", wcs);

    XtFree((char *)wcs);
  }

  /// テキストを末尾に追加する
  static void append_text(Widget text, const wchar_t *buf) {
    XmTextPosition last = XmTextGetLastPosition(text);
    XmTextReplaceWcs(text, last, last, (wchar_t *)buf);
  }

  /// テキストをカレット位置に追加する
  static void insert_text(Widget text, const wchar_t *buf) {
    XmTextPosition pos = XmTextGetCursorPosition(text);
    XmTextReplaceWcs(text, pos, pos, (wchar_t *)buf);
  }

  /// テキストを置き換える
  static void replace_text(Widget text, const wchar_t *buf) {
    XmTextPosition last = XmTextGetLastPosition(text);
    XmTextReplaceWcs(text, 0, last, (wchar_t *)buf);
  }

  /// 地域時間を入手する
  static void fetch_localtime(struct tm *local) {
    time_t now;

    if (time(&now) == (time_t)-1) { perror("time"); return; }

    if (!localtime_r(&now, local)) {
      cerr << "unkown localtime." << endl;
    }
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
    if (len != (size_t)-1) {
      wcs[len] = 0;
      insert_text(text, wcs);
    }
  }

  void Editor_Frame::append_hello_text_proc( Widget widget, XtPointer client_data, XtPointer call_data) {
    Editor_Frame *app = (Editor_Frame *)client_data;
    wchar_t buf[] = L"Hello,world\n";
    append_text(app->buf, buf);
  }

  /// ファイルが選択された
  void Editor_Frame::open_file(Widget widget, XtPointer client_data, XtPointer call_data) {
    XmFileSelectionBoxCallbackStruct *cs = (XmFileSelectionBoxCallbackStruct *)call_data;
    Editor_Frame *app = (Editor_Frame *)client_data;

    char *filename = (char *)
      XmStringUnparse(cs->value,
		      XmFONTLIST_DEFAULT_TAG,
		      XmCHARSET_TEXT,
		      XmCHARSET_TEXT,
		      NULL, 0, XmOUTPUT_ALL);

    if (!filename || !*filename) {
      cerr << "WARNING: No file selected." << endl;
      XtFree(filename);
      XtUnmanageChild(widget);
      return;
    }

    cerr << "TRACE: Filename given: " << filename << endl;
    wstring wtext = load_wtext(filename);

    replace_text(app->buf, wtext.c_str());
    XtFree(filename);
    XtUnmanageChild(widget);
  }

  /// ダイアログのボタンが押されるとポップダウンする
  static void close_dialog(Widget widget, XtPointer client_data, XtPointer call_data) {
    XtUnmanageChild(widget);
  }

  /// ファイルを選択してテキストを読み込む
  void Editor_Frame::open_file_dialog(Widget widget, XtPointer client_data, XtPointer call_data) {
    Editor_Frame *app = (Editor_Frame *)client_data;
    cerr << "TRACE: " << XtName(widget) << " open file dialog selected." << endl;

    if (!app->dialog) {
      Arg args[2];
      XtSetArg (args[0], XmNpathMode, XmPATH_MODE_RELATIVE);

      Widget dialog = 
	XmCreateFileSelectionDialog(find_shell(app->buf,0), "filesb", args, 1);
      XtAddCallback(dialog, XmNcancelCallback, close_dialog, app);
      XtAddCallback(dialog, XmNokCallback, open_file, app);
      show_component_tree(XtParent(dialog));
      app->dialog = dialog;
    }

    XtManageChild(app->dialog);
  }

  /// テキスト・エディタのメイン・ウィンドウを作成する
  Widget Editor_Frame::create_contents(Widget shell) {

    Widget main =
      XmVaCreateManagedMainWindow(shell,"main", NULL);

    Widget bar = 
      XmCreateMenuBar(main,"bar",0,0);
    XtManageChild(bar);

    this->buf =
      XmVaCreateManagedText(main, "buf", 
			    XmNeditMode, XmMULTI_LINE_EDIT,
			    NULL);

    XtVaSetValues(main, XmNworkWindow, this->buf, NULL);

    struct Menu_Item memo_items[] = {
      { "new", menu_selected, this, },
      { "open-file-dialog", open_file_dialog, this, },
      { "get-seleted", get_selected, this, },
      { "hello", append_hello_text_proc, this, },
      { "now", insert_datetime_text_proc, this->buf, },
      { "-", menu_selected, },
      { "close", quit_application, },
      { 0, },
    };

    struct Menu_Item edit_items[] = {
      { "copy-to-cripboard", copy_selected, this, },
      { "yank-from-cripboard", paste_selected, this, },
      { "delete-selected", cut_selected, this, },
      { "all-selected", all_selected, this, },
      { 0, },
    };

    XmVaCreateManagedCascadeButton(bar, "memo",
				   XmNsubMenuId, create_pulldown_menu("memo", bar, memo_items),
				   NULL);

    XmVaCreateManagedCascadeButton(bar, "edit",
				   XmNsubMenuId, create_pulldown_menu("edit", bar, edit_items),
				   NULL);
    return 0;
  }

};

// --------------------------------------------------------------------------------

namespace {

  /// MRM;Motif Resource Managerを利用するサンプル・コード
  struct mrm_app {
    MrmHierarchy hierarchy_id;
    Widget container;
    MrmType mrm_class;
  };

  static const char *get_mrm_reason(Cardinal rc) {
    switch(rc) {
    case MrmSUCCESS: return "SUCCESS";
    case MrmBAD_HIERARCHY: return "BAD HIERARCHY";
    case MrmNOT_FOUND: return "NOT FOUND";
    case MrmFAILURE: return "FAILURE";
    }
    return "UNKOWN";
  }

  static void mrm_report_proc(Widget widget, XtPointer client_data, XtPointer call_data) {
    Widget dialog = (Widget)client_data;
#if 1
    mrm_app *app = (mrm_app *)client_data;
    cerr << "container:" << XtName(app->container) << endl;
#else
    int *app = (int *)client_data;
    cerr << "int:" << *app << endl;
#endif
  }

  static void mrm_destroy_proc(Widget widget, XtPointer client_data, XtPointer call_data) {
    mrm_app *app = (mrm_app *)client_data;
    if (app->hierarchy_id) {
      Cardinal rc = MrmCloseHierarchy(app->hierarchy_id);
      cerr << "TRACE: MrmCloseHierarchy:" << get_mrm_reason(rc) << endl;
    }
    delete app;
    cerr << "TRACE: app released." << endl;
  }

  static int create_mrm_app(char *app_name, Widget top) {
    static char *uid_files[] = { "mrmtest.uid", };
    /*
     * 「UIL File Format」をキーに検索するとUILファイルの書式が見つかる
     */
    mrm_app *app = new mrm_app();

    MrmRegisterArg reg_list[] = {
      { "quit_application", (XtPointer)quit_application, },
      { "app_close", (XtPointer)mrm_destroy_proc, },
      { "app_report", (XtPointer)mrm_report_proc, },
      { "app", (XtPointer)app, }, // UIL のidentifierで定義している
    };

    MrmInitialize();
    /*
      MRMの初期化
     */

    MrmOsOpenParamPtr *ancillary_structures_list = 0;
    Cardinal rc = 
      MrmOpenHierarchyPerDisplay(XtDisplay(top), 
				 XtNumber(uid_files),uid_files, 
				 ancillary_structures_list, &app->hierarchy_id);
    /*
      UILファイルの読み取り
     */
    if (rc != MrmSUCCESS) {
      cerr << "ERROR: MrmOpenHierarchyPerDisplay:" << get_mrm_reason(rc) << endl;
      return 1;
    }

    rc = MrmRegisterNamesInHierarchy(app->hierarchy_id, reg_list, XtNumber(reg_list));
    /*
      UILの定義文字列とC関数の対応付け
     */
    if (rc != MrmSUCCESS) {
      cerr << "ERROR: MrmRegisterNamesInHierarchy:" << get_mrm_reason(rc) << endl;
      return 1;
    }

    rc = MrmFetchWidget(app->hierarchy_id, app_name, top, &app->container, &app->mrm_class);
    /*
      Widgetの生成
     */
    if (rc != MrmSUCCESS) {
      cerr << "ERROR: MrmFetchWidget:" << app_name << ":" << get_mrm_reason(rc) << endl;
      return 1;
    }
    XtManageChild(app->container);
    XtRealizeWidget(top);
    return 0;
  }

  static int motif_mrmtest(int argc, char **argv) {
    static String app_class = "MRMtest", app_name = "mrmtest",
      fallback_resouces[] = { 
      "*fontList: -*-*-*-*-*--16-*:",
      NULL,
    };
    static XrmOptionDescRec options[] = { };

    XtSetLanguageProc(NULL, NULL, NULL);

    XtAppContext context;
    Widget top = 
      XtVaOpenApplication(&context, app_class, options, XtNumber(options),
			  &argc, argv, fallback_resouces, applicationShellWidgetClass, NULL);

    XtAddEventHandler(top, NoEventMask, True, _XEditResCheckMessages, NULL);

    int rc = create_mrm_app(app_name, top);
    if (rc == 0) XtAppMainLoop(context);

    XtDestroyApplicationContext(context);
    cerr << "TRACE: context destroyed." << endl;
    return 0;
  }

};

// --------------------------------------------------------------------------------

namespace {

  enum Error_Level { F, E, W, N, I, A, D, T };

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
    XtAddEventHandler(owner, NoEventMask, True, _XEditResCheckMessages, NULL);

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

  /// Athena Widget Set を使うアプリケーションコンテナの利用開始
  static int motif_app01(int argc, char **argv) {

    struct { char *name; Frame_Factory *ff; } fflist[] = {
      { "hello", new Hello_Frame_Factory(), },
      { "motif-parts", new Parts_Frame_Factory(), },
      { "motif-editor", new Editor_Frame_Factory(), },
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
      "Hello_Frame.geometry: 300x200",
      "*fontList: -*-*-*-*-*--16-*:",
      "*msg.labelString: Welcome to Motif World. 日本語も表示できます",

      "*fontList: -*-*-*-*-*--16-*:",

      "*tab-render.tabList: 20fu, +10fu, +10fu, +30fu, +30fu, +30fu",
      "*tab-render.fontName: -*-*-*-*-*--16-*",
      "*tab-render.fontType: FONT_IS_FONTSET",

      "*buf.columns: 30",
      "*buf.rows: 15",

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
    static XrmOptionDescRec options[] = { };
    static XtActionsRec actions[] = { };

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

};

// --------------------------------------------------------------------------------

#include "subcmd.h"

subcmd motif_cmap[] = {
  { "motif-parts", motif_app01, },
  { "motif-editor", motif_app01,  },
  { "mrm", motif_mrmtest,  },
  { "motif", motif_app01,  },
  { 0 },
};

