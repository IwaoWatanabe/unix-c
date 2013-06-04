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

#ifdef USE_SUBCMD

#include "subcmd.h"

subcmd xview_cmap[] = {
  { "olt-frame", frame_only, },
  { 0 },
};

#endif

