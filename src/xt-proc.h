#ifndef _xt_proc_h
#define _xt_proc_h

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

extern void show_component_tree(Widget target, int level);

#ifdef __cplusplus
extern void show_component_tree(Widget target, int level = 2);
#endif

#ifdef __cplusplus
};
#endif

#endif
