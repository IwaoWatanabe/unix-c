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

using namespace std;

namespace xwin {

  /// トップレベル・シェルの破棄する
  static void quit_application(Widget widget, XtPointer client_data, XtPointer call_data) {
    dispose_shell(widget);
    cerr << "TRACE: quit application called." << endl;
  }

  /// メッセージを出現させる
  static int motif_hello(int argc, char **argv) {
    static String app_class = "HelloMotif", 
      fallback_resouces[] = { 
      "*fontList: -*-*-*-*-*--16-*:",
      "*msg.labelString: Welcome to Motif World. 日本語も表示できます",
      "HelloMotif.geometry: 300x200",
      NULL,
    };
    
    static XrmOptionDescRec options[] = { };
    
    XtAppContext context;

    Widget top = 
      XtVaOpenApplication(&context, app_class, options, XtNumber(options),
			  &argc, argv, fallback_resouces, applicationShellWidgetClass, NULL);

    Widget pb = XmVaCreateManagedPushButton(top,"msg",NULL);

#if 0
    XtVaSetValues(pb, XmNlabelString, XmStringCreateLocalized("日本語テキスト"), NULL);
#endif
    XtAddCallback(pb, XmNactivateCallback, quit_application, NULL);
    XtRealizeWidget(top);

    XtAppMainLoop(context);
    XtDestroyApplicationContext(context);
    cerr << "TRACE: context destroyed." << endl;
    return 0;
  }
};

// --------------------------------------------------------------------------------

namespace xwin {

  /// メニュー・アイテムの登録準備
  struct menu_item {
    String name; ///< アイテム名
    XtCallbackProc proc; ///< コールバック
    XtPointer closure; ///< クライアント・データに渡す値
    struct menu_item *sub_menu; ///< カスケード
  };

  /// ポップアップ・メニューの作成(motif)
  static Widget create_pulldown_menu(String menu_name, Widget parent, menu_item *items) {
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

  extern bool load_dirent(const char *dirpath, vector<string> &entries, bool with_hidden_file = false);

  /// フォルダ一覧を表示するリスト
  static void create_folder_list(Widget top) {
    vector<string> entries;

    load_dirent(".", entries, false);

    Widget main =
      XmVaCreateManagedMainWindow(top,"list-main", NULL);

    Widget bar =
      XmCreateMenuBar(main,"bar",0,0);
    XtManageChild(bar);

    struct menu_item dir_items[] = {
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

    struct menu_item dir_items[] = {
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

  struct parts_app {
    Widget info, warn, err;
    Widget prompt, command, selection;
    Widget tree;
  };

  /// メニューアイテムを選択した時の処理
  static void menu_selected(Widget widget, XtPointer client_data, XtPointer call_data) {
    cout << "TRACE: " << XtName(widget) << " selected." << endl;
  }
  
  static void list_menu_selected(Widget widget, XtPointer client_data, XtPointer call_data) {
    cout << "TRACE: " << XtName(widget) << " selected." << endl;
    String app_class = "FileList";
    Widget top = XtVaAppCreateShell(NULL, app_class, applicationShellWidgetClass, XtDisplay(XtParent(widget)), NULL);
    app_shell_count++;
    create_folder_list(top);
  }

  static void passwdlist_menu_selected(Widget widget, XtPointer client_data, XtPointer call_data) {
    cout << "TRACE: " << XtName(widget) << " selected." << endl;
    String app_class = "PasswdList";
    Widget top = XtVaAppCreateShell(NULL, app_class, applicationShellWidgetClass, XtDisplay(XtParent(widget)), NULL);
    app_shell_count++;
    create_passwd_list(top);
  }

  static void parts_menu_selected(Widget widget, XtPointer client_data, XtPointer call_data) {
    cout << "TRACE: " << XtName(widget) << " selected." << endl;
  }

  /// ボタン押下の通知
  static void button_selected(Widget widget, XtPointer client_data, XtPointer call_data) {
    cout << "TRACE: " << XtName(widget) << " selected." << endl;
    char *label = (char *)client_data;
    cout << "TRACE: press button:" << label << endl;
  }

  /// 情報メッセージの出力
  static void info_menu_selected(Widget widget, XtPointer client_data, XtPointer call_data) {
    cout << "TRACE: " << XtName(widget) << " selected." << endl;
    parts_app *app = (parts_app *)client_data;

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
  static void warn_menu_selected(Widget widget, XtPointer client_data, XtPointer call_data) {
    cout << "TRACE: " << XtName(widget) << " selected." << endl;
    parts_app *app = (parts_app *)client_data;

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
  static void err_menu_selected(Widget widget, XtPointer client_data, XtPointer call_data) {
    cout << "TRACE: " << XtName(widget) << " selected." << endl;
    parts_app *app = (parts_app *)client_data;

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
    cout << "TRACE: " << XtName(widget) << " inputs callback." << endl;
    XmSelectionBoxCallbackStruct *cbs = (XmSelectionBoxCallbackStruct *)call_data;

    char *value = (char *)XmStringUnparse(cbs->value, NULL, XmMULTIBYTE_TEXT, XmMULTIBYTE_TEXT, NULL, 0, XmOUTPUT_ALL);
    cerr << "INFO: " << value << endl;
    XtFree(value);
    /// マルチバイトで取り出す

    wchar_t *wcs = (wchar_t *)XmStringUnparse(cbs->value, NULL, XmWIDECHAR_TEXT, XmWIDECHAR_TEXT, NULL, 0, XmOUTPUT_ALL);
    wcerr << "INFO: " << wcs << "(wcs)" << endl;
    XtFree((char *)wcs);
    /// ワイドキャラクタで取り出す
  }

  /// プロンプト・ダイアログ表示機能の選択
  static void prompt_menu_selected(Widget widget, XtPointer client_data, XtPointer call_data) {
    cout << "TRACE: " << XtName(widget) << " selected." << endl;
    parts_app *app = (parts_app *)client_data;

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
    cout << "TRACE: " << XtName(widget) << " command callback." << endl;
    XmCommandCallbackStruct *cbs = (XmCommandCallbackStruct *)call_data;

    switch (cbs->reason) {
    case XmCR_COMMAND_ENTERED:
      break;
    case XmCR_COMMAND_CHANGED:
      cerr << "TRACE: CHANGED" << endl;
    }

    wchar_t *wcs = (wchar_t *)XmStringUnparse(cbs->value, NULL, XmWIDECHAR_TEXT, XmWIDECHAR_TEXT, NULL, 0, XmOUTPUT_ALL);
    wcerr << "INFO: " << wcs << "(wcs)" << endl;
    XtFree((char *)wcs);
    /// ワイドキャラクタで取り出す
  }

  /// コマンド・ダイアログ表示機能の選択
  static void command_menu_selected(Widget widget, XtPointer client_data, XtPointer call_data) {
    cout << "TRACE: " << XtName(widget) << " selected." << endl;
    parts_app *app = (parts_app *)client_data;

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
    cout << "TRACE: " << XtName(widget) << " selected." << endl;
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
    XtFree((char *)wcs);
    /// ワイドキャラクタで取り出す
  }

  static void selection_menu_selected(Widget widget, XtPointer client_data, XtPointer call_data) {
    cout << "TRACE: " << XtName(widget) << " selected." << endl;
    parts_app *app = (parts_app *)client_data;

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
  static void create_tree_test(Widget shell) {
    parts_app *app = new parts_app();

    Widget main, tree;
    main = XtVaCreateManagedWidget("main", xmMainWindowWidgetClass, shell, NULL);

    Widget bar =
      XmCreateMenuBar(main,"bar",0,0);

    struct menu_item tree_items[] = {
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

  static void tree_menu_selected(Widget widget, XtPointer client_data, XtPointer call_data) {
    cout << "TRACE: " << XtName(widget) << " selected." << endl;
    String app_class = "Tree";
    Widget top = XtVaAppCreateShell(NULL, app_class, applicationShellWidgetClass, 
				    XtDisplay(XtParent(widget)), NULL);
    app_shell_count++;
    create_tree_test(top);
  }

  /// パーツの振る舞いの確認
  static void create_parts_test(Widget shell) {
    parts_app *app = new parts_app();

    Widget row_column = XmVaCreateManagedRowColumn(shell,"row-column", NULL);

    struct {
      String name; ///< アイテム名
      XtCallbackProc proc; ///< コールバック
      XtPointer closure; ///< クライアント・データに渡す値
    } items[] = {
      { "none", menu_selected, },
      { "tree", tree_menu_selected, },
      { "list", list_menu_selected, },
      { "password-list", passwdlist_menu_selected, },
      { "info", info_menu_selected, app, },
      { "warning", warn_menu_selected, app, },
      { "error", err_menu_selected, app, },

      { "prompt", prompt_menu_selected, app, },
      { "command", command_menu_selected, app, },
      { "selection", selection_menu_selected, app, },
      { "quit", quit_application, },
    };

    for(int i = 0; i < XtNumber(items); i++) {
      Widget item = 
	XmVaCreateManagedPushButtonGadget(row_column, items[i].name, NULL);
      XtAddCallback(item, XmNactivateCallback, items[i].proc, items[i].closure);
    }

    XtRealizeWidget(shell);
  }

  /// Motifのさまざまなウィジェットの振る舞いを確認する
  static int motif_parts(int argc, char **argv) {
    static String app_class = "MotifParts", 
      fallback_resouces[] = { 
      "*fontList: -*-*-*-*-*--16-*:",
      "*tab-render.tabList: 20fu, +10fu, +10fu, +30fu, +30fu, +30fu",
      "*tab-render.fontName: -*-*-*-*-*--16-*",
      "*tab-render.fontType: FONT_IS_FONTSET",
      NULL,
    };

    static XrmOptionDescRec options[] = { };
    XtAppContext context;

    XtSetLanguageProc(NULL, NULL, NULL);
    Widget top = 
      XtVaOpenApplication(&context, app_class, options, XtNumber(options),
			  &argc, argv, fallback_resouces, applicationShellWidgetClass, NULL);
    app_shell_count++;
    
    XtAddEventHandler(top, NoEventMask, True, _XEditResCheckMessages, NULL);

    create_parts_test(top);

    XtAppMainLoop(context);
    XtDestroyApplicationContext(context);
    cerr << "TRACE: context destroyed." << endl;
    return 0;
  }

};

#include "subcmd.h"

subcmd motif_cmap[] = {
  { "hello03", xwin::motif_hello,  },
  { "motif", xwin::motif_parts,  },
  { 0 },
};

