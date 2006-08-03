#ifndef _FSFINDER_H_
#define _FSFINDER_H_

typedef struct _FSFind
{
    Display        *dpy;
/*     XContext        xcontext; */
    XWMHints       *hints;
    XClassHint     *class;
    XSizeHints     *size;
    
    FSViewer       *fsViewer;
    WMScreen       *scr;
    WMWindow       *win;

    WMFrame        *mainF;
    WMLabel        *label;        
    WMList         *list;
    WMButton       *findRadioBtns[2];
    WMTextField    *searchField;

    int             x, y;
    unsigned short  w, h;

} _FSFind;

FSFinder *FSCreateFinder(FSViewer *fsViewer);
void      FSDestroyFinder(WMWidget *self, void *client);
WMWindow *FSGetFinderWindow(FSFinder *finder);

#endif /* _FSFINDER_H_ */
