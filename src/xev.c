
#include <X11/Xproto.h>

const char *Yes = "YES";
const char *No = "NO";
const char *Unknown = "unknown";

XIC xic = NULL;

static void
prologue (XEvent *eventp, const char *event_name)
{
    XAnyEvent *e = (XAnyEvent *) eventp;

    printf ("\n%s event, serial %ld, synthetic %s, window 0x%lx,\n",
	    event_name, e->serial, e->send_event ? Yes : No, e->window);
}

static void
dump (char *str, int len)
{
    printf("(");
    len--;
    while (len-- > 0)
        printf("%02x ", (unsigned char) *str++);
    printf("%02x)", (unsigned char) *str++);
}

static void
do_KeyPress (XEvent *eventp)
{
    XKeyEvent *e = (XKeyEvent *) eventp;
    KeySym ks;
    KeyCode kc = 0;
    Bool kc_set = False;
    const char *ksname;
    int nbytes, nmbbytes = 0;
    char str[256+1];
    static char *buf = NULL;
    static int bsize = 8;
    Status status;

    if (buf == NULL)
      buf = (char *)malloc (bsize);

    nbytes = XLookupString (e, str, 256, &ks, NULL);

    /* not supposed to call XmbLookupString on a key release event */
    if (e->type == KeyPress && xic) {
        do {
            nmbbytes = XmbLookupString (xic, e, buf, bsize - 1, &ks, &status);
            buf[nmbbytes] = '\0';

            if (status == XBufferOverflow) {
                bsize = nmbbytes + 1;
                buf = (char *)realloc (buf, bsize);
            }
        } while (status == XBufferOverflow);
    }

    if (ks == NoSymbol)
	ksname = "NoSymbol";
    else {
	if (!(ksname = XKeysymToString (ks)))
	    ksname = "(no name)";
	kc = XKeysymToKeycode(e->display, ks);
	kc_set = True;
    }
	
    printf ("    root 0x%lx, subw 0x%lx, time %lu, (%d,%d), root:(%d,%d),\n",
	    e->root, e->subwindow, e->time, e->x, e->y, e->x_root, e->y_root);
    printf ("    state 0x%x, keycode %u (keysym 0x%lx, %s), same_screen %s,\n",
	    e->state, e->keycode, (unsigned long) ks, ksname,
	    e->same_screen ? Yes : No);
    if (kc_set && e->keycode != kc)
	printf ("    XKeysymToKeycode returns keycode: %u\n",kc);
    if (nbytes < 0) nbytes = 0;
    if (nbytes > 256) nbytes = 256;
    str[nbytes] = '\0';
    printf ("    XLookupString gives %d bytes: ", nbytes);
    if (nbytes > 0) {
        dump (str, nbytes);
        printf (" \"%s\"\n", str);
    } else {
    	printf ("\n");
    }

    /* not supposed to call XmbLookupString on a key release event */
    if (e->type == KeyPress && xic) {
        printf ("    XmbLookupString gives %d bytes: ", nmbbytes);
        if (nmbbytes > 0) {
           dump (buf, nmbbytes);
           printf (" \"%s\"\n", buf);
        } else {
    	   printf ("\n");
        }
    }

    printf ("    XFilterEvent returns: %s\n", 
	    XFilterEvent (eventp, e->window) ? "True" : "False");
}

static void
do_KeyRelease (XEvent *eventp)
{
    do_KeyPress (eventp);		/* since it has the same info */
}

static void
do_ButtonPress (XEvent *eventp)
{
    XButtonEvent *e = (XButtonEvent *) eventp;

    printf ("    root 0x%lx, subw 0x%lx, time %lu, (%d,%d), root:(%d,%d),\n",
	    e->root, e->subwindow, e->time, e->x, e->y, e->x_root, e->y_root);
    printf ("    state 0x%x, button %u, same_screen %s\n",
	    e->state, e->button, e->same_screen ? Yes : No);
}

static void
do_ButtonRelease (XEvent *eventp)
{
    do_ButtonPress (eventp);		/* since it has the same info */
}

static void
do_MotionNotify (XEvent *eventp)
{
    XMotionEvent *e = (XMotionEvent *) eventp;

    printf ("    root 0x%lx, subw 0x%lx, time %lu, (%d,%d), root:(%d,%d),\n",
	    e->root, e->subwindow, e->time, e->x, e->y, e->x_root, e->y_root);
    printf ("    state 0x%x, is_hint %u, same_screen %s\n",
	    e->state, e->is_hint, e->same_screen ? Yes : No);
}

static void
do_EnterNotify (XEvent *eventp)
{
    XCrossingEvent *e = (XCrossingEvent *) eventp;
    const char *mode, *detail;
    char dmode[10], ddetail[10];

    switch (e->mode) {
      case NotifyNormal:  mode = "NotifyNormal"; break;
      case NotifyGrab:  mode = "NotifyGrab"; break;
      case NotifyUngrab:  mode = "NotifyUngrab"; break;
      case NotifyWhileGrabbed:  mode = "NotifyWhileGrabbed"; break;
      default:  mode = dmode, sprintf (dmode, "%u", e->mode); break;
    }

    switch (e->detail) {
      case NotifyAncestor:  detail = "NotifyAncestor"; break;
      case NotifyVirtual:  detail = "NotifyVirtual"; break;
      case NotifyInferior:  detail = "NotifyInferior"; break;
      case NotifyNonlinear:  detail = "NotifyNonlinear"; break;
      case NotifyNonlinearVirtual:  detail = "NotifyNonlinearVirtual"; break;
      case NotifyPointer:  detail = "NotifyPointer"; break;
      case NotifyPointerRoot:  detail = "NotifyPointerRoot"; break;
      case NotifyDetailNone:  detail = "NotifyDetailNone"; break;
      default:  detail = ddetail; sprintf (ddetail, "%u", e->detail); break;
    }

    printf ("    root 0x%lx, subw 0x%lx, time %lu, (%d,%d), root:(%d,%d),\n",
	    e->root, e->subwindow, e->time, e->x, e->y, e->x_root, e->y_root);
    printf ("    mode %s, detail %s, same_screen %s,\n",
	    mode, detail, e->same_screen ? Yes : No);
    printf ("    focus %s, state %u\n", e->focus ? Yes : No, e->state);
}

static void
do_LeaveNotify (XEvent *eventp)
{
    do_EnterNotify (eventp);		/* since it has same information */
}

static void
do_FocusIn (XEvent *eventp)
{
    XFocusChangeEvent *e = (XFocusChangeEvent *) eventp;
    const char *mode, *detail;
    char dmode[10], ddetail[10];

    switch (e->mode) {
      case NotifyNormal:  mode = "NotifyNormal"; break;
      case NotifyGrab:  mode = "NotifyGrab"; break;
      case NotifyUngrab:  mode = "NotifyUngrab"; break;
      case NotifyWhileGrabbed:  mode = "NotifyWhileGrabbed"; break;
      default:  mode = dmode, sprintf (dmode, "%u", e->mode); break;
    }

    switch (e->detail) {
      case NotifyAncestor:  detail = "NotifyAncestor"; break;
      case NotifyVirtual:  detail = "NotifyVirtual"; break;
      case NotifyInferior:  detail = "NotifyInferior"; break;
      case NotifyNonlinear:  detail = "NotifyNonlinear"; break;
      case NotifyNonlinearVirtual:  detail = "NotifyNonlinearVirtual"; break;
      case NotifyPointer:  detail = "NotifyPointer"; break;
      case NotifyPointerRoot:  detail = "NotifyPointerRoot"; break;
      case NotifyDetailNone:  detail = "NotifyDetailNone"; break;
      default:  detail = ddetail; sprintf (ddetail, "%u", e->detail); break;
    }

    printf ("    mode %s, detail %s\n", mode, detail);
}

static void
do_FocusOut (XEvent *eventp)
{
    do_FocusIn (eventp);		/* since it has same information */
}

static void
do_KeymapNotify (XEvent *eventp)
{
    XKeymapEvent *e = (XKeymapEvent *) eventp;
    int i;

    printf ("    keys:  ");
    for (i = 0; i < 32; i++) {
	if (i == 16) printf ("\n           ");
	printf ("%-3u ", (unsigned int) e->key_vector[i]);
    }
    printf ("\n");
}

static void
do_Expose (XEvent *eventp)
{
    XExposeEvent *e = (XExposeEvent *) eventp;

    printf ("    (%d,%d), width %d, height %d, count %d\n",
	    e->x, e->y, e->width, e->height, e->count);
}

static void
do_GraphicsExpose (XEvent *eventp)
{
    XGraphicsExposeEvent *e = (XGraphicsExposeEvent *) eventp;
    const char *m;
    char mdummy[10];

    switch (e->major_code) {
      case X_CopyArea:  m = "CopyArea";  break;
      case X_CopyPlane:  m = "CopyPlane";  break;
      default:  m = mdummy; sprintf (mdummy, "%d", e->major_code); break;
    }

    printf ("    (%d,%d), width %d, height %d, count %d,\n",
	    e->x, e->y, e->width, e->height, e->count);
    printf ("    major %s, minor %d\n", m, e->minor_code);
}

static void
do_NoExpose (XEvent *eventp)
{
    XNoExposeEvent *e = (XNoExposeEvent *) eventp;
    const char *m;
    char mdummy[10];

    switch (e->major_code) {
      case X_CopyArea:  m = "CopyArea";  break;
      case X_CopyPlane:  m = "CopyPlane";  break;
      default:  m = mdummy; sprintf (mdummy, "%d", e->major_code); break;
    }

    printf ("    major %s, minor %d\n", m, e->minor_code);
    return;
}

static void
do_VisibilityNotify (XEvent *eventp)
{
    XVisibilityEvent *e = (XVisibilityEvent *) eventp;
    const char *v;
    char vdummy[10];

    switch (e->state) {
      case VisibilityUnobscured:  v = "VisibilityUnobscured"; break;
      case VisibilityPartiallyObscured:  v = "VisibilityPartiallyObscured"; break;
      case VisibilityFullyObscured:  v = "VisibilityFullyObscured"; break;
      default:  v = vdummy; sprintf (vdummy, "%d", e->state); break;
    }

    printf ("    state %s\n", v);
}

static void
do_CreateNotify (XEvent *eventp)
{
    XCreateWindowEvent *e = (XCreateWindowEvent *) eventp;

    printf ("    parent 0x%lx, window 0x%lx, (%d,%d), width %d, height %d\n",
	    e->parent, e->window, e->x, e->y, e->width, e->height);
    printf ("border_width %d, override %s\n",
	    e->border_width, e->override_redirect ? Yes : No);
}

static void
do_DestroyNotify (XEvent *eventp)
{
    XDestroyWindowEvent *e = (XDestroyWindowEvent *) eventp;

    printf ("    event 0x%lx, window 0x%lx\n", e->event, e->window);
}

static void
do_UnmapNotify (XEvent *eventp)
{
    XUnmapEvent *e = (XUnmapEvent *) eventp;

    printf ("    event 0x%lx, window 0x%lx, from_configure %s\n",
	    e->event, e->window, e->from_configure ? Yes : No);
}

static void
do_MapNotify (XEvent *eventp)
{
    XMapEvent *e = (XMapEvent *) eventp;

    printf ("    event 0x%lx, window 0x%lx, override %s\n",
	    e->event, e->window, e->override_redirect ? Yes : No);
}

static void
do_MapRequest (XEvent *eventp)
{
    XMapRequestEvent *e = (XMapRequestEvent *) eventp;

    printf ("    parent 0x%lx, window 0x%lx\n", e->parent, e->window);
}

static void
do_ReparentNotify (XEvent *eventp)
{
    XReparentEvent *e = (XReparentEvent *) eventp;

    printf ("    event 0x%lx, window 0x%lx, parent 0x%lx,\n",
	    e->event, e->window, e->parent);
    printf ("    (%d,%d), override %s\n", e->x, e->y, 
	    e->override_redirect ? Yes : No);
}

static void
do_ConfigureNotify (XEvent *eventp)
{
    XConfigureEvent *e = (XConfigureEvent *) eventp;

    printf ("    event 0x%lx, window 0x%lx, (%d,%d), width %d, height %d,\n",
	    e->event, e->window, e->x, e->y, e->width, e->height);
    printf ("    border_width %d, above 0x%lx, override %s\n",
	    e->border_width, e->above, e->override_redirect ? Yes : No);
}

static void
do_ConfigureRequest (XEvent *eventp)
{
    XConfigureRequestEvent *e = (XConfigureRequestEvent *) eventp;
    const char *detail;
    char ddummy[10];

    switch (e->detail) {
      case Above:  detail = "Above";  break;
      case Below:  detail = "Below";  break;
      case TopIf:  detail = "TopIf";  break;
      case BottomIf:  detail = "BottomIf"; break;
      case Opposite:  detail = "Opposite"; break;
      default:  detail = ddummy; sprintf (ddummy, "%d", e->detail); break;
    }

    printf ("    parent 0x%lx, window 0x%lx, (%d,%d), width %d, height %d,\n",
	    e->parent, e->window, e->x, e->y, e->width, e->height);
    printf ("    border_width %d, above 0x%lx, detail %s, value 0x%lx\n",
	    e->border_width, e->above, detail, e->value_mask);
}

static void
do_GravityNotify (XEvent *eventp)
{
    XGravityEvent *e = (XGravityEvent *) eventp;

    printf ("    event 0x%lx, window 0x%lx, (%d,%d)\n",
	    e->event, e->window, e->x, e->y);
}

static void
do_ResizeRequest (XEvent *eventp)
{
    XResizeRequestEvent *e = (XResizeRequestEvent *) eventp;

    printf ("    width %d, height %d\n", e->width, e->height);
}

static void
do_CirculateNotify (XEvent *eventp)
{
    XCirculateEvent *e = (XCirculateEvent *) eventp;
    const char *p;
    char pdummy[10];

    switch (e->place) {
      case PlaceOnTop:  p = "PlaceOnTop"; break;
      case PlaceOnBottom:  p = "PlaceOnBottom"; break;
      default:  p = pdummy; sprintf (pdummy, "%d", e->place); break;
    }

    printf ("    event 0x%lx, window 0x%lx, place %s\n",
	    e->event, e->window, p);
}

static void
do_CirculateRequest (XEvent *eventp)
{
    XCirculateRequestEvent *e = (XCirculateRequestEvent *) eventp;
    const char *p;
    char pdummy[10];

    switch (e->place) {
      case PlaceOnTop:  p = "PlaceOnTop"; break;
      case PlaceOnBottom:  p = "PlaceOnBottom"; break;
      default:  p = pdummy; sprintf (pdummy, "%d", e->place); break;
    }

    printf ("    parent 0x%lx, window 0x%lx, place %s\n",
	    e->parent, e->window, p);
}

static void
do_PropertyNotify (XEvent *eventp)
{
    XPropertyEvent *e = (XPropertyEvent *) eventp;
    char *aname = XGetAtomName (e->display, e->atom);
    const char *s;
    char sdummy[10];

    switch (e->state) {
      case PropertyNewValue:  s = "PropertyNewValue"; break;
      case PropertyDelete:  s = "PropertyDelete"; break;
      default:  s = sdummy; sprintf (sdummy, "%d", e->state); break;
    }

    printf ("    atom 0x%lx (%s), time %lu, state %s\n",
	   e->atom, aname ? aname : Unknown, e->time,  s);

    if (aname) XFree (aname);
}

static void
do_SelectionClear (XEvent *eventp)
{
    XSelectionClearEvent *e = (XSelectionClearEvent *) eventp;
    char *sname = XGetAtomName (e->display, e->selection);

    printf ("    selection 0x%lx (%s), time %lu\n",
	    e->selection, sname ? sname : Unknown, e->time);

    if (sname) XFree (sname);
}

static void
do_SelectionRequest (XEvent *eventp)
{
    XSelectionRequestEvent *e = (XSelectionRequestEvent *) eventp;
    char *sname = XGetAtomName (e->display, e->selection);
    char *tname = XGetAtomName (e->display, e->target);
    char *pname = XGetAtomName (e->display, e->property);

    printf ("    owner 0x%lx, requestor 0x%lx, selection 0x%lx (%s),\n",
	    e->owner, e->requestor, e->selection, sname ? sname : Unknown);
    printf ("    target 0x%lx (%s), property 0x%lx (%s), time %lu\n",
	    e->target, tname ? tname : Unknown, e->property,
	    pname ? pname : Unknown, e->time);

    if (sname) XFree (sname);
    if (tname) XFree (tname);
    if (pname) XFree (pname);
}

static void
do_SelectionNotify (XEvent *eventp)
{
    XSelectionEvent *e = (XSelectionEvent *) eventp;
    char *sname = XGetAtomName (e->display, e->selection);
    char *tname = XGetAtomName (e->display, e->target);
    char *pname = XGetAtomName (e->display, e->property);

    printf ("    selection 0x%lx (%s), target 0x%lx (%s),\n",
	    e->selection, sname ? sname : Unknown, e->target,
	    tname ? tname : Unknown);
    printf ("    property 0x%lx (%s), time %lu\n",
	    e->property, pname ? pname : Unknown, e->time);

    if (sname) XFree (sname);
    if (tname) XFree (tname);
    if (pname) XFree (pname);
}

static void
do_ColormapNotify (XEvent *eventp)
{
    XColormapEvent *e = (XColormapEvent *) eventp;
    const char *s;
    char sdummy[10];

    switch (e->state) {
      case ColormapInstalled:  s = "ColormapInstalled"; break;
      case ColormapUninstalled:  s = "ColormapUninstalled"; break;
      default:  s = sdummy; sprintf (sdummy, "%d", e->state); break;
    }

    printf ("    colormap 0x%lx, new %s, state %s\n",
	    e->colormap, e->c_new ? Yes : No, s);
}

static void
do_ClientMessage (XEvent *eventp)
{
    XClientMessageEvent *e = (XClientMessageEvent *) eventp;
    char *mname = XGetAtomName (e->display, e->message_type);

    printf ("    message_type 0x%lx (%s), format %d\n",
	    e->message_type, mname ? mname : Unknown, e->format);

    if (mname) XFree (mname);
}

static void
do_MappingNotify (XEvent *eventp)
{
    XMappingEvent *e = (XMappingEvent *) eventp;
    const char *r;
    char rdummy[10];

    switch (e->request) {
      case MappingModifier:  r = "MappingModifier"; break;
      case MappingKeyboard:  r = "MappingKeyboard"; break;
      case MappingPointer:  r = "MappingPointer"; break;
      default:  r = rdummy; sprintf (rdummy, "%d", e->request); break;
    }

    printf ("    request %s, first_keycode %d, count %d\n",
	    r, e->first_keycode, e->count);
    XRefreshKeyboardMapping(e);
}

static void report_action(Widget wi, XEvent *event, String *params, Cardinal *num) {
  switch (event->type) {
  case KeyPress:
    prologue (event, "KeyPress");
    do_KeyPress (event);
    break;
  case KeyRelease:
    prologue (event, "KeyRelease");
    do_KeyRelease (event);
    break;
  case ButtonPress:
    prologue (event, "ButtonPress");
    do_ButtonPress (event);
    break;
  case ButtonRelease:
    prologue (event, "ButtonRelease");
    do_ButtonRelease (event);
    break;
  case MotionNotify:
    prologue (event, "MotionNotify");
    do_MotionNotify (event);
    break;
  case EnterNotify:
    prologue (event, "EnterNotify");
    do_EnterNotify (event);
    break;
  case LeaveNotify:
    prologue (event, "LeaveNotify");
    do_LeaveNotify (event);
    break;
  case FocusIn:
    prologue (event, "FocusIn");
    do_FocusIn (event);
    break;
  case FocusOut:
    prologue (event, "FocusOut");
    do_FocusOut (event);
    break;
  case KeymapNotify:
    prologue (event, "KeymapNotify");
    do_KeymapNotify (event);
    break;
  case Expose:
    prologue (event, "Expose");
    do_Expose (event);
    break;
  case GraphicsExpose:
    prologue (event, "GraphicsExpose");
    do_GraphicsExpose (event);
    break;
  case NoExpose:
    prologue (event, "NoExpose");
    do_NoExpose (event);
    break;
  case VisibilityNotify:
    prologue (event, "VisibilityNotify");
    do_VisibilityNotify (event);
    break;
  case CreateNotify:
    prologue (event, "CreateNotify");
    do_CreateNotify (event);
    break;
  case DestroyNotify:
    prologue (event, "DestroyNotify");
    do_DestroyNotify (event);
    break;
  case UnmapNotify:
    prologue (event, "UnmapNotify");
    do_UnmapNotify (event);
    break;
  case MapNotify:
    prologue (event, "MapNotify");
    do_MapNotify (event);
    break;
  case MapRequest:
    prologue (event, "MapRequest");
    do_MapRequest (event);
    break;
  case ReparentNotify:
    prologue (event, "ReparentNotify");
    do_ReparentNotify (event);
    break;
  case ConfigureNotify:
    prologue (event, "ConfigureNotify");
    do_ConfigureNotify (event);
    break;
  case ConfigureRequest:
    prologue (event, "ConfigureRequest");
    do_ConfigureRequest (event);
    break;
  case GravityNotify:
    prologue (event, "GravityNotify");
    do_GravityNotify (event);
    break;
  case ResizeRequest:
    prologue (event, "ResizeRequest");
    do_ResizeRequest (event);
    break;
  case CirculateNotify:
    prologue (event, "CirculateNotify");
    do_CirculateNotify (event);
    break;
  case CirculateRequest:
    prologue (event, "CirculateRequest");
    do_CirculateRequest (event);
    break;
  case PropertyNotify:
    prologue (event, "PropertyNotify");
    do_PropertyNotify (event);
    break;
  case SelectionClear:
    prologue (event, "SelectionClear");
    do_SelectionClear (event);
    break;
  case SelectionRequest:
    prologue (event, "SelectionRequest");
    do_SelectionRequest (event);
    break;
  case SelectionNotify:
    prologue (event, "SelectionNotify");
    do_SelectionNotify (event);
    break;
  case ColormapNotify:
    prologue (event, "ColormapNotify");
    do_ColormapNotify (event);
    break;
  case ClientMessage:
    prologue (event, "ClientMessage");
    do_ClientMessage (event);
    break;
  case MappingNotify:
    prologue (event, "MappingNotify");
    do_MappingNotify (event);
    break;
  default:
    printf ("Unknown event type %d\n", event->type);
    break;
  }
  
  if (params && num) {
    const char *sep = "";
    for (int i = 0; i < *num; i++) {
      printf ("%s%s", sep, params[i]);
      sep = ", ";
    }
    putchar('\n');
  }
}

