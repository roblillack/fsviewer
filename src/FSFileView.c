#include <limits.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xproto.h>
#include <WMaker.h>
#include <WINGs/WINGsP.h>
#include <X11/Xlocale.h>
#include <X11/keysym.h>
#include <sys/stat.h>
#include <unistd.h>
#include <X11/cursorfont.h>

#include "files.h"
#include "filebrowser.h"
#include "FSFileButton.h"
#include "FSViewer.h"
#include "FSFileView.h"
#include "DnD.h"
#include "FSUtils.h"

#define  MIN_UPPER_HEIGHT  90
#define  MIN_LOWER_HEIGHT  200
#define  FILE_ICON_WIDTH   95
#define  FILE_ICON_HEIGHT  80
#define  PADX              10
#define  PADY              10
#define  PADDED_WIDTH      115
#define  PADDED_HEIGHT     100

static void notificationObserver(void *self, WMNotification *notif);
static void FSAddFileViewShelfItem(FSFileView *fView, FileInfo *fileInfo);
static void handleShelfButtonActions(WMWidget *self, void *data);
static void handleShelfEventActions(XEvent *event, void *data);
static void FSAddFileViewItemIntoShelf(FSViewer *fView);
static void FSRemoveFileViewItemFromShelf(FSFileView *fView, 
					  FSFileIcon *fileIcon);
static void FSSetupFileViewShelfItems(FSFileView *fView);
static void FSReorganiseShelf(FSFileView *fView);
static void handleScrollViewDoubleClick(WMWidget *self, void *clientData);
static Bool FSAddFileViewShelfItemIntoProplist(FSFileView *fView, 
					       FileInfo *fileInfo);
static void handleShelfDrop(XEvent *ev, void *clientData);
static void handleBtnDrop(XEvent *ev, void *clientData);
static void handleBtnDrag(XEvent *ev, void *clientData);
static FSFileIcon *FSFindFileIconWithFileInfo(FSFileView *fView, 
					      FileInfo *fileInfo);

static void
splitViewConstrainCallback(WMSplitView *sPtr, int divIndex, int *min, int *max)
{
    *min = 1;
    *max = WMWidgetHeight(sPtr)-WMGetSplitViewDividerThickness(sPtr)-MIN_UPPER_HEIGHT;
}

static void 
handleScrollViewDoubleClick(WMWidget *self, void *clientData)
{
    char        *pathname;
    FileInfo    *fileInfo;
    FileBrowser *bPtr  = (FileBrowser *)self;
    FSFileView  *fView = (FSFileView *)clientData;

    if( (fileInfo = FSGetFileBrowserSelectedFileInfo(bPtr)) == NULL )
	return;

    pathname = (char *)wmalloc(strlen(fileInfo->path)+
			       strlen(fileInfo->name)+1);
    strcpy(pathname, fileInfo->path);
    strcat(pathname, fileInfo->name);
    
    if(access(pathname, X_OK) == 0)
	LaunchApp(fView->fsViewer, fileInfo, AppExec);
    else if(access(pathname, R_OK) == 0)
	LaunchApp(fView->fsViewer, fileInfo, AppView);
    else if(access(pathname, W_OK) == 0)
	LaunchApp(fView->fsViewer, fileInfo, AppEdit);	
    
    if(pathname)
	free(pathname);
}

static void
notificationObserver(void *self, WMNotification *notif)
{
    FSFileView *fView = (FSFileView*)self;
    void *object = WMGetNotificationObject(notif);
    int divThickness = WMGetSplitViewDividerThickness(fView->split);

    if (WMGetNotificationName(notif) == WMViewSizeDidChangeNotification) 
    {
        if (object == WMWidgetView(fView->fileView)) 
	{
            fView->h = WMWidgetHeight(fView->fileView);
            fView->w = WMWidgetWidth(fView->fileView);

	    WMResizeWidget(fView->split, fView->w, fView->h);
        } 
	else if (object == WMWidgetView(fView->shelfF)) 
	{
	    int w = WMWidgetWidth(fView->shelfF);

	    if(fView->shelfW != w)
	    {
		FSReorganiseShelf(fView);
		fView->shelfW = w;
	    }
        } 
	else if (object == WMWidgetView(fView->fileBrowserF)) 
	{
	    WMResizeWidget(fView->fileBrowser, 
			   WMWidgetWidth(fView->fileBrowserF)-10, 
			   WMWidgetHeight(fView->fileBrowserF)-5);
	}
    }
}

static void
handleKeyPressEvents(XEvent *event, void *data)
{
    char buffer[64];
    KeySym ksym;
    int count;
    Bool meta_pressed  = False;
    FSViewer *fsViewer = (FSViewer *) data;
    FSFileView *fView = FSGetFSViewerCurrentView(fsViewer);

    if (event->xkey.state & FSGetFSViewerMetaMask(fsViewer))
	meta_pressed = True;
    else
	return;

    /* 	FSFileBrowserKeyPress(fView->fileBrowser, event); */

    count = XLookupString(&event->xkey, buffer, 63, &ksym, NULL);
    buffer[count] = '\0';


    switch (ksym)
    {
	/* Lower Case */
    case XK_a : 
	FSSelectAllCB(fsViewer, 0, 0);
	break;
    case XK_c : 
	FSCopyCB(fsViewer, 0, 0);
	break;
    case XK_d : 
	FSCopyCB(fsViewer, 0, 0);
	break;
/*     case XK_e :  */
/* 	FSEjectDisksCB(fsViewer, 0, 0); */
/* 	break; */
    case XK_f : 
	FSFilterViewCB(fsViewer, 0, 0);
	break;
    case XK_h : 
	FSHideCB(NULL, 0, 0);
	break;
    case XK_i :
	FSInspectorCB(fsViewer, 0, 0);
	break;
    case XK_l : 
	FSLinkCB(fsViewer, 0, 0);
	break;
    case XK_m :
	FSMiniaturizeCB(fsViewer, 0, 0);
	break;
    case XK_n :
	FSNewDirCB(fsViewer, 0, 0);
	break;
    case XK_o :
	FSViewCB(fsViewer, 0, 0);
	break;
    case XK_q : 
	FSQuitCB(NULL, NULL);
	break;
    case XK_r : 
	/* Should be Destroy */
	FSDeleteCB(fsViewer, 0, 0);
	break;
    case XK_s : 
	FSSortOrderCB(fsViewer, 0, 0);
	break;
    case XK_u : 
	FSUpdateViewCB(fsViewer, 0, 0);
	break;
    case XK_v : 
	FSPasteCB(fsViewer, 0, 0);
	break;
    case XK_w : 
	FSCloseWindowCB(fsViewer, 0, 0);
	break;
    case XK_x : 
	FSCutCB(fsViewer, 0, 0);
	break;
    case XK_question : 
	FSHelpCB(fsViewer, 0, 0);
	break;
	
	/* Upper Case */
    case XK_B :
	FSBrowseCB(fsViewer, 0, 0);
	break;
    case XK_C : 
	FSConsoleCB(fsViewer, 0, 0);
	break;
    case XK_D : 
	FSDotFilesCB(fsViewer, 0, 0);
	break;
    case XK_E : 
	FSEditCB(fsViewer, 0, 0);
	break;
    case XK_F : 
	FSFinderCB(fsViewer, 0, 0);
	break;
    case XK_I :
	FSIconCB(fsViewer, 0, 0);
	break;
    case XK_L : 
	FSListCB(fsViewer, 0, 0);
	break;
    case XK_N :
	FSNewViewCB(fsViewer, 0, 0);
	break;
    case XK_O :
	FSOpenAsFolderCB(fsViewer, 0, 0);
	break;
    case XK_P :
	FSProcessCB(fsViewer, 0, 0);
	break;
    case XK_R : 
	FSRunCB(fsViewer, 0, 0);
	break;
    case XK_S : 
	FSViewCB(fsViewer, 0, 0);
	break;
    
    default:    /* Do Nothing */
	break;
    }
}

FSFileView *
FSCreateFileView(FSViewer *fsViewer, char *path, Bool primary)
{
    int                   divThickness;
    WMPropList* array      = NULL;
    FSFileView           *fView;
    GNUstepWMAttributes   attributes;

    if (!path)
	return NULL;

    if (!(fView = (FSFileView *) malloc(sizeof(FSFileView))))
	return NULL;
        
    memset((void *) fView, 0, sizeof(FSFileView));

    if (!(fView->path = wstrdup(path)))
    {
	free((void *) fView);
	return NULL;
    }

    fView->dpy = FSGetFSViewerDisplay(fsViewer);
    fView->scr = FSGetFSViewerScreen(fsViewer);
    fView->fsViewer = fsViewer;
    fView->ctrlKey = False;
    fView->fileIcons = NULL;
    fView->dirtyFileIcon = NULL;
/*
    default values from property list:
    164, "XPOS"
    164, "YPOS"
    520, "WSIZE"
    390, "HSIZE"
*/
    fView->x = FSGetIntegerForName("XPOS");
    if (fView->x < 0)
    {
      wwarning(_("Wrong XPOS value: %d. Using default."), fView->x);
      fView->x = 164;
    }
    fView->y = FSGetIntegerForName("YPOS");
    if (fView->y < 0)
    {
      wwarning(_("Wrong YPOS value: %d. Using default."), fView->y);
      fView->y = 164;
    }
    fView->w = FSGetIntegerForName("WSIZE");
    if (fView->w <= 0)
    {
      wwarning(_("Wrong WSIZE value: %d. Using default."), fView->w);
      fView->w = 520;
    }
    fView->h = FSGetIntegerForName("HSIZE");
    if (fView->h <= 0)
    {
      wwarning(_("Wrong HSIZE value: %d. Using default."), fView->h);
      fView->h = 390;
    }

    fView->xcontext = FSGetFSViewerXContext(fsViewer);
    fView->primary = primary;

    fView->fileView = WMCreateWindow(fView->scr, "fileView");
    WMSetWindowTitle(fView->fileView, _("FileView"));
    if ((fView->primary == False))
    {
	WMSetWindowCloseAction(fView->fileView, FSDestroyFileView,
			       (void *) fView);
    }
	
    WMSetViewNotifySizeChanges(WMWidgetView(fView->fileView), True);

    WMResizeWidget(fView->fileView, fView->w, fView->h);
    WMMoveWidget(fView->fileView, fView->x, fView->y);

    fView->split = WMCreateSplitView(fView->fileView);
    WMResizeWidget(fView->split, fView->w, fView->h);
    WMSetSplitViewConstrainProc(fView->split, splitViewConstrainCallback);

    divThickness = WMGetSplitViewDividerThickness(fView->split);

    /* Initialise the FileButton Widget */
    InitFSFileButton(fView->scr);

    /* Initialize drag'n'drop */
    DndInitialize(fView->fileView);

    fView->shelfF = WMCreateFrame(fView->fileView);
    WMSetViewNotifySizeChanges(WMWidgetView(fView->shelfF), True);
    WMSetFrameRelief(fView->shelfF, WRFlat);
    
    fView->fileBrowserF = WMCreateFrame(fView->fileView);
    WMSetViewNotifySizeChanges(WMWidgetView(fView->fileBrowserF), True);
    WMSetFrameRelief(fView->fileBrowserF, WRFlat);

    
    WMAddSplitViewSubview(fView->split, W_VIEW(fView->shelfF));
    WMAddSplitViewSubview(fView->split, W_VIEW(fView->fileBrowserF));

    InitFileBrowser(fView->scr);
    fView->fileBrowser = FSCreateFileBrowser(fView->fileBrowserF);
    WMRealizeWidget(fView->fileView);
    WMMoveWidget(fView->fileBrowser, 5, 0);
    /* watch values for w and h, should not be to small */
    WMResizeWidget(fView->fileBrowser, 
		   fView->w-10, fView->h-5-MIN_UPPER_HEIGHT-divThickness);

    int cw = FSGetIntegerForName("ColumnWidth");
    if (cw < 169)
        cw = 169;
    FSSetFileBrowserColumnWidth(fView->fileBrowser, cw);

    WMResizeWidget(fView->shelfF, fView->w, MIN_UPPER_HEIGHT);
    FSSetupFileViewShelfItems(fView);

    fView->shelfW = fView->w;
    fView->shelfH = MIN_UPPER_HEIGHT;
    WMResizeWidget(fView->fileBrowserF, 
		   fView->w, fView->h-MIN_UPPER_HEIGHT-divThickness);
    WMMoveWidget(fView->fileBrowserF, 0, MIN_UPPER_HEIGHT+divThickness);

/*     WMRealizeWidget(fView->fileView); */

/*     WMRealizeWidget(fView->split); */

    /* Initialise the FileButton Widget */
/*     InitFSFileButton(fView->scr); */
    /* Initialize drag'n'drop */
/*     DndInitialize(fView->fileView); */
    /* Initialize Shelf */
/*     initShelf(fView); */
    /* Initialize File Browser */
/*     initFileBrowser(fView); */
/*     WMAdjustSplitViewSubviews(fView->split); */

    /* register notification observers */
/*     WMAddNotificationObserver(notificationObserver, fView, */
/*                               WMViewSizeDidChangeNotification, */
/*                               WMWidgetView(fView->fileView)); */

    if ((fView->hints = XAllocWMHints()))
    {
	fView->hints->window_group = FSGetFSViewerLeader(fsViewer);
	fView->hints->flags = WindowGroupHint;
	XSetWMHints(fView->dpy, WMWidgetXID(fView->fileView),
		    fView->hints);
    }

    if ((fView->class = XAllocClassHint()))
    {
	fView->class->res_name = "fileViewer";
	fView->class->res_class = "FileViewer";
	XSetClassHint(fView->dpy, WMWidgetXID(fView->fileView),
		      fView->class);
    }

    if ((fView->size = XAllocSizeHints()))
    {
	fView->size->width = fView->w;
	fView->size->height = fView->h;
	fView->size->min_width = fView->w;
	fView->size->max_width =
	    DisplayWidth(fView->dpy, DefaultScreen(fView->dpy)); 
	fView->size->min_height = 250;
	fView->size->max_height =
	    DisplayHeight(fView->dpy, DefaultScreen(fView->dpy));
	fView->size->width_inc = 171;
	fView->size->height_inc = 1;
	fView->size->flags = (USSize | PSize | PMinSize | 
			      PMaxSize | PResizeInc);
	if ((fView->primary == True))
	{
	    fView->size->x = fView->x;
	    fView->size->y = fView->y;
	    fView->size->flags |= (USPosition | PPosition);
	}
	XSetWMNormalHints(fView->dpy, WMWidgetXID(fView->fileView),
			  fView->size);
    }
    
    if ((fView->primary))
    {
	memset((void *) &attributes, 0, sizeof(GNUstepWMAttributes));
	attributes.window_style = (WMTitledWindowMask |
				   WMMiniaturizableWindowMask | 
				   WMResizableWindowMask);
	attributes.window_level = WMNormalWindowLevel;
	attributes.extra_flags = GSFullKeyboardEventsFlag;
	attributes.flags = (GSWindowStyleAttr | GSWindowLevelAttr |
			    GSExtraFlagsAttr);
	WMSetWindowAttributes(fView->dpy, WMWidgetXID(fView->fileView),
			      &attributes);
    }
    
    WMAppAddWindow(FSGetFSViewerWMContext(fsViewer), 
		   WMWidgetXID(fView->fileView));

    FSSetFileBrowserPath(fView->fileBrowser, fView->path); 
    FSSetFileBrowserDoubleAction(fView->fileBrowser, 
				 handleScrollViewDoubleClick, 
				 (void *)fView);

    /* Drag'n'Drop handles */
    DndRegisterDropWidget(fView->shelfF, handleShelfDrop, fView);

    WMMapSubwidgets(fView->fileView); 
    WMMapWidget(fView->shelfF);
    WMMapWidget(fView->fileBrowserF);
    WMMapWidget(fView->fileBrowser);
    WMMapWidget(fView->fileView);

    XSaveContext(fView->dpy, WMWidgetXID(fView->fileView),
		 fView->xcontext,
		 (XPointer) fView);
    FSAddViewToFSViewer(fsViewer, fView);

    WMCreateEventHandler(WMWidgetView(fView->fileView), KeyPressMask,
			 handleKeyPressEvents, fsViewer);

    /* register notification observers */
    WMAddNotificationObserver(notificationObserver, fView,
                              WMViewSizeDidChangeNotification,
                              WMWidgetView(fView->fileView));
    WMAddNotificationObserver(notificationObserver, fView,
                              WMViewSizeDidChangeNotification,
                              WMWidgetView(fView->shelfF));
    WMAddNotificationObserver(notificationObserver, fView,
                              WMViewSizeDidChangeNotification,
                              WMWidgetView(fView->fileBrowserF));

    WMSetWindowMiniwindowPixmap(fView->fileView, 
			       WMGetApplicationIconPixmap(fView->scr));

    array = FSGetUDObjectForKey(defaultsDB, "SHELF");
    if(array && WMIsPLArray(array))
    {
	fView->shelf = WMDeepCopyPropList(array);
    }

    FSUpdateFileViewTitles(fView);

    return fView;
}

void 
FSDestroyFileView(WMWidget *self, void *client)
{
    WMPoint pos;
    WMSize  size;
    FSFileView  *fileView = (FSFileView *) client;

    size = WMGetViewSize(W_VIEW(fileView->fileView));
    pos  = WMGetViewScreenPosition(W_VIEW(fileView->fileView));

    FSSetIntegerForName("XPOS", pos.x);
    FSSetIntegerForName("YPOS", pos.y);
    FSSetIntegerForName("WSIZE", size.width);
    FSSetIntegerForName("HSIZE", size.height);

    XDeleteContext(fileView->dpy, WMWidgetXID(fileView->fileView),
		   fileView->xcontext);
    WMUnmapWidget(fileView->fileView);

    while(fileView->fileIcons != NULL)
    {
	WMUnmapWidget(fileView->fileIcons->btn);
	WMDestroyWidget(fileView->fileIcons->btn);
        fileView->fileIcons = fileView->fileIcons->next;
    }
    fileView->fileIcons = NULL;
    fileView->fileIconCnt = 0;

    if(fileView->dirtyFileIcon)
    {
	WMUnmapWidget(fileView->dirtyFileIcon->btn);
	WMDestroyWidget(fileView->dirtyFileIcon->btn);
	fileView->dirtyFileIcon = NULL;
    }
	
    WMDestroyWidget(fileView->fileView);
    FSRemoveViewFromFSViewer(fileView->fsViewer, fileView);
    free(fileView);

    fileView = NULL;
}

Bool 
FSIsFileViewPrimary(FSFileView *fileView)
{
    return fileView->primary;
}

WMWidget *
FSFileViewWMWidget(FSFileView *fileView)
{
    return fileView->fileView;
}

char *
FSGetFileViewPath(FSFileView *fileView)
{
    return FSGetFileBrowserPath(fileView->fileBrowser);
}

FileInfo *
FSGetFileViewFileInfo(FSFileView *fileView)
{
    return FSGetFileInfo(FSGetFileViewPath(fileView));
}

void
FSUpdateFileViewPath(FSFileView *fileView, FileAction action, 
		     FileInfo *src, FileInfo *dest)
{
    char       *path     = NULL;
    FileInfo   *fileInfo = NULL;
    FSFileIcon *fileIcon = NULL;
    
    switch(action)
	{
	case FileCopy   : 
	    FSUpdateFileBrowser(fileView->fileBrowser, action, src, dest);
	    break;
	case FileMove   :
	case FileDelete : 
	    FSUpdateFileBrowser(fileView->fileBrowser, action, src, dest);
	    fileIcon = FSFindFileIconWithFileInfo(fileView, src);
	    FSRemoveFileViewItemFromShelf(fileView, fileIcon);
	    break;
	case FileRename : 
	    FSUpdateFileBrowser(fileView->fileBrowser, action, src, dest);
	    fileIcon = FSFindFileIconWithFileInfo(fileView, src);
	    FSRemoveFileViewItemFromShelf(fileView, fileIcon);
	    fileInfo = FSGetFileInfo(GetPathnameFromPathName(dest->path, 
							     dest->name));
	    if(fileInfo && fileIcon)
	    {
		if(FSAddFileViewShelfItemIntoProplist(fileView, fileInfo))
		    FSAddFileViewShelfItem(fileView, fileInfo);
		FSFreeFileInfo(fileInfo);
	    }
	    break;
	case FileSync : 
	    FSUpdateFileViewShelf(fileView);
	    path = FSGetFileViewPath(fileView);
	    FSSetFileBrowserPath(fileView->fileBrowser, path);
	    break;
	}

    if(path)
	free(path);
}

void
FSSetFileViewPath(FSFileView *fileView, char *path)
{
    FSSetFileBrowserPath(fileView->fileBrowser, path);
}

void
FSUpdateFileViewTitles(FSFileView *fView)
{
    char *title = NULL;
    char *path  = NULL;

    path = FSGetFileViewPath(fView);
    if(path)
    {
	title = (char *) wmalloc(strlen(path)+12); 
	snprintf(title, strlen(path)+12, "FSViewer - %s", path);
    }
    else
    {
	title = (char *) wmalloc(33);
	snprintf(title, 33, _("FSViewer - we've got a problem!"));
    }

    WMSetWindowMiniwindowTitle(fView->fileView, title);
    WMSetWindowTitle(fView->fileView, title);

    if(path)
	free(path);
    if(title)
	free(title);
}

WMWindow *
FSGetFileViewWindow(FSFileView *fView)
{
    return fView->fileView;
}

void
FSSetFileViewFilter(FSFileView *fView, char *filter)
{
    FSSetFileBrowserFilter(fView->fileBrowser, filter);
    FSUpdateFileViewPath(fView, FileSync, NULL, NULL);
}

char *
FSGetFileViewFilter(FSFileView *fView)
{
    return FSGetFileBrowserFilter(fView->fileBrowser);
}

/****************************************************************************
 *
 * Shelf Related Functions Below
 *
 ****************************************************************************/

static void 
handleShelfDrop(XEvent *ev, void *clientData)
{
    FSFileView *fView = (FSFileView *) clientData;
    FileInfo *fileInfo = NULL;
    unsigned char *data=NULL;
    unsigned long Size;
    unsigned int Type;
    
    Type=DndDataType(ev);
    if( (Type != DndFile) &&  (Type != DndDir)  && 
	(Type != DndLink) && (Type!=DndExe) )
    { 
	return;
    }
    DndGetData(&data,&Size);
    
    fileInfo = FSGetFileInfo(data);
    if(fileInfo) 
    {
	/* 
	 * If fileInfo was successfully entered into the 
	 * array, place it on the shelf
	 */
	if(FSAddFileViewShelfItemIntoProplist(fView, fileInfo))
	    FSAddFileViewShelfItem(fView, fileInfo);
    }

}

static void 
handleShelfEventActions(XEvent *event, void *data)
{
    FSFileView *fView = (FSFileView *)data;
    
    if(event->xkey.state & ControlMask) 
    {
        fView->ctrlKey = True;
    }

}

static void 
handleShelfButtonActions(WMWidget *self, void *data)
{
    int i;
    Bool          found      = False;
    FSFileButton *btn        = (FSFileButton *)self;
    FSFileView   *fView      = (FSFileView *)data;
    FSFileIcon   *fileIcon   = fView->fileIcons;
    
    while(fileIcon != NULL)
    {
	if(btn == fileIcon->btn)
	{
	    found = True;
	    break;
	}
	fileIcon = fileIcon->next;
    }

    if(found)
    {
	if(fView->ctrlKey)
	{
	    FSRemoveFileViewItemFromShelf(fView, fileIcon);
	    fView->ctrlKey = False;
	}
	else
	{
	    char *pathname;
	    FileInfo *fileInfo = FSGetFileButtonFileInfo(fileIcon->btn);

	    if(fileInfo == NULL)
		return;

	    pathname = (char *)malloc(strlen(fileInfo->path)+
				      strlen(fileInfo->name)+1);
	    strcpy(pathname, fileInfo->path);
	    strcat(pathname, fileInfo->name);

	    if(isDirectory(fileInfo->fileType))
		FSSetFileBrowserPath(fView->fileBrowser, pathname);
	    else if(access(pathname, X_OK) == 0)
		LaunchApp(fView->fsViewer, fileInfo, AppExec);
	    else if(access(pathname, R_OK) == 0)
		LaunchApp(fView->fsViewer, fileInfo, AppView);
	    else if(access(pathname, W_OK) == 0)
		LaunchApp(fView->fsViewer, fileInfo, AppEdit);
	}
    }
}

/* Drag'n'Drop */
/* Drop handle for Shelf icon */
static void 
handleShelfButtonDrop(XEvent *ev, void *clientData)
{
    unsigned long Size;
    unsigned int Type,Keys;
    unsigned char *data  = NULL;
    FSFileIcon *fileIcon = (FSFileIcon *) clientData;
    FileInfo   *src      = NULL;
    FileInfo   *dest     = FSGetFileButtonFileInfo(fileIcon->btn);
    FSFileView *fView    = WMGetHangedData(fileIcon->btn);
    
    Type = DndDataType(ev);
    if( (Type!=DndFile) && (Type!=DndFiles) && 
        (Type!=DndExe) && (Type!=DndDir) && (Type!=DndLink)) 
    {
	return;
    }
  
    Keys=DndDragButtons(ev);
  
    DndGetData(&data,&Size);
    if(!data) 
	return;

    if(Type!=DndFiles) 
    {
	char *srcPath  = NULL;
	char *destPath = NULL;

	src=FSGetFileInfo(data);

	srcPath = GetPathnameFromPathName(src->path, src->name);
	destPath = GetPathnameFromPathName(dest->path, dest->name);
	
	if(strcmp(srcPath, destPath) != 0)
	{
	    if(Keys & ShiftMask) /* Copy */
	    {
		wwarning("%s %d", __FILE__, __LINE__);
		FSCopy(src, dest);
	    }
	    else
	    {
/* 		FSFileIcon *fileIcon = NULL; */
		
		FSMove(src, dest);
/* 		fileIcon = FSFindFileIconWithFileInfo(fView, src); */
/* 		if(fileIcon) */
/* 		    FSRemoveFileViewItemFromShelf(fView, fileIcon);	     */
	    }
	}
	if(srcPath)
	    free(srcPath);
	if(destPath)
	    free(destPath);		
    }
}

/* Drag handle for Shelf icon */
static void
handleShelfButtonDrag(XEvent *ev, void *clientData)
{
    int         type;
    char       *path     = NULL;
    WMPixmap   *pixmap   = NULL;
    FSFileIcon *fileIcon = (FSFileIcon *) clientData;
    FileInfo   *fileInfo = FSGetFileButtonFileInfo(fileIcon->btn);
  
    if (!DragStarted) 
	return;
    
    if(!fileInfo)
	return;

    path = GetPathnameFromPathName(fileInfo->path, fileInfo->name);
    type = FSGetDNDType(fileInfo);
    
    DndSetData(type, path, (unsigned long)(strlen(path)+1));

    pixmap = FSCreateBlurredPixmapFromFile(WMWidgetScreen(fileIcon->btn),
					   fileInfo->imgName);
    DndHandleDragging(fileIcon->btn, ev, pixmap);

    if(path) 
	free(path);
    if(pixmap)
	WMReleasePixmap(pixmap);
}

/*
 * Place the fileIcon in the next free slot on the shelf.
 * all very orderly!!
 */
static void
FSPositionItemInShelf(FSFileView *fView, FSFileIcon *fileIcon, int num)
{
    int w, h;
    int row, col;
    int numCol = 0;
    int width = 0;

    w = WMWidgetWidth(fView->shelfF)-25;
    h = WMWidgetHeight(fView->shelfF);
    
    numCol = (int) (w / PADDED_WIDTH);
    row = (int) (num/numCol);
    col = num-(numCol*row);

    WMMoveWidget(fileIcon->btn, 25+(PADDED_WIDTH*(col)), 
		 PADY+(PADDED_HEIGHT*row));

}

/*
  FSReorganiseShelf: Local Function to reorganise shelf layout if
  the contents changed.
*/
static void
FSReorganiseShelf(FSFileView *fView)
{
    int cnt = 0;
    FSFileIcon *tmp    = fView->fileIcons;

    if(fView->dirtyFileIcon)
    {
	WMUnmapWidget(fView->dirtyFileIcon->btn);
	WMDestroyWidget(fView->dirtyFileIcon->btn);
	fView->dirtyFileIcon = NULL;
    }

    if(tmp == NULL)
	return;
    else 
    {
	while(tmp->next)
	{
	    FSPositionItemInShelf(fView, tmp, cnt);
	    cnt++;
	    tmp = tmp->next;
	}
	FSPositionItemInShelf(fView, tmp, cnt);
    }
}

/*
 * FSAddFileViewShelfItem: 
 * Create a fileIcon, add it to FileView->fileIcons
 *  and add it to the shelf after the last fileIcon
*/
static void
FSAddFileViewShelfItem(FSFileView *fView, FileInfo *fileInfo)
{
    char       *pathname;
    FSFileIcon *fileIcon;
    FSFileIcon *tmp = fView->fileIcons;

    if(fileInfo == NULL)
	return;

    fileIcon = (FSFileIcon *) wmalloc(sizeof(FSFileIcon));
    memset(fileIcon, 0, sizeof(FSFileIcon));
    fileIcon->next = NULL;

    /* Create a FileButton */
    fileIcon->btn = FSCreateFileButton(fView->shelfF);
    pathname = GetPathnameFromPathName(fileInfo->path, fileInfo->name);
    FSSetFileButtonPathname(fileIcon->btn, pathname, 0);
    FSSetFileButtonAction(fileIcon->btn, handleShelfButtonActions, fView);
    WMHangData(fileIcon->btn, fView);
    WMRealizeWidget(fileIcon->btn);
    WMMapWidget(fileIcon->btn);


    /* Drag'n'Drop */
    DndRegisterDropWidget(fileIcon->btn, handleShelfButtonDrop, fileIcon);
    DndRegisterDragWidget(fileIcon->btn, handleShelfButtonDrag, fileIcon);

    /* Add the new Btn to the fileIcon linked list */
    if(fView->fileIcons == NULL)
	fView->fileIcons = fileIcon;
    else 
    {
	while(tmp->next)
	    tmp = tmp->next;
	
	fileIcon->next = tmp->next;
	tmp->next = fileIcon;
    }

    /* Position the btn in the shelf */
    FSPositionItemInShelf(fView, fileIcon, fView->fileIconCnt);
    fView->fileIconCnt++;

    /* 
     * Add a Button Press handler, 
     * used to check for control key presses
     */
    WMCreateEventHandler(WMWidgetView(fileIcon->btn), 
			 ButtonPressMask, 
			 handleShelfEventActions, fView);
}

/*
 * FSAddFileViewItemIntoShelf: 
 * Add the selected path to the shelf and
 * update the SHELF entry in the user defaults DB
*/
static void
FSAddFileViewItemIntoShelf(FSViewer *fsViewer)
{
    FSFileView *fView    = NULL;
    FileInfo   *fileInfo = NULL;

    /* Get the current path fileInfo */
    fView    = FSGetFSViewerCurrentView(fsViewer);
    fileInfo = FSGetFileBrowserSelectedFileInfo(fView->fileBrowser);

    if(fileInfo == NULL)
	return;

    if(FSAddFileViewShelfItemIntoProplist(fView, fileInfo))
    {
	/* 
	 * If fileInfo was successfully entered into the 
	 * array, place it on the shelf
	 */
	FSAddFileViewShelfItem(fView, fileInfo);
    }
}

/*
 * FSAddFileViewShelfItemIntoProplist:
 * Add the fileInfo into the user DC, if it was successfully
 * entered return False, if the fileInfo is already in there 
 * return true.
 */
static Bool
FSAddFileViewShelfItemIntoProplist(FSFileView *fView, FileInfo *fileInfo)
{
    Bool        notFound;
    char       *pathname = NULL;
    WMPropList*  val      = NULL;
    WMPropList*  array    = NULL;

    /*
     * Get the Shelf entry and insert the pathname into
     * the array
     */
    pathname = GetPathnameFromPathName(fileInfo->path, fileInfo->name);
    val = WMCreatePLString(pathname);
    array = FSGetUDObjectForKey(defaultsDB, "SHELF");
    if (!array)
    {
	array = WMCreatePLArray(WMCreatePLString(wgethomedir()), NULL );
	WMSetUDObjectForKey(defaultsDB, array, "SHELF");
    }	
    notFound = InsertArrayElement(array, val);

    /* 
     * If val hasn't been found in the original array
     * it is inserted, therefore we want to update the
     * shelf array.
     */
    if(notFound)
    {
	WMSetUDObjectForKey(defaultsDB, array, "SHELF");
	
	WMReleasePropList(fView->shelf);
	fView->shelf = WMDeepCopyPropList(array);
    }

    if(pathname)
	free(pathname);

    return notFound;
}

/*
 * Remove the fileIcon from the shelf and the proplist entry
 */ 
static void
FSRemoveFileViewItemFromShelf(FSFileView *fView, FSFileIcon *fileIcon)
{
    char       *pathname = NULL;
    WMPropList*  key      = NULL;
    WMPropList*  val      = NULL;
    WMPropList*  array    = NULL;
    FileInfo   *fileInfo = NULL;
    FSFileIcon *tmp = fView->fileIcons;

    if(fileIcon == NULL)
	return;
    /*
      The following code is needed to remove the btn to
      prevent a seg fault when the btn widget is destroyed
      and the calling function exits. The procedure is as follows:
      a) unmap the btn and remove it from the list
      b) Remove the shelf item from the proplist entry
      c) Reorganise the shelf with the updated list.
      d) Set dirtyFileIcon so that it will be removed on the next
         reorganise shelf call or when the FileViewer is destroyed.

      To check that it seg faults at the end of this function insert
      WMWidgetDestroy(fileIcon->btn);
    */
    /* Required by dirtyIcon...*/
    WMUnmapWidget(fileIcon->btn);
    if(tmp != fileIcon)
    {
	while(tmp->next != fileIcon)
	    tmp = tmp->next;
	tmp->next = tmp->next->next;
    }
    else
    {
	tmp = tmp->next;
	fView->fileIcons = tmp;
    }
    fView->fileIconCnt--;
    /*...dirtyIcon*/
    
    fileInfo = FSGetFileButtonFileInfo(fileIcon->btn);
    pathname = GetPathnameFromPathName(fileInfo->path, fileInfo->name);
    val = WMCreatePLString(pathname);
    array = FSGetUDObjectForKey(defaultsDB, "SHELF");

    if (array && WMIsPLArray(array))
    {
	FSRemoveArrayElement(array, val);	    
    }
    WMSetUDObjectForKey(defaultsDB, array, "SHELF");

    WMReleasePropList(fView->shelf);
    fView->shelf = WMDeepCopyPropList(array);

    FSReorganiseShelf(fView);
    fView->dirtyFileIcon = fileIcon;
    
    if(key)
	WMReleasePropList(key);
    if(pathname)
	free(pathname);
}

/*
  Read the proplist entry for SHELf and add the necessary fileIcons to
  the shelf
*/
static void
FSSetupFileViewShelfItems(FSFileView *fView)
{
    WMPropList* key     = NULL;
    WMPropList* array   = NULL;

    array = FSGetUDObjectForKey(defaultsDB, "SHELF");
    if(array && WMIsPLArray(array))
    {
	char *path;
	FileInfo *fileInfo = NULL;
	int numElem, i;
	
	numElem = WMGetPropListItemCount(array);
	for(i = 0; i < numElem; i++)
	{	
	    path = WMGetFromPLString(WMGetFromPLArray(array, i));
	    fileInfo = FSGetFileInfo(path);
	    FSAddFileViewShelfItem(fView, fileInfo);
	}
    }
}
/*
  Update the Shelf. This is used when the focus moves between FileView
  windows. The local proplist shelf copy is compared with the filesDB copy,
  if they are different, the shelf and fileIcons list is regenerated.

  The local shelf copy provides a method to check if the global shelf is
  dirty. The local copy has no other function. 
*/
void
FSUpdateFileViewShelf(FSFileView *fView)
{
    WMPropList* key     = NULL;
    WMPropList* array   = NULL;
    FSFileIcon *tmp    = fView->fileIcons;

    /* rid ourselves of the dirty File Icon */
    if(fView->dirtyFileIcon)
    {
	WMUnmapWidget(fView->dirtyFileIcon->btn);
	WMDestroyWidget(fView->dirtyFileIcon->btn);
	fView->dirtyFileIcon = NULL;
    }

    /* get the global array */
    array = FSGetUDObjectForKey(defaultsDB, "SHELF");
    if(array && WMIsPLArray(array))
    {
	char *path;
	FileInfo *fileInfo = NULL;
	int numElem, i;
	
	/* Check to see if they are different */
	if(WMIsPropListEqualTo(array, fView->shelf) == False)
	{
	    /* they are, so clear the shelf */
	    while(tmp != NULL)
	    {
		WMUnmapWidget(tmp->btn);
		WMDestroyWidget(tmp->btn);
		tmp = tmp->next;
	    }
	    fView->fileIcons = NULL;
	    fView->fileIconCnt = 0;
	    
	    /* reload the shelf */
	    numElem = WMGetPropListItemCount(array);
	    for(i = 0; i < numElem; i++)
	    {	
		path = WMGetFromPLString(WMGetFromPLArray(array, i));
		fileInfo = FSGetFileInfo(path);
		FSAddFileViewShelfItem(fView, fileInfo);
	    }
	    
	    WMReleasePropList(fView->shelf);
	    fView->shelf = WMDeepCopyPropList(array);
	}
    }
}

/*
 * Find the fileIcon with the given fileInfo 
 */
static FSFileIcon *
FSFindFileIconWithFileInfo(FSFileView *fView, FileInfo *fileInfo)
{
    FileInfo   *fI     = NULL;
    FSFileIcon *tmp    = fView->fileIcons;
    FSFileIcon *theOne = NULL;

    while(tmp)
    {
	fI = FSGetFileButtonFileInfo(tmp->btn);
	if( strcmp(fileInfo->name, fI->name) == 0 &&
	    strcmp(fileInfo->path, fI->path) == 0   )
	{
	    /* found the right fileIcon */
	    theOne = tmp;
	    break;
	}
	else
	    tmp = tmp->next;
   }

    return theOne;
}

void
FSSetFileViewMode(FSFileView *fView, FSFileViewType mode)
{

    switch(mode)
    {
    case Browser : FSSetFileBrowserMode(fView->fileBrowser, 0);
	break;
    case Icon    : FSSetFileBrowserMode(fView->fileBrowser, 1);
	break;
    case List    : FSSetFileBrowserMode(fView->fileBrowser, 2);
	break;
    default      : FSSetFileBrowserMode(fView->fileBrowser, 0);
	break;
    }
}
