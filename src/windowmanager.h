#ifndef _WINDOWMANAGER_H_
#define _WINDOWMANAGER_H_

#include <X11/Xlib.h>

typedef struct
{
    unsigned long flags;
    unsigned long window_style;
    unsigned long window_level;
    unsigned long reserved;
    Pixmap miniaturize_pixmap; /* pixmap for miniaturize button */
    Pixmap close_pixmap;       /* pixmap for close button */
    Pixmap miniaturize_mask;   /* miniaturize pixmap mask */
    Pixmap close_mask;         /* close pixmap mask */
    unsigned long extra_flags;
} MyGNUstepWMAttributes;

void MyWMSetWindowAttributes(Display *dpy, Window window,
                             MyGNUstepWMAttributes *attributes);

#endif