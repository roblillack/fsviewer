#ifndef _FSFILEVIEW_H_
#define _FSFILEVIEW_H_

typedef enum _FSFileViewType
{
        Browser,
        Icon,
        List
} FSFileViewType;

typedef struct _FSFView 
{
    Display                 *dpy;
    XContext                 xcontext;
    XWMHints                *hints;
    XClassHint              *class;
    XSizeHints              *size;
    
    FSViewer                *fsViewer;
    WMScreen                *scr;
    WMWindow                *fileView;

    WMSplitView             *split;

    /* File Browser */
    WMFrame                 *fileBrowserF;
    FileBrowser             *fileBrowser;

    /* Shelf components */
    WMFrame                 *shelfF;
    WMPropList*              shelf;
    FSFileIcon              *fileIcons;
    FSFileIcon              *dirtyFileIcon;
    int                      fileIconCnt;

    FSFileViewType          *viewType;
    Bool                     primary;
    char                    *path;
    unsigned short           w, h;
    int                      x, y;
    int                      shelfW, shelfH;

    Bool                     ctrlKey;

} _FSFView;

FSFileView *FSCreateFileView(FSViewer *fsViewer, char *path, Bool primary);
void        FSDestroyFileView(WMWidget *self, void *client);
Bool        FSIsFileViewPrimary(FSFileView *fileView);
WMWidget   *FSFileViewWMWidget(FSFileView *fileView);
char       *FSGetFileViewPath(FSFileView *fileView);
void        FSUpdateFileViewPath(FSFileView *fileView, FileAction action, 
				 FileInfo *src, FileInfo *dest);
FileInfo   *FSGetFileViewFileInfo(FSFileView *fileView);
void        FSUpdateFileViewShelf(FSFileView *fView);
void        FSUpdateFileViewTitles(FSFileView *fView);
WMWindow   *FSGetFileViewWindow(FSFileView *fView);
void        FSSetFileViewFilter(FSFileView *fView, char *filter);
char       *FSGetFileViewFilter(FSFileView *fView);
void        FSSetFileViewPath(FSFileView *fileView, char *path);
void        FSSetFileViewMode(FSFileView *fView, FSFileViewType mode);

#endif
