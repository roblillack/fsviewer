#ifndef FSVIEWER_H_
#define FSVIEWER_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <X11/Xlib.h>
#include <WMaker.h>

#include <wraster.h>

#include <WINGs/WINGs.h>
#include <WINGs/WUtil.h>

#include "files.h"

#define INITIALIZED_PANEL	(1<<0)
/* #define FSVERSION	        "0.2.3b" */
#define WMVERSION               "0.80.0"

#ifndef RM_DITHER
#define RM_DITHER 0 /* Taken from wraster.h for WM v0.53.0 */
#endif

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#ifdef ENABLE_NLS
# include <X11/Xlocale.h>
# include <libintl.h>
# define _(text) gettext(text)
#else
# define _(text) (text)
#endif

typedef struct _FSFView  FSFileView;
typedef struct _FSFind FSFinder;
typedef struct _Panel Panel;

typedef enum ClipAction {
    ClipCopy,
    ClipCut,
    ClipRename,
    ClipLink,
    ClipPaste
} ClipAction;


typedef struct _FSViewer 
{    
    Display        *dpy;
    Window          leader;
    XClassHint     *class;
    XWMHints       *hints;
    XContext        xContext;
    WMScreen       *scr;
    WMAppContext   *wmContext;
    WMMenu         *menu;

    char           *initPath;
    /* Application icons */
    RContext       *rcontext;
    WMPixmap       *wmpixmap;
    RImage         *image;
    Pixmap          appicon;
    Pixmap          appmask;
    
    FSFileView     *current;
    unsigned short  nviews;

    int             metaMask;

    FSFinder       *finder;

    FileInfo       *clip;
    enum ClipAction clipAction;

} _FSViewer;

typedef struct _FSViewer FSViewer;


void          FSInitInspector(FSViewer *fsViewer);
void          FSShowInspectorWindow(WMScreen *scr, FileInfo *fileInfo);
void          FSUpdateInspectorWindow(FileInfo *fileInfo);
void          FSHideInspectorWindow(WMScreen *scr);
WMScreen     *FSGetFSViewerScreen(FSViewer *fsViewer);
WMAppContext *FSGetFSViewerWMContext(FSViewer *fsViewer);
Display      *FSGetFSViewerDisplay(FSViewer *fsViewer);
XContext      FSGetFSViewerXContext(FSViewer *fsViewer);
RContext     *FSGetFSViewerRContext(FSViewer *fsViewer);
Window        FSGetFSViewerLeader(FSViewer *fsViewer);
void          FSAddViewToFSViewer(FSViewer *fsViewer, FSFileView *fView);
void          FSRemoveViewFromFSViewer(FSViewer *fsViewer, FSFileView *fView);
FSFileView   *FSGetFSViewerCurrentView(FSViewer *fsViewer);
void          FSSetFSViewerTransientWindow(FSViewer *fsViewer, Window window);
void          FSAddWindow(FSViewer *fsViewer, Window window);
void          FSSetFSViewerConfirmWindow(FSViewer *fsViewer, Window window);
Bool          FSIsFSViewerClipSet(FSViewer *fsViewer);
void          FSSetFSViewerClip(FSViewer *fsViewer, FileInfo *fileInfo);
FileInfo     *FSGetFSViewerClip(FSViewer *fsViewer);
void          FSSetFSViewerClipAction(FSViewer *fsViewer, ClipAction action);
ClipAction    FSGetFSViewerClipAction(FSViewer *fsViewer);
void          FSUpdateCurrentFileViewTitles();
int           FSGetFSViewerMetaMask(FSViewer *fsViewer);
void          FSSetFSViewerFinder(FSViewer *fsViewer, FSFinder *finder);
FSFinder     *FSGetFSViewerFinder(FSViewer *fsViewer);
void          FSSetFSViewerPath(FSViewer *fsViewer, char *path);
char         *FSGetFSViewerPath(FSViewer *fsViewer);

typedef struct {
    unsigned flags;		  /* reserved. Don't access it */
    
    void (*createWidgets)(Panel*);/* called when showing for first time */
    void (*updateDomain)(Panel*); /* save the changes to the dictionary */
    void (*updateDisplay)(Panel*);
    Bool (*validate)(Panel*);	  /* do validation checks if necessary */
    void (*undoChanges)(Panel*);  /* reset values to those in the dictionary */
    char* (*getPathname)(Panel*); /* get the pathname from the current panel */
} CallbackRec;

/* Application defaults */
WMUserDefaults *defaultsDB;
WMPropList *filesDB;

/* all Panels must start with the following layout */
typedef struct PanelRec {
    WMFrame *frame;

    char *sectionName;		       /* section name to display in titlebar */
    
    CallbackRec callbacks;
} PanelRec;


void        magic_parse_file(char *name);
void        magic_get_type(char *name, char *buf);

WMPropList* GetDictObject(WMPropList* dictKey, WMPropList* valKey);
WMPropList* GetCmdForExtn(char *extn, char *cmd);
char       *GetExecStringForExtn(char *extn);
char       *GetViewerStringForExtn(char *extn);
char       *GetEditorStringForExtn(char *extn);
char       *GetIconStringForExtn(char *extn);
char       *LocateImage(char *name);
Bool        InsertArrayElement(WMPropList* array, WMPropList* val);
WMPropList* FSRemoveArrayElement(WMPropList* array, WMPropList* val);
void        InitFilesDB(FSViewer *fsViewer);
char       *FSGetStringForNameKey(char *name, char *key);
char       *FSGetStringForName(char *name);
void        FSSetStringForNameKey(char *name, char *dictKey, char *str);
int         FSGetIntegerForName(char *name);
void        FSSetIntegerForName(char *name, int val);
void        FSSetStringForName(char *name, char *str);
WMPropList* FSGetArrayForNameKey(char *name, char *key);
WMPropList* FSGetDBObjectForKey(WMPropList* dict, char *key);
WMPropList* FSGetUDObjectForKey(WMUserDefaults *database, char *defaultName);

Panel* InitAttribs    (WMWindow *win, FileInfo *fileInfo);
Panel* InitViewer     (WMWindow *win, FileInfo *fileInfo);
Panel* InitEditor     (WMWindow *win, FileInfo *fileInfo);
Panel* InitExecutable (WMWindow *win, FileInfo *fileInfo);
Panel* InitExtn(WMWindow *win, FSViewer *app, FileInfo *fileInfo);
Panel* InitIcon(WMWindow *win, FSViewer *app, 
		FileInfo *fileInfo, int x, int y);

#define FRAME_TOP	5
#define FRAME_LEFT	5
#define FRAME_WIDTH	510
#define FRAME_HEIGHT	380

#endif
