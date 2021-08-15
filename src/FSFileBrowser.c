#include <WMaker.h>
#include <WINGs/WINGs.h>
#include <WINGs/WINGsP.h>
#include <sys/stat.h>
#include <unistd.h>

#include "files.h"
/* #include "FSFileButton.h" */
#include "FSPathView.h"
#include "filebrowser.h"
#include "FSViewer.h"
#include "FSBrowser.h"
#include "FSUtils.h"
#include "FSMCList.h"
#include "DnD.h"

#define LIST_WIDTH              170
#define COLUMN_SPACING 	        4
#define TITLE_SPACING           2

#define DEFAULT_WIDTH		510
#define DEFAULT_HEIGHT		390

#define PV_HEIGHT               110
#define PVICON_WIDTH            169
#define PADY                    9

#define DEFAULT_SEPARATOR	"/"

#define BROWSE                  0
#define ICON                    1
#define LIST                    2

typedef struct W_FileBrowser {
    W_Class       widgetClass;
    WMView       *view;

    WMView       *viewport;

    WMFrame      *displayFrame;

    int           columnCount;
    int           selectedCol;

    int           maxVisibleColumns;
    int           firstVisibleColumn;

    short         lineScroll;
    short         pageScroll;

    int           columnWidth;

    void         *clientData;
    WMAction     *action;
    void         *doubleClientData;
    WMAction     *doubleAction;

    FSPathView   *pathView;
    WMFrame      *pathViewF;

    char         *filter;
    FSBrowser    *browser;
    WMView       *icon;
    WMFrame      *listF;
    FSMCList     *list;

    char         *text;

    int           currentMode;

    struct {
	unsigned int isTitled:1;
	unsigned int allowMultipleSelection:1;
	
	unsigned int loaded:1;
	unsigned int loadingColumn:1;
    } flags;
} _FileBrowser;

static void handleEvents(XEvent *event, void *data);
static void destroyFileBrowser(FileBrowser *bPtr);
static void setupPathView(FileBrowser *bPtr, int x, int y, 
			     int width, int height);
static void setupBrowserMode(FileBrowser *bPtr, int x, int y, 
			     int width, int height);
static void setupListMode(FileBrowser *bPtr, int x, int y, 
			     int width, int height);
static void setupIconMode(FileBrowser *bPtr);
/* static void resizeFileBrowser(WMWidget*, unsigned int, unsigned int); */
static void resizePathView(FileBrowser *bPtr, unsigned int, unsigned int);
static void scrollPathView(FileBrowser *bPtr, int col);
static void scrolledBrowser(void *observerData, WMNotification *notification);
static void scrolledPathView(void *observerData, WMNotification *notification);
static void scrollCallback(WMWidget *self, void *clientData);
/* static void fillPathView(FileBrowser *bPtr, int column); */
static void handleDoubleActions(WMWidget *self, void *data);
static void handleItemSelection(WMWidget *self, void *clientData);
static int  FSGetFileBrowserFirstVisibleColumn(FileBrowser *bPtr);
static int  FSGetFileBrowserColumnCount(FileBrowser *bPtr);
static FileInfo *FSGetFileBrowserFileInfoAt(FileBrowser *bPtr, int col);
static void insertIntoColumn(FileBrowser *bPtr, char *pathname);
static void removeFromColumn(FileBrowser *bPtr, char *path, char *name);
static int  findColumnFromPath(char *path);
static void notificationObserver(void *self, WMNotification *notif);
static void removeFromView(FileBrowser *bPtr, char *path, char *name);
static void insertIntoView(FileBrowser *bPtr, char *pathname);

static void willResizeFileBrowser(W_ViewDelegate*, WMView*,
				  unsigned int*, unsigned int*);

/* W_ViewProcedureTable _FileBrowserViewProcedures = { */
/*     NULL, */
/* 	resizeFileBrowser, */
/* 	NULL */
/* }; */

W_ViewDelegate _FileBrowserViewDelegate = {
    NULL,
        NULL,
        NULL,
        NULL,
        willResizeFileBrowser
};

// Widget class ID.
static	W_Class	fileBrowserWidgetClass = 0;

W_Class InitFileBrowser(WMScreen *scr)
{
    // Register the widget with WINGs and get the widget class ID.
    if (!(fileBrowserWidgetClass))
    {
	fileBrowserWidgetClass =
/* 	    W_RegisterUserWidget(&_FileBrowserViewProcedures); */
	    W_RegisterUserWidget();
    }

    return fileBrowserWidgetClass;
}


FileBrowser*
FSCreateFileBrowser(WMWidget *parent)
{
    FileBrowser *bPtr;
    int i;

    bPtr = wmalloc(sizeof(FileBrowser));
    memset(bPtr, 0, sizeof(FileBrowser));

    bPtr->widgetClass = fileBrowserWidgetClass;
    
    bPtr->view = W_CreateView(W_VIEW(parent));
    if (!bPtr->view) {
	free(bPtr);
	return NULL;
    }
    bPtr->view->self = bPtr;
    bPtr->view->delegate = &_FileBrowserViewDelegate;

    WMCreateEventHandler(bPtr->view, ExposureMask|StructureNotifyMask
			 |ClientMessageMask, handleEvents, bPtr);
	
   /* default configuration */
    bPtr->lineScroll = bPtr->columnWidth;
    bPtr->pageScroll = 0;

    bPtr->firstVisibleColumn = 0;
    bPtr->maxVisibleColumns = 3;
    bPtr->selectedCol = -1;
    bPtr->columnCount = 0;
    bPtr->columnWidth = PVICON_WIDTH;

    bPtr->displayFrame = WMCreateFrame(bPtr);
    WMSetFrameRelief(bPtr->displayFrame, WRFlat);
    WMMoveWidget(bPtr->displayFrame, 0, 119);
    WMResizeWidget(bPtr->displayFrame, 510, 275);  
    setupBrowserMode(bPtr, 0, 0, 510, 275);
    setupListMode(bPtr, 0, 0, 510, 275);

    setupPathView(bPtr, 0, 0, DEFAULT_WIDTH, PV_HEIGHT);

/*     resizeFileBrowser(bPtr, DEFAULT_WIDTH, DEFAULT_HEIGHT); */
    WMResizeWidget(bPtr, DEFAULT_WIDTH, DEFAULT_HEIGHT);
    WMMapWidget(bPtr->pathView);
    WMMapWidget(bPtr->browser);
/*     WMMapWidget(bPtr->listF); */
    WMMapWidget(bPtr->displayFrame);
    bPtr->icon = NULL;
/*     bPtr->currentMode = LIST; */
    bPtr->currentMode = BROWSE;
    bPtr->filter = NULL;
    bPtr->doubleAction = NULL;
    bPtr->doubleClientData = NULL;
 
    return bPtr;
}

static void 
willResizeFileBrowser(W_ViewDelegate *self, WMView *view,
		      unsigned int *width, unsigned int *height)
{
    FileBrowser *bPtr = (FileBrowser *) view->self;

    bPtr->maxVisibleColumns = *width/bPtr->columnWidth;

    WMResizeWidget(bPtr->displayFrame, 
		   *width, *height-PV_HEIGHT-PADY);

/*     W_ResizeView(bPtr->view, *width, *height); */

    if(bPtr->browser)
    {
	FSSetBrowserMaxVisibleColumns(bPtr->browser, bPtr->maxVisibleColumns);
	WMResizeWidget(bPtr->browser, 
		       *width, *height-PV_HEIGHT-PADY);
    }
    if(bPtr->list)
    {
	WMResizeWidget(bPtr->listF, *width, *height-PV_HEIGHT-PADY);
	FSResizeMCList(bPtr->list,  *width, *height-PV_HEIGHT-PADY);
    }
    FSSetPathViewMaxVisibleColumns(bPtr->pathView, 
				   bPtr->maxVisibleColumns);
    WMResizeWidget(bPtr->pathViewF, *width,   PV_HEIGHT);
    WMResizeWidget(bPtr->pathView,  *width-3, PV_HEIGHT-3);
}

/* static void  */
/* resizeFileBrowser(WMWidget *w, unsigned int width, unsigned int height) */
/* { */
/*     FileBrowser *bPtr = (FileBrowser*)w; */

/*     bPtr->maxVisibleColumns = width/bPtr->columnWidth; */

/*     WMResizeWidget(bPtr->displayFrame,  */
/* 		   width, height-PV_HEIGHT-PADY); */

/*     W_ResizeView(bPtr->view, width, height); */

/*     if(bPtr->browser) */
/*     { */
/* 	FSSetBrowserMaxVisibleColumns(bPtr->browser, bPtr->maxVisibleColumns); */
/* 	WMResizeWidget(bPtr->browser, width, height-PV_HEIGHT-PADY); */
/*     } */
/*     if(bPtr->list) */
/*     { */
/* 	WMResizeWidget(bPtr->listF, width, height-PV_HEIGHT-PADY); */
/* 	FSResizeMCList(bPtr->list,  width, height-PV_HEIGHT-PADY); */
/*     } */

/*     FSSetPathViewMaxVisibleColumns(bPtr->pathView,  */
/* 				   bPtr->maxVisibleColumns); */
/*     WMResizeWidget(bPtr->pathViewF, width,   PV_HEIGHT); */
/*     WMResizeWidget(bPtr->pathView,  width-3, PV_HEIGHT-3); */
/* } */

static void 
fillBrowserColumn(FSBrowser *bPtr, int column)
{
    char *path;
    FileInfo *fileList;
    FileInfo *start;
    WMListItem *listItem;
    FileBrowser *fB = (FileBrowser *)(WMGetHangedData(bPtr));

    FSSetBusyCursor(bPtr, True);

    path = FSGetBrowserPathToColumn(bPtr, column);
    if(path)
    {
	start = GetDirList(path);
	fileList = start;

	while(fileList != NULL)
	{
	    if( DisplayFile(fileList->name, fB->filter, fileList->fileType) )
	    {
		listItem = FSInsertBrowserItem(bPtr, column, -1, 
					       fileList->name,
					       isDirectory(fileList->fileType));
/* 		listItem->clientData = fileList; */
	    }

	    fileList = fileList->next;
	}
    }

    if (path)
    {
	free(path);
    }

    if(start)
	FSFreeFileInfo(start);

    FSSetBusyCursor(bPtr, False);
}

static void 
handleItemSelection(WMWidget *self, void *clientData)
{
    int col;
    char *path;
    FileBrowser *bPtr = (FileBrowser *) clientData;
    static int lastCol = 0;
    WMListItem *item = NULL;
    FileInfo *fileInfo = NULL;

    col = FSGetFileBrowserColumnCount(bPtr);
    fileInfo = FSGetFileBrowserSelectedFileInfo(bPtr);
    switch(bPtr->currentMode)
    {
    case BROWSE :
        FSSetPathViewToColumn(bPtr->pathView, col-1);
        /* Update Inspector if open */
	FSUpdateInspectorWindow(fileInfo);
	break;
    case ICON   : 
	break;
    case LIST   : 
	if(lastCol >= col && !isDirectory(fileInfo->fileType))
	    FSSetPathViewToColumn(bPtr->pathView, col);
	else
	    FSSetPathViewToColumn(bPtr->pathView, col-1);
	lastCol = col;
        /* Update Inspector if open */
	FSUpdateInspectorWindow(fileInfo);
	break;
    default     : 
	break;
    }

    if(bPtr->text)
    {
	wwarning("%s %d: %s", __FILE__, __LINE__, bPtr->text);
	free(bPtr->text);
	bPtr->text = NULL;
    }
}

static void 
handlePathViewAction(WMWidget *self, void *clientData)
{
    int col, i;
    FileBrowser *bPtr = (FileBrowser *) clientData;
    /* 
       Will be used to help in the scolling when
       the icon and list views have been implemented
    */
    col = FSGetPathViewLastClicked(self);
    i = FSGetPathViewNumberOfColumns(self) - 1;

/*     if(bPtr->currentMode == BROWSE) */
/* 	FSScrollBrowserToColumn(bPtr->browser, col-1, False); */
/*     else  */
    if(bPtr->currentMode == LIST && col < i)
	FSScrollMCListToColumn(bPtr->list, col, False);
}

static void 
handleDoubleAction(WMWidget *self, void *clientData)
{
    FileBrowser *bPtr = (FileBrowser *) clientData;

    if (bPtr->doubleAction) 
	(*bPtr->doubleAction)(bPtr, bPtr->doubleClientData);
}

static void 
fillPV(FSPathView *pvPtr, int column)
{
    char *path;
    FileBrowser *bPtr = (FileBrowser *)(WMGetHangedData(pvPtr));

    path = FSGetFileBrowserPathToColumn(bPtr, column-1);
    if(path)
    {
	int backlight = FSGetIntegerForName("DisplaySVBG");

	FSSetPathViewColumnContents(pvPtr, column, path, 1, backlight);
	free(path);
    }
    FSUpdateCurrentFileViewTitles();
}

static void
setupPathView(FileBrowser *bPtr, int x, int y, int width, int height)
{
    InitFSPathView(WMWidgetScreen(bPtr));
    
    bPtr->pathViewF = WMCreateFrame(bPtr);
    WMResizeWidget(bPtr->pathViewF, width, height);
    WMMoveWidget(bPtr->pathViewF, x, y);
    WMSetFrameRelief(bPtr->pathViewF, WRSunken);
    WMMapWidget(bPtr->pathViewF);

    bPtr->pathView = FSCreatePathView(bPtr->pathViewF);
    WMMoveWidget(bPtr->pathView, x+2, y+2);
    WMResizeWidget(bPtr->pathView, width-3, height-3);
    FSSetPathViewFillColumnProc(bPtr->pathView, fillPV); 
    FSSetPathViewAction(bPtr->pathView, handlePathViewAction, bPtr);
    FSSetPathViewDoubleAction(bPtr->pathView, handleDoubleAction, bPtr);
    WMAddNotificationObserver(scrolledPathView, bPtr,  
			      FSPathViewDidScrollNotification, 
			      bPtr->pathView);
    WMHangData(bPtr->pathView, bPtr);
}

static void
setupBrowserMode(FileBrowser *bPtr, int x, int y, int width, int height)
{
    InitFSBrowser(WMWidgetScreen(bPtr->displayFrame));
    bPtr->browser = FSCreateBrowser(bPtr->displayFrame);

    FSSetBrowserHasScroller(bPtr->browser, False);
    FSSetBrowserTitled(bPtr->browser, False);
    WMMoveWidget(bPtr->browser, x, y); 
    WMResizeWidget(bPtr->browser, width, height);  
    FSSetBrowserFillColumnProc(bPtr->browser, fillBrowserColumn); 
    FSSetBrowserPathSeparator(bPtr->browser, DEFAULT_SEPARATOR);
    FSSetBrowserAction(bPtr->browser, handleItemSelection, bPtr);
    FSSetBrowserDoubleAction(bPtr->browser, handleDoubleAction, bPtr);

    WMHangData(bPtr->browser, bPtr);

}

static void
scrolledPathView(void *observerData, WMNotification *notification)
{
    int col            = 0;
    FileBrowser *bPtr  = (FileBrowser *)observerData;

    /* Get the col number we're at */
    col = *(int *)(WMGetNotificationClientData(notification));
    /* Scroll to this col */
    if(bPtr->currentMode == BROWSE)
	FSScrollBrowserToColumn(bPtr->browser, col, False);
/*     else if(bPtr->currentMode == LIST) */
/* 	FSScrollMCListToColumn(bPtr->list, col+2, False); */
	
}

static void
setupIconMode(FileBrowser *bPtr)
{
}

static void
setupListMode(FileBrowser *bPtr, int x, int y, int width, int height)
{
    bPtr->listF = WMCreateFrame(bPtr->displayFrame);
    WMSetFrameRelief(bPtr->listF, WRFlat);
    WMResizeWidget(bPtr->listF, width, height);
    WMMoveWidget(bPtr->listF, 0, 0);

    bPtr->list = FSCreateMCList(bPtr->listF);
    FSSetMCListAction(bPtr->list, handleItemSelection, bPtr);
    FSSetMCListDoubleAction(bPtr->list, handleDoubleAction, bPtr);
}

Bool
FSSetFileBrowserPath(FileBrowser *bPtr, char *path)
{
    Bool ok= True;
    int i, col;

    switch(bPtr->currentMode)
    {
    case BROWSE : WMUnmapWidget(bPtr);
	          FSLoadBrowserColumnZero(bPtr->browser);
	          FSLoadPathViewColumnZero(bPtr->pathView);
	          FSSetBrowserPath(bPtr->browser, path);
		  col = FSGetFileBrowserColumnCount(bPtr);
		  FSSetPathViewToColumn(bPtr->pathView, col);
		  WMMapWidget(bPtr);
	          break;
    case ICON   : ok = False;
	          break;
    case LIST   : WMUnmapWidget(bPtr);
	          FSLoadMCListColumnZero(bPtr->list);
	          FSLoadPathViewColumnZero(bPtr->pathView);
	          FSSetMCListPath(bPtr->list, path);
		  col = FSGetFileBrowserColumnCount(bPtr);
		  FSSetPathViewToColumn(bPtr->pathView, col);
		  WMMapWidget(bPtr);
	          break;
    default     : ok = False;
	          break;
    }

    return ok;
}


char*
FSGetFileBrowserPath(FileBrowser *bPtr)
{
    int   len;
    char *path  = NULL;
    char *npath = NULL;

    switch(bPtr->currentMode)
    {
    case BROWSE : path  = FSGetBrowserPath(bPtr->browser);
		  len = strlen(path);
	          npath = (char *)wmalloc(len); 
		  if(len > 1)
		  {
		      strncpy(npath, path, len-1);
		      npath[len-1] = '\0';
		  }
		  else
		      strcpy(npath, path);
		  if(path)
		      free(path);
		  break;
    case ICON   : npath = NULL;
	          break;
    case LIST   : npath  = FSGetMCListPath(bPtr->list);
	          break;
    default     : npath = NULL;
	          break;
    }

    return npath;
    
}


static void
handleEvents(XEvent *event, void *data)
{
    FileBrowser *bPtr = (FileBrowser*)data;

/*     CHECK_CLASS(data, W_FileBrowser); */

    switch (event->type) {
    case Expose:
/* 	paintFileBrowser(bPtr); */
	break;
	
    case DestroyNotify:
	destroyFileBrowser(bPtr);
	break;
    case ClientMessage:
	break;
    }
}

static void
destroyFileBrowser(FileBrowser *bPtr)
{
    if(bPtr->text)
	free(bPtr->text);

    free(bPtr);
}

char*
FSGetFileBrowserPathToColumn(FileBrowser *bPtr, int column)
{
    int   len;
    char *path = NULL;

    switch(bPtr->currentMode)
    {
    case BROWSE : path = FSGetBrowserPathToColumn(bPtr->browser, column);
		  break;
    case ICON   : path = NULL;
	          break;
    case LIST   : path = FSGetMCListPathToColumn(bPtr->list, column);
	          break;
    default     : path = NULL;
	          break;
    }

    return path;
}

static int
FSGetFileBrowserColumnCount(FileBrowser *bPtr)
{
    int num = -1;

    switch(bPtr->currentMode)
    {
    case BROWSE : num = FSGetBrowserNumberOfColumns(bPtr->browser);
		  break;
    case ICON   : num = -1;
	          break;
    case LIST   : num = FSGetMCListNumberOfColumns(bPtr->list);
	          break;
    default     : num = -1;
	          break;
    }

    return num;
}

static int
FSGetFileBrowserFirstVisibleColumn(FileBrowser *bPtr)
{
    int first = -1;

    switch(bPtr->currentMode)
    {
    case BROWSE : first = FSGetBrowserFirstVisibleColumn(bPtr->browser);
		  break;
    case ICON   : first = -1;
	          break;
    case LIST   : first = -1;
	          break;
    default     : first = -1;
	          break;
    }

    return first;
}

static FileInfo *
FSGetFileBrowserFileInfoAt(FileBrowser *bPtr, int col)
{
    char *pathname;
    FileInfo *fileInfo = NULL;
    
    switch(bPtr->currentMode)
    {
    case BROWSE : pathname = FSGetFileBrowserPathToColumn(bPtr, col);
	          fileInfo = FSGetFileInfo(pathname);
		  break;
    case ICON   : fileInfo = NULL;
	          break;
    case LIST   : pathname = FSGetFileBrowserPathToColumn(bPtr, col+1);
	          fileInfo = FSGetFileInfo(pathname);
	          break;
    default     : fileInfo = NULL;
	          break;
    }

    return fileInfo;
}

FileInfo *
FSGetFileBrowserSelectedFileInfo(FileBrowser *bPtr)
{
    int col;

    col = FSGetFileBrowserColumnCount(bPtr);

    if(col > 1)
	return FSGetFileBrowserFileInfoAt(bPtr, col-2);
    else
	return NULL;
}

void
FSSetFileBrowserFilter(FileBrowser *bPtr, char *filter)
{
    if(bPtr->filter)
	free(bPtr->filter);

    if(filter)
	bPtr->filter = wstrdup(filter);
    else
	bPtr->filter = NULL;

    if(bPtr->list)
	FSSetMCListFilter(bPtr->list, bPtr->filter);
}

char *
FSGetFileBrowserFilter(FileBrowser *bPtr)
{
    return bPtr->filter;
}

void
FSSetFileBrowserDoubleAction(FileBrowser *bPtr, 
			     WMAction *doubleAction, 
			     void *doubleClientData)
{
    bPtr->doubleAction = doubleAction;
    bPtr->doubleClientData = doubleClientData;
}

static void
insertIntoColumn(FileBrowser *bPtr, char *pathname)
{
    int col;
    WMList   *list;
    FileInfo *new;

    col = findColumnFromPath(GetPathFromPathname(pathname));
    new = FSGetFileInfo(pathname);

    list = FSGetBrowserListInColumn(bPtr->browser, col);

    if( DisplayFile(new->name, bPtr->filter, new->fileType) && list)
    {
	WMListItem *item;
	item = WMInsertListItem(list, -1, new->name);
	item->isBranch = isDirectory(new->fileType);

/*         listItem =  FSInsertBrowserItem(bPtr->browser, col, -1, new->name, */
/* 					isDirectory(new->fileType)); */
/* 	listItem->clientData = new; */
    }
    if(new)
	FSFreeFileInfo(new);
}

static void
removeFromColumn(FileBrowser *bPtr, char *path, char *name)
{
    int col, row;
    WMList *list;

    col  = findColumnFromPath(path);
    list = FSGetBrowserListInColumn(bPtr->browser, col);
    row  = WMFindRowOfListItemWithTitle(list, name);
    WMRemoveListItem(list, row);
}

static int
findColumnFromPath(char *path)
{
    int i = 0;
    char *str = wstrdup(path);
    char *tmp = NULL;

    tmp = strtok(str, "/");
    while (tmp) 
    {
	tmp = strtok(NULL, "/");
	i++;
    }
    free(str);

    return i;
}

void
FSSetFileBrowserColumnWidth(FileBrowser *bPtr, int width)
{
    WMSize size;

    assert(width >= PVICON_WIDTH);

    size = WMGetViewSize(bPtr->view);
    bPtr->columnWidth = width;
    WMResizeWidget(bPtr, size.width, size.height);
/*     resizeFileBrowser(bPtr, size.width, size.height); */
}

void
FSFileBrowserKeyPress(FileBrowser *bPtr, XEvent *event)
{
    int    count;
    KeySym ksym;
    char   buffer[64];

    count = XLookupString(&event->xkey, buffer, 63, &ksym, NULL);
    buffer[count] = '\0';

/*     navigateViews(bPtr, buffer); */
/*     switch (ksym) */
/*     { */

}

void
FSUpdateFileBrowser(FileBrowser *bPtr, FileAction action, 
		    FileInfo *src, FileInfo *dest)
{
    int   col;
    char *tmp  = NULL;
    char *tmp2 = NULL;

    switch(action)
	{
	case FileCopy   : 
	    if(isDirectory(dest->fileType))
	    {
		tmp = GetPathnameFromPathName(dest->path, dest->name);
		tmp2 = GetPathnameFromPathName(tmp, src->name);
	    }
	    else
		tmp2 = GetPathnameFromPathName(dest->path, dest->name);
	    insertIntoView(bPtr, tmp2);
	    break;
	case FileMove   :
	    /* Move It */
	    tmp = GetPathnameFromPathName(dest->path, dest->name);
	    tmp2 = GetPathnameFromPathName(tmp, src->name);
	    insertIntoView(bPtr, tmp2);
	    /* Remove it */
	    tmp = GetPathnameFromPathName(src->path, src->name);
	    col = findColumnFromPath(tmp);
	    removeFromView(bPtr, src->path, src->name);
	    /* Update the display */
	    col = findColumnFromPath(FSGetFileBrowserPath(bPtr));
	    FSSetPathViewToColumn(bPtr->pathView, col);
	    break;
	case FileRename   :
	    tmp2 = GetPathnameFromPathName(dest->path, dest->name);
	    insertIntoView(bPtr, tmp2);
	    tmp = GetPathnameFromPathName(src->path, src->name);
	    col = findColumnFromPath(tmp);
	    removeFromView(bPtr, src->path, src->name);
	    break;
	case FileLink   :
	    tmp2 = GetPathnameFromPathName(dest->path, dest->name);
	    insertIntoView(bPtr, tmp2);
	    break;
	case FileDelete : 
	    /* Remove it */
	    tmp = GetPathnameFromPathName(src->path, src->name);
	    removeFromView(bPtr, src->path, src->name);
	    /* Update the PathView */
	    col = findColumnFromPath(FSGetFileBrowserPath(bPtr));
	    FSSetPathViewToColumn(bPtr->pathView, col);
	    break;
	case FileSync : 
	    /* redisplay current Path and update shelf */
	    break;
	}

    if(tmp)
	free(tmp);
    if(tmp2)
	free(tmp2);
}

/* 
 * Need to pass fileinfo instead of pathname to 
 * facilitate multiple file insertions  
 */
static void 
insertIntoView(FileBrowser *bPtr, char *pathname)
{
    int col;
    WMList   *list;
    FileInfo *new;

    col = findColumnFromPath(GetPathFromPathname(pathname));
    new = FSGetFileInfo(pathname);

    list = FSGetBrowserListInColumn(bPtr->browser, col);
    switch(bPtr->currentMode)
	{
	case BROWSE : list = FSGetBrowserListInColumn(bPtr->browser, col);
	    break;
	case ICON   : list = NULL;
	    break;
	case LIST   : list = FSGetMCListListInColumn(bPtr->list, col);
	    break;
	default     :
	    break;
    }


    if( DisplayFile(new->name, bPtr->filter, new->fileType) && list)
    {
	WMListItem *item;
	item = WMInsertListItem(list, -1, new->name);
	if(bPtr->currentMode == LIST)
	    item->clientData = FSMCListFormatFileInfo(bPtr->list, new);
	item->isBranch = isDirectory(new->fileType);
    }

    if(new)
	FSFreeFileInfo(new);
}

static void
removeFromView(FileBrowser *bPtr, char *path, char *name)
{
    int col, row;
    WMList *list;

    col  = findColumnFromPath(path);
    switch(bPtr->currentMode)
	{
	case BROWSE : list = FSGetBrowserListInColumn(bPtr->browser, col);
	    break;
	case ICON   : list = NULL;
	    break;
	case LIST   : list = FSGetMCListListInColumn(bPtr->list, col);
	    break;
	default     : list = NULL;
	    break;
    }

    if(list)
    {
    	row  = WMFindRowOfListItemWithTitle(list, name);
    	WMRemoveListItem(list, row);
	}
}

void
FSSetFileBrowserMode(FileBrowser *bPtr, int mode)
{
    char *path;

    /* ICON ain't implemented yet */
    if(bPtr->currentMode == mode || mode == ICON)
	return;

    path = FSGetFileBrowserPath(bPtr);

    switch(bPtr->currentMode)
    {
    case BROWSE : 
	    WMUnmapWidget(bPtr->browser);
	break;
    case ICON   : 
/* 	    WMUnmapWidget(bPtr->icon); */
	break;
    case LIST   :
	    WMUnmapWidget(bPtr->listF);
	break;
    }

    bPtr->currentMode = mode;

    switch(bPtr->currentMode)
    {
    case BROWSE : WMMapWidget(bPtr->browser);
	          break;
    case ICON   : break;
    case LIST   : WMMapWidget(bPtr->listF);
	          break;
    }

    FSSetFileBrowserPath(bPtr, path);
}
