/*! \file
 * \brief XViewを使ったサンプルコード
 */

#include <iostream>
#include <libintl.h>
#include <xview/xview.h>
#include <xview/panel.h>
#include <xview/textsw.h>

using namespace std;

// --------------------------------------------------------------------------------

/// XViewのフレーム表示

static int frame_only(int argc,char **argv) {
  /// XViewの初期化
  xv_init(XV_USE_LOCALE, TRUE,
	  XV_INIT_ARGC_PTR_ARGV, &argc, argv, NULL);
  // フレームの生成
  Frame frame = (Frame)
    xv_create(XV_NULL, FRAME, 
	      FRAME_LABEL, gettext("frame test"),
	      FRAME_SHOW_FOOTER, TRUE,
	      FRAME_LEFT_FOOTER, gettext("left footer"),
	      FRAME_RIGHT_FOOTER, gettext("right footer"),
	      NULL);

  xv_main_loop(frame); // イベント処理の無限ループ
  return 0;
}

// --------------------------------------------------------------------------------

// ボタンのコール・バック関数の本体
static int quit_proc(Panel_item item, Event *event) {
  Frame frame = xv_get(item, PANEL_CLIENT_DATA);
  xv_destroy_safe(frame);
  return XV_OK;
}

static int show_frame_proc(Panel_item item, Event *event) {
  Frame frame = xv_get(item, PANEL_CLIENT_DATA);
  xv_set(frame, XV_SHOW, TRUE, NULL);
  return XV_OK;
}

static int pushpin_in_proc(Panel_item item, Event *event) {
  Frame frame = item = xv_get(item, PANEL_CLIENT_DATA);
  xv_set(frame, FRAME_CMD_PUSHPIN_IN, TRUE, NULL);
  return XV_OK;
}

static int pushpin_out_proc(Panel_item item, Event *event) {
  Frame frame = item = xv_get(item, PANEL_CLIENT_DATA);
  xv_set(frame, FRAME_CMD_PUSHPIN_IN, FALSE, NULL);
  return XV_OK;
}

static int menu_ok_proc(Panel_item item, Event *event) {
  cerr << "menu ok" << endl;
  return XV_OK;
}

// チョイスのコール・バック関数
static int choice_proc(Panel_item item, int index, Event *event) {
  Frame frame = xv_get(item, PANEL_CLIENT_DATA);

  cerr << "choice proc:" << item << " index:" << index << endl;
  cerr << "frame:" << frame << endl;
  return XV_OK;
}

// チョイスのコール・バック関数
static int list_proc(Panel_item item, Panel_list_op op, Event *event) {
  Frame frame = xv_get(item, PANEL_CLIENT_DATA);

  char *op_text = "UNKOWN";

  switch(op) {
  case PANEL_LIST_OP_DELETE: op_text = "DELETE"; break;
  case PANEL_LIST_OP_SELECT: op_text = "SELECT"; break;
  case PANEL_LIST_OP_DESELECT: op_text = "DESELECT"; break;
  case PANEL_LIST_OP_VALIDATE: op_text = "VALIDATE"; break;
  }

  // 想定通りのオペレーションではない。書籍の何かが間違っている
  cerr << "list proc:" << item << " operation:" << op << ":" << op_text << endl;

#if 0
  op_text = "UNKOWN";
  if ((op & PANEL_LIST_OP_DELETE) == PANEL_LIST_OP_DELETE)
    op_text = "DELETE";
  else if ((op & PANEL_LIST_OP_SELECT) == PANEL_LIST_OP_SELECT)
    op_text = "SELECT";
  else if ((op & PANEL_LIST_OP_DESELECT) == PANEL_LIST_OP_DESELECT)
    op_text = "DESELECT";
  else if ((op & PANEL_LIST_OP_VALIDATE) == PANEL_LIST_OP_VALIDATE)
    op_text = "VALIDATE";

  cerr << "list proc:" << item << " operation:" << op << ":" << op_text << endl;
#endif

  cerr << "frame:" << frame << endl;

  cerr << "selected 0 ?" << xv_get(item, PANEL_LIST_SELECTED, 0) << endl;
  
  return XV_OK;
}

// メニューのコール・バック関数
static void menu_proc(Menu menu, Menu_item item) {
  const char *item_text = (char *)xv_get(item, MENU_STRING);
  Frame frame = xv_get(menu, MENU_CLIENT_DATA);

  cerr << "menu proc:" << item_text << endl;
  cerr << "frame:" << frame << endl;

  xv_set(frame, FRAME_LEFT_FOOTER, item_text, NULL);
}

// テキスト・アイテムのコールバック
static int text_notify(Panel_item item, Event *event) {
  cerr << "text notify" << item << endl;

  /*
    PANEL_NONE: 特別な操作をしない
    PANEL_NEXT: キャレットを次のアイテムに移動する
    PANEL_PREVIOUS: キャレットを前のアイテムに移動する
    PANEL_INSERT: 入力データを値として挿入する
   */

  if (event_is_down(event)) {

    const char *text = "";

    switch(event_action(event)) {
    case '\n': case '\r':
      text = (char *)xv_get(item, PANEL_VALUE);
      cerr << "value: " << text << endl;
      return PANEL_NEXT;
      
    case '\t':
      return PANEL_NEXT;

    default:
      return PANEL_INSERT;
    }
  }

  return PANEL_NONE;
}

// パネル・アイテムの確認
static int vpanel(int argc,char **argv) {

  char *domain = "messages";
  bindtextdomain(domain, ".");
  textdomain(domain);

  xv_init(XV_USE_LOCALE, TRUE,
	  XV_INIT_ARGC_PTR_ARGV, &argc, argv, NULL);

  Frame frame = (Frame)
    xv_create(XV_NULL, FRAME,
	      FRAME_SHOW_HEADER, TRUE,
	      FRAME_LABEL, gettext("button test"),
	      FRAME_SHOW_FOOTER, TRUE,
	      FRAME_LEFT_FOOTER, gettext("left footer"),
	      FRAME_RIGHT_FOOTER, gettext("right footer"),
	      NULL);

  Panel panel = (Panel)
    xv_create( frame, PANEL, NULL );

  Panel_item button, choice, list, message;
  
  button = (Panel_item)
    xv_create( panel, PANEL_BUTTON,
	       PANEL_LABEL_STRING,  gettext("quit"),
	       PANEL_NOTIFY_PROC, quit_proc,
	       PANEL_CLIENT_DATA, frame,
	       NULL );


  Menu menu = 
    xv_create( XV_NULL, MENU,
	       MENU_STRINGS, "AAA", "BBB", "CCC", "DDD", NULL,
#if 1
	      MENU_NOTIFY_PROC, menu_proc,
	      MENU_CLIENT_DATA, frame,
#endif
	      NULL);

  Panel_item menu_button = (Panel_item)
    xv_create(panel, PANEL_BUTTON,
	      PANEL_LABEL_STRING,  gettext("menu"),
	      PANEL_NOTIFY_PROC, menu_ok_proc,
	      PANEL_ITEM_MENU, menu,
	      NULL );
  
  int next_row = 4;

  // チョイス（ラジオボタン）
  choice = (Panel_item)
    xv_create(panel, PANEL_CHOICE,
	      PANEL_LABEL_STRING, gettext("Choice:"),
	      PANEL_CHOICE_STRINGS,
	      "One", "Two", "Three", "Four", NULL,
	      PANEL_NOTIFY_PROC, choice_proc,
	      PANEL_CLIENT_DATA, frame,
	      PANEL_NEXT_ROW, next_row + 4,
	      NULL);

  // オプション（選択した一つだけ表示）
  choice = (Panel_item)
    xv_create(panel, PANEL_CHOICE_STACK,
	      PANEL_LABEL_STRING, gettext("Choice:"),
	      PANEL_CHOICE_STRINGS,
	      "One", "Two", "Three", "Four", NULL,
	      PANEL_NOTIFY_PROC, choice_proc,
	      PANEL_CLIENT_DATA, frame,
	      PANEL_NEXT_ROW, next_row,
	      NULL);

  // チェックボックス付のラジオボタン
  choice = (Panel_item)
    xv_create(panel, PANEL_CHOICE,
	      PANEL_LABEL_STRING, gettext("Choice:"),
	      PANEL_CHOICE_STRINGS,
	      "One", "Two", "Three", "Four", NULL,
	      PANEL_FEEDBACK, PANEL_MARKED,
	      PANEL_NOTIFY_PROC, choice_proc,
	      PANEL_CLIENT_DATA, frame,
	      PANEL_NEXT_ROW, next_row,
	      NULL);

  list = 
    xv_create(panel, PANEL_LIST,
	      PANEL_LABEL_STRING, gettext("Number List:"),
	      PANEL_CHOOSE_ONE, FALSE,
	      PANEL_LIST_STRINGS,
	      "One", "Two", "Three", "Four", "Five", "Six", "Seven", "Eight", "Nine", "Ten", NULL,
	      PANEL_NOTIFY_PROC, list_proc, 
	      PANEL_CLIENT_DATA, frame,
	      PANEL_NEXT_ROW, next_row,
	      NULL);
  #if 0
  xv_create(panel, PANEL_SLIDER,
	    PANEL_LABEL_STRING, gettext("NUMERIC:"),
	    PANEL_SLIDER_WIDTH, 200,
	    PANEL_MIN_VALUE, 0,
	    PANEL_MAX_VALUE, 99,
	    NULL);
  #endif

  message = 
    xv_create(panel, PANEL_MESSAGE,
	      PANEL_LABEL_STRING, gettext("message text."),
	      PANEL_NEXT_ROW, next_row,
	      NULL);

  message = 
    xv_create(panel, PANEL_TEXT,
	      PANEL_LABEL_STRING, gettext("text inputs: "),
	      PANEL_VALUE_DISPLAY_LENGTH, 30,
	      PANEL_VALUE_STORED_LENGTH, 500,
	      PANEL_NOTIFY_PROC, text_notify,
	      PANEL_NEXT_ROW, next_row,
	      NULL);

  message = 
    xv_create(panel, PANEL_TEXT,
	      PANEL_LABEL_STRING, gettext("passwd inputs: "),
	      PANEL_VALUE_DISPLAY_LENGTH, 30,
	      PANEL_VALUE_STORED_LENGTH, 100,
	      PANEL_MASK_CHAR, '*',
	      PANEL_NEXT_ROW, next_row,
	      NULL);

  Frame cmd_frame = (Frame)
    xv_create(frame, FRAME_CMD,
	      FRAME_SHOW_HEADER, TRUE,
	      FRAME_LABEL, gettext("panel item test"),
	      NULL);

  button = (Panel_item)
    xv_create( panel, PANEL_BUTTON,
	       PANEL_LABEL_STRING,  gettext("command panel"),
	       PANEL_CLIENT_DATA, cmd_frame,
	       PANEL_NOTIFY_PROC, show_frame_proc,
	       PANEL_NEXT_ROW, next_row + 4,
	       NULL );

  window_fit( panel );  /* パネルのサイズの自動調整 */
  window_fit( frame );  /* フレームのサイズの自動調整 */

  Panel cmd_panel = (Panel)xv_get(cmd_frame, FRAME_CMD_PANEL);
  xv_create(cmd_panel, PANEL_BUTTON,
	    PANEL_LABEL_STRING, gettext("push pin in"),
	    PANEL_NOTIFY_PROC, pushpin_in_proc,
	    PANEL_CLIENT_DATA, cmd_frame,
	    NULL);

  xv_create(cmd_panel, PANEL_BUTTON,
	    PANEL_LABEL_STRING, gettext("push pin out"),
	    PANEL_NOTIFY_PROC, pushpin_out_proc,
	    PANEL_CLIENT_DATA, cmd_frame,
	    NULL);
  window_fit( cmd_panel );  /* パネルのサイズの自動調整 */
  window_fit( cmd_frame );  /* フレームのサイズの自動調整 */

  xv_main_loop(frame);

  cerr << "exit main loop" << endl;
  return 0;
}

// --------------------------------------------------------------------------------

#ifdef USE_SUBCMD

#include "subcmd.h"

subcmd xview_cmap[] = {
  { "olt-frame", frame_only, },
  { "olt-panel", vpanel, },
  { 0 },
};

#endif

