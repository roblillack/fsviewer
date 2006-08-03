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

#include "files.h"
#include "FSViewer.h"
#include "FSFinder.h"
#include "FSUtils.h"

#define  MIN_UPPER_HEIGHT  90
#define  MIN_LOWER_HEIGHT  200
#define  FILE_ICON_WIDTH   95
#define  FILE_ICON_HEIGHT  80
#define  PADX              10
#define  PADY              10
#define  LISTX             130
#define  PADDED_WIDTH      115
#define  PADDED_HEIGHT     100

static void notificationObserver(void *self, WMNotification *notif);
static void populateList(FSFinder *finder);
static void listDoubleClick(WMWidget *self, void *clientData);

FSFinder *
FSCreateFinder(FSViewer *fsViewer)
{
    WMFrame            *frame;
    FSFinder           *finder;
    GNUstepWMAttributes attributes;

    if (!(finder = (FSFinder *) malloc(sizeof(FSFinder))))
	return NULL;
    memset((void *) finder, 0, sizeof(FSFinder));

    finder->dpy = FSGetFSViewerDisplay(fsViewer);
    finder->scr = FSGetFSViewerScreen(fsViewer);
    finder->fsViewer = fsViewer;
    finder->x = 200;
    finder->y = 185;
    finder->w = 272;
    finder->h = 350;
/*     finder->xcontext = FSGetFSViewerXContext(fsViewer); */

    finder->win = WMCreateWindow(finder->scr, "finder");
    WMSetWindowTitle(finder->win, _("FSViewer - Finder"));
    WMSetWindowCloseAction(finder->win, FSDestroyFinder, (void *) finder);
	
    WMResizeWidget(finder->win, finder->w, finder->h);
    WMMoveWidget(finder->win, finder->x, finder->y);

    finder->mainF = WMCreateFrame(finder->win);
    WMMoveWidget(finder->mainF, 0, 0);
    WMResizeWidget(finder->mainF, finder->w, finder->h);
    WMSetFrameRelief(finder->mainF, WRFlat);
    WMMapWidget(finder->mainF);

    frame = WMCreateFrame(finder->mainF);
    WMMoveWidget(frame, PADX, PADY);
    WMResizeWidget(frame, finder->w-(PADX*2), LISTX-(PADY*2));
    WMSetFrameRelief(frame, WRGroove);
    WMMapWidget(frame);

    finder->findRadioBtns[0] = WMCreateRadioButton(frame);
    WMResizeWidget(finder->findRadioBtns[0], finder->w-(PADX*4)-PADX, PADY*2);
    WMMoveWidget(finder->findRadioBtns[0], PADX, PADY);
    WMSetButtonText(finder->findRadioBtns[0], "locate \"%s\"");
    WMSetButtonSelected(finder->findRadioBtns[0], 1);
    WMMapWidget(finder->findRadioBtns[0]); 
    WMGroupButtons(finder->findRadioBtns[0], finder->findRadioBtns[0]);

    finder->findRadioBtns[1] = WMCreateRadioButton(frame);
    WMResizeWidget(finder->findRadioBtns[1], finder->w-(PADX*4)-PADX, PADY*2);
    WMMoveWidget(finder->findRadioBtns[1], PADX, PADY*3);
    WMSetButtonText(finder->findRadioBtns[1], 
		   "find \"%p\" -name \"%s\" -print");
    WMMapWidget(finder->findRadioBtns[1]); 
    WMGroupButtons(finder->findRadioBtns[0], finder->findRadioBtns[1]);

    finder->label = WMCreateLabel(frame);
    WMResizeWidget(finder->label, finder->w-(PADX*4)-PADX, PADY*2);
    WMMoveWidget(finder->label, PADX, PADY*6);
    WMSetLabelText(finder->label, _("Search String:"));
    WMMapWidget(finder->label); 

    finder->searchField = WMCreateTextField(frame);
    WMResizeWidget(finder->searchField, finder->w-(PADX*4)-PADX, PADY*2);
    WMMoveWidget(finder->searchField, PADX, PADY*8);
    WMMapWidget(finder->searchField);

    finder->list = WMCreateList(finder->mainF);
    WMResizeWidget(finder->list, finder->w-(PADX*2), finder->h-LISTX-(PADY*2));
    WMMoveWidget(finder->list, PADX, LISTX+PADY);
    WMSetListDoubleAction(finder->list, listDoubleClick, finder);
    WMMapWidget(finder->list);

    WMRealizeWidget(finder->win);

    if ((finder->hints = XAllocWMHints()))
    {
	finder->hints->window_group = FSGetFSViewerLeader(fsViewer);
	finder->hints->flags = WindowGroupHint;
	XSetWMHints(finder->dpy, WMWidgetXID(finder->win),
		    finder->hints);
    }

    if ((finder->class = XAllocClassHint()))
    {
	finder->class->res_name = "fsfinder";
	finder->class->res_class = "FSViewer";
	XSetClassHint(finder->dpy, WMWidgetXID(finder->win), finder->class);
    }

    if ((finder->size = XAllocSizeHints()))
    {
	finder->size->width      = finder->w;
	finder->size->height     = finder->h;
	finder->size->min_width  = finder->w;
	finder->size->max_width  = DisplayWidth(finder->dpy, 
						DefaultScreen(finder->dpy)); 
	finder->size->min_height = finder->h;
	finder->size->max_height = DisplayHeight(finder->dpy, 
						 DefaultScreen(finder->dpy));
	finder->size->flags      = (USSize | PSize | PMinSize | PMaxSize);
	finder->size->x = finder->x;
	finder->size->y = finder->y;
	finder->size->flags |= (USPosition | PPosition);

	XSetWMNormalHints(finder->dpy, WMWidgetXID(finder->win),
			  finder->size);
    }
    
    memset((void *) &attributes, 0, sizeof(GNUstepWMAttributes));
    attributes.window_style = (WMTitledWindowMask |
			       WMMiniaturizableWindowMask | 
			       WMClosableWindowMask |
			       WMResizableWindowMask);
    attributes.window_level = WMNormalWindowLevel;
    attributes.extra_flags = GSFullKeyboardEventsFlag;
    attributes.flags = (GSWindowStyleAttr | GSWindowLevelAttr |
			GSExtraFlagsAttr);
    WMSetWindowAttributes(finder->dpy, WMWidgetXID(finder->win),
			  &attributes);

    WMAppAddWindow(FSGetFSViewerWMContext(fsViewer), 
		   WMWidgetXID(finder->win));

/*     WMMapSubwidgets(finder->win);  */
    WMMapWidget(finder->win); 

/*     XSaveContext(finder->dpy, WMWidgetXID(finder->win), */
/* 		 finder->xcontext, (XPointer) finder); */
    FSSetFSViewerFinder(fsViewer, finder);
/*     FSAddViewToFSViewer(fsViewer, finder); */

/*     WMCreateEventHandler(WMWidgetView(finder->finder), KeyPressMask, */
/* 			 handleKeyPressEvents, fsViewer); */

    WMSetViewNotifySizeChanges(WMWidgetView(finder->win), True);
    /* register notification observers */
    WMAddNotificationObserver(notificationObserver, finder,
                              WMViewSizeDidChangeNotification,
                              WMWidgetView(finder->win));
    WMAddNotificationObserver(notificationObserver, finder,
                              WMTextDidEndEditingNotification, 
			      finder->searchField);

    WMSetWindowMiniwindowPixmap(finder->win, 
			       WMGetApplicationIconPixmap(finder->scr));

    return finder;
}

void 
FSDestroyFinder(WMWidget *self, void *client)
{
    FSFinder *finder = (FSFinder *) client;

/*     XDeleteContext(finder->dpy, WMWidgetXID(finder->win), finder->xcontext); */
    WMUnmapWidget(finder->win);

    FSSetFSViewerFinder(finder->fsViewer, NULL);
    WMDestroyWidget(finder->win);
    free(finder);

    finder = NULL;
}

WMWindow *
FSGetFinderWindow(FSFinder *finder)
{
    return finder->win;
}

static void
notificationObserver(void *self, WMNotification *notif)
{
    FSFinder *finder = (FSFinder *)self;
    void *object = WMGetNotificationObject(notif);

    if (WMGetNotificationName(notif) == WMViewSizeDidChangeNotification) 
    {
        if (object == WMWidgetView(finder->win)) 
	{
            finder->h = WMWidgetHeight(finder->win);
            finder->w = WMWidgetWidth(finder->win);

	    WMResizeWidget(finder->mainF, finder->w, finder->h);
	    WMResizeWidget(finder->list, 
			   finder->w-(PADY*2), finder->h-LISTX-(PADY*2));
        } 
    }
    else if( (int)WMGetNotificationClientData(notif) == WMReturnTextMovement )
    {
	populateList(finder);
    }
}

static void 
populateList(FSFinder *finder)
{
    int   len;
    char  buffer[1024];
    FILE *f = NULL;
    char *str;
    char *cmd;
    
    str = WMGetTextFieldText(finder->searchField);

    if(!str || (strlen(str) == 0))
	return;

    /* 
     * It seems a bit weird that the first button should be 1
     * and the second should be 0, maybe it's LIFO
     */
    if(WMGetButtonSelected(finder->findRadioBtns[0]) == 1)
    {
	cmd = (char *) wmalloc(strlen(str)+10);
	sprintf(cmd, "locate \"%s\"", str);
    }
    else
    {
	char *path;

	path = FSGetFSViewerPath(finder->fsViewer);
	cmd = (char *) wmalloc(strlen(str)+strlen(path)+24);
	
	sprintf(cmd, "find \"%s\" -name \"%s\" -print", path, str);
    }

    if( (f = popen(cmd, "r")) == NULL)
	return;

    WMClearList(finder->list);
    while(fgets(buffer, 1024, f)) 
    {
	len = strlen(buffer);
	/* Need to fix this!! */
	if(buffer[len-1] == '\n')
	    buffer[len-1] = '\0';
	WMAddListItem(finder->list, buffer);
    }
    pclose(f);
}

static void
listDoubleClick(WMWidget *self, void *clientData)
{
    WMListItem *item = NULL;
    FSFinder *finder = (FSFinder *) clientData;

    item = WMGetListSelectedItem(finder->list);
    if(item)
	FSSetFSViewerPath(finder->fsViewer, item->text);
}
