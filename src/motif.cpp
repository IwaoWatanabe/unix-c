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

#include <Xm/PushB.h>

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

#include "subcmd.h"

subcmd motif_cmap[] = {
  { "hello03", xwin::motif_hello,  },
  { 0 },
};

