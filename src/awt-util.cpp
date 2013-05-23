/*! \file                                                                                                           
 * \brief Athena Widget 依存の拡張モジュール                                                                        
 */

#include <X11/IntrinsicP.h>
#include <X11/ObjectP.h>
#include <X11/CoreP.h>

#ifdef XAW3D
#include <X11/Xaw3d/ListP.h>
#include <X11/Xaw3d/TextP.h>
#else
#include <X11/Xaw/ListP.h>
#endif


/// リストのカラム数を入手する                                                                                      
int getXawListColumns(Widget list) {
  return ((struct _ListRec *)list)->list.ncols;
}

/// リストの行数を入手する                                                                                          
int getXawListRows(Widget list) {
  return ((struct _ListRec *)list)->list.nrows;
}

#ifdef XAW3D
extern "C" {

/// テキストの最後の位置を返す                                                                                      
XawTextPosition XawTextLastPosition(Widget textw) {
  return (((TextWidget)textw)->text.lastPos);
}

};
#endif

