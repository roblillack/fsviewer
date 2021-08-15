/*****************************************************************************
 *
 * FSFilebutton, is a subset of WMButton to provide certain functionality.
 * It will provide shaped backgrounds in button the image and text sections. 
 * It will facilitate the current implemntation of DND.
 *
 *****************************************************************************/

#include <WMaker.h>
#include <WINGs/WINGsP.h>
#include <sys/stat.h>
#include <unistd.h>

#include "files.h"
#include "FSFileButton.h"
#include "FSViewer.h"
#include "FSUtils.h"
#include "DnD.h"

/*
 * Default Widget width/height
 * Default Button width/height
 * Default Label  width/height
 */
#define DEFAULT_WIDTH		64
#define DEFAULT_HEIGHT	        80
#define BUTTON_WIDTH		64
#define BUTTON_HEIGHT		64
#define LABEL_WIDTH             20
#define LABEL_HEIGHT            16
/* Size of the font to be used in labels */
#define FONT_PT                 14

/*
 * FSFileButton Widget 
 */
typedef struct W_FSFileButton 
{
    W_Class     widgetClass;
    W_View     *view;

    W_View     *textView;

    FileInfo   *fileInfo;
    char       *pathname;

    void       *clientData;
    WMAction   *action;
    void       *doubleClientData;
    WMAction   *doubleAction;

    int         groupIndex;

    WMHandlerID timer;
    WMHandlerID paintTimer;

    unsigned int width;
    unsigned int height;
    WMPoint    textViewPos;

    struct {
	unsigned int addedObserver:1;
	unsigned int springLoaded:1;
	unsigned int extSelected:1;
	unsigned int selected:1;
	unsigned int cleared:1;
	unsigned int isBranch:1;
	unsigned int pushed:1;
    } flags;
} _FSFileButton;


static void handleEvents(XEvent *event, void *data);
static void handleActionEvents(XEvent *event, void *data);
static void paintFSFileButton(_FSFileButton *bPtr);
static void destroyFSFileButton(_FSFileButton *bPtr);
/* static void resizeFSFileButton(WMWidget*, unsigned int, unsigned int); */
static void paintImageAndText(W_View *view, int x, int y, 
			      int width, int height,
			      char *imgName, char *text, GC backGC);
static void radioPushObserver(void *observerData,WMNotification *notification);
static void showTextView(FSFileButton *bPtr);
static W_View *createTextView(W_Screen *scr, char *text, int width, int len);
static void destroyTextView(FSFileButton *bPtr);
static void disablePaintTimer(FSFileButton *bPtr);

static void willResizeFSFileButton(W_ViewDelegate*, WMView*,
				   unsigned int*, unsigned int*);
/*
 * WMResizeWidget callback Function
 */
static W_ViewDelegate _FSFileButtonViewDelegate = {
        NULL,
        NULL,
        NULL,
        NULL,
        willResizeFSFileButton
};

/* W_ViewProcedureTable _FSFileButtonViewProcedures = { */
/*     NULL, */
/* 	resizeFSFileButton, */
/* 	NULL */
/* }; */

static char *FSPushedFileButtonNotification = "FSPushedFileButtonNotification";

/*
 * Widget class ID
 */
static W_Class	fileButtonWidgetClass = 0;

/* 
 * Initializer for FSFileButton Widget. Must be called before creating any
 * instances of the widget.
 */
W_Class  
InitFSFileButton(WMScreen *scr) 
{
    /* register the widget with WINGs and get the widget class ID */
    if (!(fileButtonWidgetClass))
    {
	fileButtonWidgetClass =
/* 	    W_RegisterUserWidget(&_FSFileButtonViewProcedures); */
	    W_RegisterUserWidget();
    }

    return fileButtonWidgetClass;
}

/*
 * Function to create the Widget
 */
FSFileButton*
FSCreateFileButton(WMWidget *parent)
{
    FSFileButton *bPtr;

    bPtr = wmalloc(sizeof(FSFileButton));
    memset(bPtr, 0, sizeof(FSFileButton));

    bPtr->widgetClass = fileButtonWidgetClass;
    
    bPtr->view = W_CreateView(W_VIEW(parent));
    if (!bPtr->view) 
    {
	free(bPtr);
	return NULL;
    }
    bPtr->view->self = bPtr;
    bPtr->view->delegate = &_FSFileButtonViewDelegate;

    /*
     * Intercept some events
     * ExposureMask: allows us to know when we should paint
     * StructureNotifyMask: allows us to know things like when we are destroyed
     */
    WMCreateEventHandler(bPtr->view, ExposureMask|StructureNotifyMask, 
			 handleEvents, bPtr);

    /*
     * Intercept some more events, this time those that relate to
     * action events
     */
    WMCreateEventHandler(bPtr->view, ButtonPressMask | ButtonReleaseMask |
			 EnterWindowMask | LeaveWindowMask,
			 handleActionEvents, bPtr);
	
/*     resizeFSFileButton(bPtr, DEFAULT_WIDTH, DEFAULT_HEIGHT); */
    W_ResizeView(bPtr->view, DEFAULT_WIDTH, DEFAULT_HEIGHT);
    /* 
     * Default widget configuration 
     */

    bPtr->flags.addedObserver = 0;
    bPtr->flags.springLoaded  = 1;
    bPtr->flags.extSelected   = 0;
    bPtr->flags.selected      = 0;
    bPtr->flags.cleared       = 1;
    bPtr->flags.isBranch      = 0;
    bPtr->flags.pushed        = 0;
    bPtr->groupIndex          = 0;

    bPtr->doubleAction        = NULL;
    bPtr->doubleClientData    = NULL;

    bPtr->textViewPos.x = -1;
    bPtr->textViewPos.y = -1;
    bPtr->pathname = NULL;
/*     bPtr->fileInfo = FSCreateFileInfo(); */
 
    return bPtr;
}

static void 
willResizeFSFileButton(W_ViewDelegate *self, WMView *view,
		       unsigned int *width, unsigned int *height)
{
    int x = 0;
    int y = 0;
    FSFileButton *bPtr = (FSFileButton *)view->self;

    /*
     * We can't make the widget smaller than the defaults
     * Is this okay???
     */
    assert(*height >= DEFAULT_HEIGHT);
    assert(*width  >= DEFAULT_WIDTH);

    bPtr->width = *width;
    bPtr->height = *height;
    /*
     * Resize the widget views
     */
/*     W_ResizeView(bPtr->view, width, height); */
}

/* static void  */
/* resizeFSFileButton(WMWidget *w, unsigned int width, unsigned int height) */
/* { */
/*     int x = 0; */
/*     int y = 0; */
/*     FSFileButton *bPtr = (FSFileButton *)w; */

     /* 
      * We can't make the widget smaller than the defaults 
      * Is this okay??? 
      */
/*     assert(height >= DEFAULT_HEIGHT); */
/*     assert(width  >= DEFAULT_WIDTH); */

/*     bPtr->width = width; */
/*     bPtr->height = height; */
     /* 
      * Resize the widget views
      */ 
/*     W_ResizeView(bPtr->view, width, height); */
/* } */

static void
paintFSFileButton(_FSFileButton *bPtr)
{
    if (!bPtr->view->flags.mapped)
	return;

    if(bPtr->flags.cleared)
    {
	paintImageAndText(bPtr->view, 0, 0, bPtr->width, bPtr->height,
			  NULL, NULL, WMColorGC(bPtr->view->screen->white));
	return;
    }
    if(bPtr->paintTimer)
	return;

    /*
     * add a timer such that on a double click the button doesn't flash
     * It makes it more pleasing to the eye, IMO. I don't what kind of
     * over head it adds. I will make the delay customiseable. 
     * To make it work properly, it needs to be greater than the double 
     * click timeout.
     */
    if(bPtr->flags.selected)
    {
	bPtr->paintTimer = WMAddTimerHandler(500,
					     (WMCallback *) disablePaintTimer,
					     bPtr);
    }
    /*
     * If selected , draw the image and text with a white shaped 
     * background else draw with no background
     */
    if(bPtr->flags.selected || bPtr->flags.extSelected)
	paintImageAndText(bPtr->view, 0, 0, bPtr->width, bPtr->height,
			  bPtr->fileInfo->imgName, bPtr->fileInfo->name, 
			  WMColorGC(bPtr->view->screen->white));
    else
	paintImageAndText(bPtr->view, 0, 0, bPtr->width, bPtr->height,
			  bPtr->fileInfo->imgName, bPtr->fileInfo->name, 
			  NULL);

    if(isDirectory(bPtr->fileInfo->fileType) && bPtr->flags.isBranch) 
    {
	W_Screen *scr = bPtr->view->screen;
	Drawable  d   = bPtr->view->window;

	XDrawLine(scr->display, d, WMColorGC(scr->darkGray), 
		  bPtr->width-11,((bPtr->height-8)/2),
		  bPtr->width-6, bPtr->height/2);
	XDrawLine(scr->display, d,WMColorGC(scr->white), 
		  bPtr->width-11, ((bPtr->height+8)/2),
		  bPtr->width-6,  bPtr->height/2);
	XDrawLine(scr->display, d, WMColorGC(scr->black), 
		  bPtr->width-12, (bPtr->height-8)/2,
		  bPtr->width-12, (bPtr->height+8)/2);
    }
}

static void
destroyFSFileButton(_FSFileButton *bPtr)
{
    if (bPtr->flags.addedObserver) 
        WMRemoveNotificationObserver(bPtr);
    if(bPtr->fileInfo)
	FSFreeFileInfo(bPtr->fileInfo);
    if(bPtr->pathname)
	free(bPtr->pathname);
    destroyTextView(bPtr);
    disablePaintTimer(bPtr);

    free(bPtr);
}

static void
handleEvents(XEvent *event, void *data)
{
    FSFileButton *bPtr = (FSFileButton *)data;

    switch (event->type) 
    {
    case Expose:
	paintFSFileButton(bPtr);
	break;
	
    case DestroyNotify:
	destroyFSFileButton(bPtr);
	break;
    }
}

static void
handleActionEvents(XEvent *event, void *data)
{
    int dopaint = 0;
    int doclick = 0;
    FSFileButton *bPtr = (FSFileButton *)data;
    
    switch (event->type) 
	{
	case EnterNotify :
	    if(bPtr->textView)
	    {
		bPtr->textViewPos.x = event->xmotion.x_root-20;
		bPtr->textViewPos.y = event->xmotion.y_root-20;
		bPtr->timer = WMAddTimerHandler(1000,
						(WMCallback*)showTextView,bPtr);
	    }
	    break;
	    
	case LeaveNotify :
	    if(bPtr->textView)
	    {
		if (bPtr->timer) 
		{
		    WMDeleteTimerHandler(bPtr->timer);
		    bPtr->timer = NULL;
		}
		W_UnmapView(bPtr->textView);
	    }
	    if (bPtr->flags.springLoaded && bPtr->flags.pushed) 
	    {
		bPtr->flags.selected = 0;
		bPtr->flags.pushed = 0;
		dopaint = 1;
	    }
	    break;
	    
	case ButtonPress :
	    if(bPtr->textView)
	    {
		if (bPtr->timer) 
		{
		    WMDeleteTimerHandler(bPtr->timer);
		    bPtr->timer = NULL;
		}
		W_UnmapView(bPtr->textView);
	    }
	    if (event->xbutton.button == Button1) 
	    {
		if (bPtr->groupIndex>0) 
		{
		    if (!bPtr->flags.selected)
			doclick = 1;
		}
		bPtr->flags.pushed = 1;
		bPtr->flags.selected = 1;
		dopaint = 1;
	    }
	    break;
	    
	case ButtonRelease :
	    if(bPtr->textView)
	    {
		if (bPtr->timer) 
		{
		    WMDeleteTimerHandler(bPtr->timer);
		    bPtr->timer = NULL;
		}
		W_UnmapView(bPtr->textView);
	    }
	    if (event->xbutton.button == Button1) 
	    {
		if (bPtr->flags.pushed) 
		{
		    if(bPtr->groupIndex==0)
		    {
			doclick = 1;
			dopaint = 1;
		    }
		    /* 
		     * Is this really needed? 
		     * Would groupIndex == 0 cover the condition?
		     */
		    if(bPtr->flags.springLoaded) 
			bPtr->flags.selected = 0;
		}
		bPtr->flags.pushed = 0;
	    }
	    break;
	default:
	    wwarning("%s %d: %d", __FILE__, __LINE__, event->type);
	    break;
	}
	
    /* 
     * Paint the button unless that state
     * has been explicitly set external to
     * the button. Prevents Flashing!
     */
    if(dopaint && !bPtr->flags.extSelected)
	paintFSFileButton(bPtr);
	
    if(doclick)
    {
        if (bPtr->flags.selected && bPtr->groupIndex>0) 
	{
            WMPostNotificationName(FSPushedFileButtonNotification, bPtr, NULL);
        }

	/* call callback for click */
	if (bPtr->action)
	    (*bPtr->action)(bPtr, bPtr->clientData);	
    }

    if(WMIsDoubleClick(event))
    {
	/* call callback for double click */
	if (bPtr->doubleAction)
	    (*bPtr->doubleAction)(bPtr, bPtr->doubleClientData);
    } 	    
}

static void
paintImageAndText(W_View *view, int x, int y, int width, int height,
		  char *imgName, char *text, GC backGC)
{
    int ix   = 0;
    int iy   = 0;
    int xoff = 0;
    int yoff = 0;
    int strwidth = 1;
    RColor    color;
    WMFont   *aFont;
    WMPixmap *image;
    W_Screen *screen = view->screen;
    Drawable  d      = view->window;

    color.red = 0xae;
    color.green = 0xaa;
    color.blue = 0xae;
    color.alpha = 0;

    /*
     * Clear the area we are painting on
     */
    XSetForeground(screen->display, screen->copyGC,
		   view->attribs.background_pixel);
    XFillRectangle(screen->display, d, screen->copyGC, x, y, width, height);

    if(imgName == NULL)
	return;

    if(text)
    {
	aFont = WMSystemFontOfSize(screen, FONT_PT);
	strwidth = WMWidthOfString(aFont, text, strlen(text));
	WMReleaseFont(aFont);
    }
    ix = (width - strwidth)/2;

    /* background */
    if(backGC)
    {
	image = FSCreatePixmapWithBackingFromFile(screen, imgName, &color);

	yoff = (height - image->height - LABEL_HEIGHT)/2;
	iy = yoff + image->height;
    
	/* text background rectangle */
	if(text)
	    XFillRectangle(screen->display, d, backGC, x+ix, y+iy, 
			   strwidth, LABEL_HEIGHT);
    }
    else
    {        /* FS.. */
	image = WMCreateBlendedPixmapFromFile(screen, imgName, &color);	
	yoff = (height - image->height - LABEL_HEIGHT)/2;
	iy = yoff + image->height;
    }

    xoff = (width - image->width)/2;
    if(image)
    {
	WMDrawPixmap(image, d, x+xoff, y+yoff);
	WMReleasePixmap(image);
    }

    if(text)
    {
	W_PaintText(view, d, screen->normalFont, x+ix, y+iy,
		    strwidth, WACenter, WMBlackColor(screen),
		    True, text, strlen(text));
    }

}

/*
 * If a button in a group is pressed, deselected the currently 
 * selected one.
 */
static void
radioPushObserver(void *observerData, WMNotification *notification)
{
    FSFileButton *bPtr = (FSFileButton *)observerData;
    FSFileButton *pushedButton = 
	(FSFileButton *)WMGetNotificationObject(notification);

    if (bPtr != pushedButton && 
	pushedButton->groupIndex == bPtr->groupIndex && 
	bPtr->groupIndex!=0) 
    {
        if (bPtr->flags.selected) 
	{
            bPtr->flags.selected = 0;
            paintFSFileButton(bPtr);
        }
    }
}

/*
 * Allow the buttons to be grouped
 * Handy for the Icon view mode of browsing
 */
void
FSGroupFileButtons(FSFileButton *bPtr, FSFileButton *newMember)
{
    static int tagIndex = 0;
  
    CHECK_CLASS(bPtr, fileButtonWidgetClass);
    CHECK_CLASS(newMember, fileButtonWidgetClass);

    if (!bPtr->flags.addedObserver) 
    {
        WMAddNotificationObserver(radioPushObserver, bPtr,
                                  FSPushedFileButtonNotification, NULL);
        bPtr->flags.addedObserver = 1;
	bPtr->flags.springLoaded = 0;
    }
    if (!newMember->flags.addedObserver) 
    {
        WMAddNotificationObserver(radioPushObserver, newMember,
                                  FSPushedFileButtonNotification, NULL);
        newMember->flags.addedObserver = 1;
	newMember->flags.springLoaded = 0;
    }

    if (bPtr->groupIndex==0) 
    {
        bPtr->groupIndex = ++tagIndex;
    }
    newMember->groupIndex = bPtr->groupIndex;
}


FileInfo *
FSGetFileButtonFileInfo(FSFileButton *bPtr)
{
    CHECK_CLASS(bPtr, fileButtonWidgetClass);

    return bPtr->fileInfo;
}

char *
FSGetFileButtonName(FSFileButton *bPtr)
{
    CHECK_CLASS(bPtr, fileButtonWidgetClass);

    return wstrdup(bPtr->fileInfo->name);
}

char *
FSGetFileButtonPathname(FSFileButton *bPtr)
{
    CHECK_CLASS(bPtr, fileButtonWidgetClass);

    return wstrdup(bPtr->pathname);
}

void
FSSetFileButtonPathname(FSFileButton *bPtr, char *pathname, int isBranch)
{
    CHECK_CLASS(bPtr, fileButtonWidgetClass);

    if(pathname)
    {
	int textLen;
	int textWidth;
	FileInfo *fileInfo = NULL;
	WMScreen *scr = WMWidgetScreen(bPtr);

	if(bPtr->pathname)
	    free(bPtr->pathname);
	bPtr->pathname = wstrdup(pathname);
	bPtr->flags.isBranch = isBranch;
	if(bPtr->fileInfo)
	    FSFreeFileInfo(bPtr->fileInfo);
	bPtr->fileInfo = FSGetFileInfo(pathname);
	bPtr->flags.cleared = 0;
	paintFSFileButton(bPtr);

	textLen = strlen(bPtr->fileInfo->name);
	textWidth = WMWidthOfString(scr->normalFont, bPtr->fileInfo->name, 
				    textLen);
	if (textWidth > bPtr->width) 
	{
	    if(bPtr->textView)
		destroyTextView(bPtr);
	    bPtr->textView = createTextView(scr, bPtr->fileInfo->name, 
					    textWidth, textLen);
	}
    }
}

void
FSSetFileButtonSelected(FSFileButton *bPtr, int isSelected)
{
    CHECK_CLASS(bPtr, fileButtonWidgetClass);

    bPtr->flags.extSelected = isSelected;
    paintFSFileButton(bPtr);
}

void
FSSetFileButtonAction(FSFileButton *bPtr, WMAction *action, void *clientData)
{
    CHECK_CLASS(bPtr, fileButtonWidgetClass);

    bPtr->action = action;
    bPtr->clientData = clientData;
}

void
FSSetFileButtonDoubleAction(FSFileButton *bPtr, 
			    WMAction *doubleAction, 
			    void *doubleClientData)
{
    CHECK_CLASS(bPtr, fileButtonWidgetClass);

    bPtr->doubleAction = doubleAction;
    bPtr->doubleClientData = doubleClientData;
}

static void
showTextView(FSFileButton *bPtr)
{
    if(bPtr->textView)
    {
	W_MoveView(bPtr->textView, bPtr->textViewPos.x, bPtr->textViewPos.y);
	W_MapView(bPtr->textView);
	WMDeleteTimerHandler(bPtr->timer);
	bPtr->timer = NULL;
    }
}

W_View *
createTextView(W_Screen *scr, char *text, int width, int len)
{
    Pixmap pix;
    W_View *view = NULL;

    if(!text)
	return;

    view = W_CreateTopView(scr);
    view->attribFlags |= CWOverrideRedirect|CWSaveUnder;
    view->attribs.event_mask = StructureNotifyMask;
    view->attribs.override_redirect = True;
    view->attribs.save_under = True;
    
    pix = XCreatePixmap(scr->display, W_DRAWABLE(scr), 
			width+4, 16, scr->depth);
    
    XFillRectangle(scr->display, pix, WMColorGC(scr->white), 
		   0, 0, width+4, 16);
    
    XDrawRectangle(scr->display, pix, WMColorGC(scr->black), 
		   0, 0, width+3, 15);
    
    W_ResizeView(view, width+4, 16);
    W_RealizeView(view);
    XSetWindowBackgroundPixmap(WMScreenDisplay(scr), view->window, pix);
    XClearWindow(WMScreenDisplay(scr), view->window);
    W_PaintText(view, pix, scr->normalFont, 2, 1, width, WACenter, 
		WMBlackColor(scr), False, text, len);
    if(pix)
	XFreePixmap(scr->display, pix);

    return view;
}

static void
destroyTextView(FSFileButton *bPtr)
{
    if(bPtr->textView)
    {
	W_DestroyView(bPtr->textView);
	bPtr->textView = NULL;
    }
    if (bPtr->timer) 
    {
	WMDeleteTimerHandler(bPtr->timer);
	bPtr->timer = NULL;
    }
}

static void
disablePaintTimer(FSFileButton *bPtr)
{
    if (bPtr->paintTimer) 
    {
	WMDeleteTimerHandler(bPtr->paintTimer);
	bPtr->paintTimer = NULL;
	paintFSFileButton(bPtr);
    }
}

void
FSClearFileButton(FSFileButton *bPtr)
{

    bPtr->flags.cleared = 0;
    paintFSFileButton(bPtr);
}
