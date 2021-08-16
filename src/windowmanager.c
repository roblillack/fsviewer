#include "windowmanager.h"

void MyWMSetWindowAttributes(Display *dpy, Window window, MyGNUstepWMAttributes *attributes)
{
    Atom atom;

    atom = XInternAtom(dpy, "_GNUSTEP_WM_ATTR", False);
    XChangeProperty(dpy, window, atom, atom, 32, PropModeReplace,
                    (unsigned char *)attributes,
                    sizeof(MyGNUstepWMAttributes) / sizeof(unsigned long));
}