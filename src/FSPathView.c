#include <WMaker.h>
#include <WINGs/WINGsP.h>
#include <math.h> /* for : double rint (double) */

#include "files.h"
#include "FSViewer.h"
#include "FSUtils.h"
#include "FSFileButton.h"
#include "FSPathView.h"

#include "DnD.h"

#include "xpm/arrow.xpm"

/* typedef struct PVIcon */
/* { */
/*     FSFileButton *btn; */
/*     WMButton     *arrow; */

/*     struct PVIcon *next; */

/* } PVIcon; */

char *FSPathViewDidScrollNotification = "FSBrowserDidScrollNotification";


typedef struct W_FSPathView 
{
    W_Class widgetClass;
    W_View *view;

    char **titles;
    FSFileButton **columns;

    short columnCount;
    short usedColumnCount;	       /* columns actually being used */
    short minColumnWidth;
 
    short maxVisibleColumns;
    short firstVisibleColumn;

    short titleHeight;
    
    short selectedColumn;
    short lastClicked;

    WMSize columnSize;
    

    void *clientData;
    WMAction *action;
    void *doubleClientData;
    WMAction *doubleAction;

    FSPathViewFillColumnProc *fillColumn;
    
    WMScroller *scroller;

    char *pathSeparator;
    
    struct {
	unsigned int isTitled:1;
	unsigned int allowMultipleSelection:1;
	unsigned int hasScroller:1;
	
	/* */
	unsigned int loaded:1;
	unsigned int loadingColumn:1;
    } flags;
} _FSPathView;


#define COLUMN_SPACING 	4
#define TITLE_SPACING 2

#define DEFAULT_WIDTH                 305
#define DEFAULT_HEIGHT                115
#define DEFAULT_HAS_SCROLLER          True
#define DEFAULT_TITLE_HEIGHT          20
#define DEFAULT_IS_TITLED             False
#define DEFAULT_MAX_VISIBLE_COLUMNS   3
#define DEFAULT_SEPARATOR             "/"

#define MIN_VISIBLE_COLUMNS           1
#define MAX_VISIBLE_COLUMNS           32


#define COLUMN_IS_VISIBLE(b, c)	((c) >= (b)->firstVisibleColumn \
				&& (c) < (b)->firstVisibleColumn + (b)->maxVisibleColumns)


static void  handleEvents(XEvent *event, void *data);
static void  destroyPathView(FSPathView *pvPtr);
static void  setupScroller(FSPathView *pvPtr);
static void  scrollToColumn(FSPathView *pvPtr, int column,
			    Bool updateScroller);
static void  loadColumn(FSPathView *pvPtr, int column);
static void  removeColumn(FSPathView *pvPtr, int column);
static void  handlePVButtonDrop(XEvent *ev, void *clientData);
static void  handlePVButtonDrag(XEvent *ev, void *clientData);
static char *createTruncatedString(WMFont *font, char *text, int *textLen, 
				   int width);
/* static void resizeFSPathView(WMWidget*, unsigned int, unsigned int); */
static void willResizeFSPathView(W_ViewDelegate*, WMView*,
				 unsigned int*, unsigned int*);


static W_ViewDelegate _FSPathViewViewDelegate = {
        NULL,
        NULL,
        NULL,
        NULL,
        willResizeFSPathView
};
/* W_ViewProcedureTable _FSPathViewViewProcedures = { */
/*     NULL, */
/* 	resizeFSPathView, */
/* 	NULL */
/* }; */


// Widget class ID.
static	W_Class	fsPathViewWidgetClass = 0;

W_Class InitFSPathView(WMScreen *scr)
{
    // Register the widget with WINGs and get the widget class ID.
    if (!(fsPathViewWidgetClass))
    {
	fsPathViewWidgetClass =
/* 	    W_RegisterUserWidget(&_FSPathViewViewProcedures); */
	    W_RegisterUserWidget();
    }

    return fsPathViewWidgetClass;
}

FSPathView*
FSCreatePathView(WMWidget *parent)
{
    FSPathView *pvPtr;
    int i;

    wassertrv(parent, NULL);

    pvPtr = wmalloc(sizeof(FSPathView));
    memset(pvPtr, 0, sizeof(FSPathView));

    pvPtr->widgetClass = fsPathViewWidgetClass;
    
    pvPtr->view = W_CreateView(W_VIEW(parent));
    if (!pvPtr->view) {
	free(pvPtr);
	return NULL;
    }
    pvPtr->view->self = pvPtr;
    pvPtr->view->delegate = &_FSPathViewViewDelegate;

    WMCreateEventHandler(pvPtr->view, ExposureMask|StructureNotifyMask
			 |ClientMessageMask, handleEvents, pvPtr);
	
    /* default configuration */
    pvPtr->flags.hasScroller = DEFAULT_HAS_SCROLLER;

    pvPtr->titleHeight = DEFAULT_TITLE_HEIGHT;
    pvPtr->flags.isTitled = DEFAULT_IS_TITLED;
    pvPtr->maxVisibleColumns = DEFAULT_MAX_VISIBLE_COLUMNS;

    WMResizeWidget(pvPtr, DEFAULT_WIDTH, DEFAULT_HEIGHT);
/*     resizeFSPathView(pvPtr, DEFAULT_WIDTH, DEFAULT_HEIGHT); */
    
    pvPtr->pathSeparator = wstrdup(DEFAULT_SEPARATOR);

    if (pvPtr->flags.hasScroller)
	setupScroller(pvPtr);

    for (i=0; i<pvPtr->maxVisibleColumns; i++) {
	FSAddPathViewColumn(pvPtr);
    }
    pvPtr->usedColumnCount = 0;
    
    pvPtr->selectedColumn = -1;

    return pvPtr;
}


void
FSSetPathViewMaxVisibleColumns(FSPathView *pvPtr, int columns)
{
    int curMaxVisibleColumns;
    int newFirstVisibleColumn = 0;

    assert ((int) pvPtr);
    
    columns = (columns < MIN_VISIBLE_COLUMNS) ? MIN_VISIBLE_COLUMNS : columns;
    columns = (columns > MAX_VISIBLE_COLUMNS) ? MAX_VISIBLE_COLUMNS : columns;
    if (columns == pvPtr->maxVisibleColumns) {
    	return;
    }
    curMaxVisibleColumns = pvPtr->maxVisibleColumns;
    pvPtr->maxVisibleColumns = columns;
    /* browser not loaded */
    if (!pvPtr->flags.loaded) {
    	if ((columns > curMaxVisibleColumns) && (columns > pvPtr->columnCount)) {
	    int i = columns - pvPtr->columnCount;
	    pvPtr->usedColumnCount = pvPtr->columnCount;
	    while (i--) {
	        FSAddPathViewColumn(pvPtr);
	    }
	    pvPtr->usedColumnCount = 0;
	}
    /* browser loaded and columns > curMaxVisibleColumns */
    } else if (columns > curMaxVisibleColumns) {
	if (pvPtr->usedColumnCount > columns) {
	    newFirstVisibleColumn = pvPtr->usedColumnCount - columns;
	}
	if (newFirstVisibleColumn > pvPtr->firstVisibleColumn) {
	    newFirstVisibleColumn = pvPtr->firstVisibleColumn;
	}
	if (columns > pvPtr->columnCount) {
	    int i = columns - pvPtr->columnCount;
	    int curUsedColumnCount = pvPtr->usedColumnCount;
	    pvPtr->usedColumnCount = pvPtr->columnCount;
	    while (i--) {
		FSAddPathViewColumn(pvPtr);
	    }
	    pvPtr->usedColumnCount = curUsedColumnCount;
	}
    /* browser loaded and columns < curMaxVisibleColumns */
    } else {
	newFirstVisibleColumn = pvPtr->firstVisibleColumn;
	if (newFirstVisibleColumn + columns >= pvPtr->usedColumnCount) {
	    removeColumn(pvPtr, newFirstVisibleColumn + columns);
	}
    }
/*     resizeFSPathView(pvPtr, pvPtr->view->size.width, pvPtr->view->size.height); */
    WMResizeWidget(pvPtr, pvPtr->view->size.width, pvPtr->view->size.height);
    if (pvPtr->flags.loaded) {
	XClearArea(pvPtr->view->screen->display, pvPtr->view->window, 0, 0,
		   pvPtr->view->size.width, pvPtr->titleHeight, False);
	scrollToColumn (pvPtr, newFirstVisibleColumn, True);
    }
}


int 
FSGetPathViewNumberOfColumns(FSPathView *pvPtr)
{
    return pvPtr->usedColumnCount;
}

/* FSFileButton* */
/* FSGetPathViewFileButtonInColumn(FSPathView *pvPtr, int column) */
/* { */
/*     if (column < 0 || column >= pvPtr->usedColumnCount) */
/* 	return NULL; */
    
/*     return pvPtr->columns[column]; */
/* } */


void 
FSSetPathViewFillColumnProc(FSPathView *pvPtr, FSPathViewFillColumnProc *proc)
{
    pvPtr->fillColumn = proc;
}


int 
FSGetPathViewFirstVisibleColumn(FSPathView *pvPtr)
{
    return pvPtr->firstVisibleColumn;
}


static void
removeColumn(FSPathView *pvPtr, int column)
{
    int i, clearEnd, destroyEnd;
    FSFileButton **clist;
    char **tlist;
    
    assert ((int) pvPtr);
    
    column = (column < 0) ? 0 : column;
    if (column >= pvPtr->columnCount) {
	return;
    }
    if (column < pvPtr->maxVisibleColumns) {
/* 	clearEnd = pvPtr->maxVisibleColumns; */
/* 	destroyEnd = pvPtr->columnCount; */
/* 	pvPtr->columnCount = pvPtr->maxVisibleColumns; */
	clearEnd = column;
	destroyEnd = pvPtr->columnCount;
	pvPtr->columnCount = column;
    } else {
	clearEnd = column;
	destroyEnd = pvPtr->columnCount;
	pvPtr->columnCount = column;
    }
    if (column < pvPtr->usedColumnCount) {
	pvPtr->usedColumnCount = column;
    }
    for (i=column; i < clearEnd; i++) {
	if (pvPtr->titles[i]) {
	    free(pvPtr->titles[i]);
	    pvPtr->titles[i] = NULL;
	}
/* 	WMUnmapWidget(pvPtr->columns[i]); */
	/* Unmap the widget or something here */
	FSClearFileButton(pvPtr->columns[i]);
	/* WMClearList(pvPtr->columns[i]); */
    }
    for (;i < destroyEnd; i++) {
	if (pvPtr->titles[i]) {
	    free(pvPtr->titles[i]);
	    pvPtr->titles[i] = NULL;
	}
	/* Make sure the destroying the widget frees the memeory */
	WMDestroyWidget(pvPtr->columns[i]);
	pvPtr->columns[i] = NULL;
    }
    clist = wmalloc(sizeof(FSFileButton*) * (pvPtr->columnCount));
    tlist = wmalloc(sizeof(char*) * (pvPtr->columnCount));
    memcpy(clist, pvPtr->columns, sizeof(FSFileButton*) *(pvPtr->columnCount));
    memcpy(tlist, pvPtr->titles, sizeof(char*) * (pvPtr->columnCount));
    free(pvPtr->titles);
    free(pvPtr->columns);
    pvPtr->titles = tlist;
    pvPtr->columns = clist;
}

static void 
willResizeFSPathView(W_ViewDelegate *self, WMView *view,
		     unsigned int *width, unsigned int *height)
{
    FSPathView *pvPtr = (FSPathView*)view->self;
    int cols = pvPtr->maxVisibleColumns;
    int colX, colY;
    int i;

    assert(*width > 0);
    assert(*height > 0);
    
    pvPtr->columnSize.width = (*width-(cols-1)*COLUMN_SPACING) / cols;
    pvPtr->columnSize.height = *height;
    
    colY = 0;
    
    if (pvPtr->flags.hasScroller) {
	pvPtr->columnSize.height -= SCROLLER_WIDTH + 4;

	if (pvPtr->scroller) {
	    WMResizeWidget(pvPtr->scroller, *width-2, 1);
	    WMMoveWidget(pvPtr->scroller, 1, *height-SCROLLER_WIDTH-1);
	}
    }

    colX = 0;
    for (i = 0; i < pvPtr->columnCount; i++) {
	WMResizeWidget(pvPtr->columns[i], pvPtr->columnSize.width,
		       pvPtr->columnSize.height);
	
	WMMoveWidget(pvPtr->columns[i], colX, colY);
	
	if (COLUMN_IS_VISIBLE(pvPtr, i)) {
	    colX += pvPtr->columnSize.width+COLUMN_SPACING;
	}
    }

/*     W_ResizeView(pvPtr->view, width, height); */
}

/* static void  */
/* resizeFSPathView(WMWidget *w, unsigned int width, unsigned int height) */
/* { */
/*     FSPathView *pvPtr = (FSPathView*)w; */
/*     int cols = pvPtr->maxVisibleColumns; */
/*     int colX, colY; */
/*     int i; */

/*     assert(width > 0); */
/*     assert(height > 0); */
    
/*     pvPtr->columnSize.width = (width-(cols-1)*COLUMN_SPACING) / cols; */
/*     pvPtr->columnSize.height = height; */
    
/*     colY = 0; */
    
/*     if (pvPtr->flags.hasScroller) { */
/* 	pvPtr->columnSize.height -= SCROLLER_WIDTH + 4; */

/* 	if (pvPtr->scroller) { */
/* 	    WMResizeWidget(pvPtr->scroller, width-2, 1); */
/* 	    WMMoveWidget(pvPtr->scroller, 1, height-SCROLLER_WIDTH-1); */
/* 	} */
/*     } */

/*     colX = 0; */
/*     for (i = 0; i < pvPtr->columnCount; i++) { */
/* 	WMResizeWidget(pvPtr->columns[i], pvPtr->columnSize.width, */
/* 		       pvPtr->columnSize.height); */
	
/* 	WMMoveWidget(pvPtr->columns[i], colX, colY); */
	
/* 	if (COLUMN_IS_VISIBLE(pvPtr, i)) { */
/* 	    colX += pvPtr->columnSize.width+COLUMN_SPACING; */
/* 	} */
/*     } */

/*     W_ResizeView(pvPtr->view, width, height); */
/* } */


static void
scrollCallback(WMWidget *scroller, void *self)
{
    FSPathView *pvPtr = (FSPathView*)self;
    WMScroller *sPtr = (WMScroller*)scroller;
    int newFirst;
#define LAST_VISIBLE_COLUMN  pvPtr->firstVisibleColumn+pvPtr->maxVisibleColumns

    switch (WMGetScrollerHitPart(sPtr)) {
     case WSDecrementLine:
	if (pvPtr->firstVisibleColumn > 0) {
	    scrollToColumn(pvPtr, pvPtr->firstVisibleColumn-1, True);
	}
	break;
	
     case WSDecrementPage:
	if (pvPtr->firstVisibleColumn > 0) {
	    newFirst = pvPtr->firstVisibleColumn - pvPtr->maxVisibleColumns;

	    scrollToColumn(pvPtr, newFirst, True);
	}
	break;

	
     case WSIncrementLine:
	if (LAST_VISIBLE_COLUMN < pvPtr->usedColumnCount) {
	    scrollToColumn(pvPtr, pvPtr->firstVisibleColumn+1, True);
	}
	break;
	
     case WSIncrementPage:
	if (LAST_VISIBLE_COLUMN < pvPtr->usedColumnCount) {
	    newFirst = pvPtr->firstVisibleColumn + pvPtr->maxVisibleColumns;

	    if (newFirst+pvPtr->maxVisibleColumns >= pvPtr->columnCount)
		newFirst = pvPtr->columnCount - pvPtr->maxVisibleColumns;

	    scrollToColumn(pvPtr, newFirst, True);
	}
	break;
	
     case WSKnob:
	{
	    double floatValue;
	    double value = pvPtr->columnCount - pvPtr->maxVisibleColumns;

	    floatValue = WMGetScrollerValue(pvPtr->scroller);

	    floatValue = (floatValue*value)/value;

	    newFirst = rint(floatValue*(float)(pvPtr->columnCount - pvPtr->maxVisibleColumns));

	    if (pvPtr->firstVisibleColumn != newFirst)
		scrollToColumn(pvPtr, newFirst, False);
/*	    else
		WMSetScrollerParameters(pvPtr->scroller, floatValue,
					pvPtr->maxVisibleColumns/(float)pvPtr->columnCount);
 */

	}
	break;

     case WSKnobSlot:
     case WSNoPart:
	/* do nothing */
	break;
    }
#undef LAST_VISIBLE_COLUMN
}


static void
setupScroller(FSPathView *pvPtr)
{
    WMScroller *sPtr;
    int y;

    y = pvPtr->view->size.height - SCROLLER_WIDTH - 1;
    
    sPtr = WMCreateScroller(pvPtr);
    WMSetScrollerAction(sPtr, scrollCallback, pvPtr);
    WMMoveWidget(sPtr, 1, y);
    WMResizeWidget(sPtr, pvPtr->view->size.width-2, SCROLLER_WIDTH);
    
    pvPtr->scroller = sPtr;
    
    WMMapWidget(sPtr);
}


void
FSSetPathViewAction(FSPathView *pvPtr, WMAction *action, void *clientData)
{
    pvPtr->action = action;
    pvPtr->clientData = clientData;
}


void
FSSetPathViewDoubleAction(FSPathView *pvPtr, WMAction *action, 
			  void *clientData)
{
    pvPtr->doubleAction = action;
    pvPtr->doubleClientData = clientData;
}


void
FSSetPathViewHasScroller(FSPathView *pvPtr, int hasScroller)
{
    pvPtr->flags.hasScroller = hasScroller;

    if(pvPtr->flags.hasScroller)
	WMMapWidget(pvPtr->scroller);
    else
	WMUnmapWidget(pvPtr->scroller);
}

static void
loadColumn(FSPathView *pvPtr, int column)
{
    assert(pvPtr->fillColumn);
    pvPtr->flags.loadingColumn = 1;
    (*pvPtr->fillColumn)(pvPtr, column);
    pvPtr->flags.loadingColumn = 0;
}


static void
paintPathView(FSPathView *pvPtr)
{
    int i;

    if (!pvPtr->view->flags.mapped)
	return;

/* 	W_DrawRelief(pvPtr->view->screen, pvPtr->view->window, 0, 0, */
/* 		     pvPtr->view->size.width, pvPtr->view->size.height, WRSunken); */
    if(pvPtr->flags.hasScroller)
	W_DrawRelief(pvPtr->view->screen, pvPtr->view->window, 0, 
		     pvPtr->view->size.height-SCROLLER_WIDTH-2,
		     pvPtr->view->size.width, 22, WRSunken);
    
}

static void
handleEvents(XEvent *event, void *data)
{
    FSPathView *pvPtr = (FSPathView*)data;

/*     CHECK_CLASS(data, fsPathViewWidgetClass); */


    switch (event->type) {
     case Expose:
	paintPathView(pvPtr);
	break;
	
     case DestroyNotify:
	destroyPathView(pvPtr);
	break;
	
    }
}



static void
scrollToColumn(FSPathView *pvPtr, int column, Bool updateScroller)
{
    int i;
    int x;
    int notify = 0;

    
    if (column != pvPtr->firstVisibleColumn)
	notify = 1;

    if (column < 0)
	column = 0;

    x = 0;
    pvPtr->firstVisibleColumn = column;
    for (i = 0; i < pvPtr->columnCount; i++) {
	if (COLUMN_IS_VISIBLE(pvPtr, i)) {
	    WMMoveWidget(pvPtr->columns[i], x,
			 WMWidgetView(pvPtr->columns[i])->pos.y);
	    if (!WMWidgetView(pvPtr->columns[i])->flags.realized)
		WMRealizeWidget(pvPtr->columns[i]);
	    WMMapWidget(pvPtr->columns[i]);
	    x += pvPtr->columnSize.width + COLUMN_SPACING;
	} else {
	    WMUnmapWidget(pvPtr->columns[i]);
	}
    }

    /* update the scroller */
    if (updateScroller) {
	if (pvPtr->columnCount > pvPtr->maxVisibleColumns) {
	    float value, proportion;

	    value = pvPtr->firstVisibleColumn
		/(float)(pvPtr->columnCount-pvPtr->maxVisibleColumns);
	    proportion = pvPtr->maxVisibleColumns/(float)pvPtr->columnCount;
	    WMSetScrollerParameters(pvPtr->scroller, value, proportion);
	} else {
	    WMSetScrollerParameters(pvPtr->scroller, 0, 1);
	}
    }
    
    if (pvPtr->view->flags.mapped)
	paintPathView(pvPtr);

    if (notify)
    {
	int *col;
	
	col = &column;
	WMPostNotificationName(FSPathViewDidScrollNotification, pvPtr, col);
    }
/* 	WMPostNotificationName(FSPathViewDidScrollNotification, pvPtr, NULL); */
}


static void
btnCallback(void *self, void *clientData)
{
    FSPathView *pvPtr = (FSPathView*)clientData;
    FSFileButton *bPtr = (FSFileButton*)self;
    static FSFileButton *oldBtn = NULL;
    int i;

    for (i=0; i<pvPtr->columnCount; i++) 
    {
	if (bPtr == pvPtr->columns[i])
	{
	    pvPtr->lastClicked = i;
	    break;
	}
    }
    

/* 	pvPtr->selectedColumn = i; */

        /* columns at right must be cleared */
/*         removeColumn(pvPtr, i+1);  */
/* 	FSAddPathViewColumn(pvPtr);  */
/*         if (pvPtr->usedColumnCount < pvPtr->maxVisibleColumns) */
/*             i = 0;  */

/* 	if(!COLUMN_IS_VISIBLE(pvPtr, i+1))  */
/* 	    scrollToColumn(pvPtr, pvPtr->firstVisibleColumn+1, True);  */
/* 	else  */
/* 	    scrollToColumn(pvPtr, pvPtr->firstVisibleColumn, True);  */

/* 	scrollToColumn(pvPtr, i, True); */
/* 	loadColumn(pvPtr, pvPtr->usedColumnCount-1); */
/*     }  */
  

    /* call callback for click */
    if (pvPtr->action) 
	(*pvPtr->action)(pvPtr, pvPtr->clientData); 
    
/*     oldBtn = bPtr;  */
} 


static void
btnDoubleCallback(void *self, void *clientData)
{
    FSPathView *pvPtr = (FSPathView*)clientData;

    /* call callback for double click */
    if (pvPtr->doubleAction) 
	(*pvPtr->doubleAction)(pvPtr, pvPtr->doubleClientData);
} 


void
FSLoadPathViewColumnZero(FSPathView *pvPtr)
{
    if (!pvPtr->flags.loaded) 
    {
	/* create column 0 */
	FSAddPathViewColumn(pvPtr);

	loadColumn(pvPtr, 0);
	    
	/* make column 0 visible */
	scrollToColumn(pvPtr, 0, True);

	pvPtr->flags.loaded = 1;
    }
    else
    {
	removeColumn(pvPtr, 1);
	loadColumn(pvPtr, 0);
	/* make column 0 visible */
	scrollToColumn(pvPtr, 0, True);
    }

}

int
FSAddPathViewColumn(FSPathView *pvPtr)
{
    FSFileButton *list;
    FSFileButton **clist;
    char **tlist;
    int colY;
    int index;

    if (pvPtr->usedColumnCount < pvPtr->columnCount) {
	return pvPtr->usedColumnCount++;
    }

    pvPtr->usedColumnCount++;

    colY = 0;

    index = pvPtr->columnCount;
    pvPtr->columnCount++;
    clist = wmalloc(sizeof(FSFileButton*)*pvPtr->columnCount);
    tlist = wmalloc(sizeof(char*)*pvPtr->columnCount);
    memcpy(clist, pvPtr->columns, sizeof(FSFileButton*)*(pvPtr->columnCount-1));
    memcpy(tlist, pvPtr->titles, sizeof(char*)*(pvPtr->columnCount-1));
    if (pvPtr->columns)
	free(pvPtr->columns);
    if (pvPtr->titles)
	free(pvPtr->titles);
    pvPtr->columns = clist;
    pvPtr->titles = tlist;

    pvPtr->titles[index] = NULL;

    list = FSCreateFileButton(pvPtr);
    FSSetFileButtonAction(list, btnCallback, pvPtr);
    FSSetFileButtonDoubleAction(list, btnDoubleCallback, pvPtr);
   /* Drag'n'Drop */
    DndRegisterDropWidget(list, handlePVButtonDrop, list);
    DndRegisterDragWidget(list, handlePVButtonDrag, list);
    WMHangData(list, pvPtr);

    pvPtr->columns[index] = list;

    WMResizeWidget(list, pvPtr->columnSize.width, pvPtr->columnSize.height);
    WMMoveWidget(list, (pvPtr->columnSize.width+COLUMN_SPACING)*index, colY);
    if (COLUMN_IS_VISIBLE(pvPtr, index)) 
	WMMapWidget(list);

    /* update the scroller */
    if (pvPtr->columnCount > pvPtr->maxVisibleColumns)
    {
	float value, proportion;

	value = pvPtr->firstVisibleColumn
	    /(float)(pvPtr->columnCount-pvPtr->maxVisibleColumns);
	proportion = pvPtr->maxVisibleColumns/(float)pvPtr->columnCount;
	WMSetScrollerParameters(pvPtr->scroller, value, proportion);
    }

    return index;
}



static void
destroyPathView(FSPathView *pvPtr)
{
    int i;

    for (i=0; i<pvPtr->columnCount; i++) {
	if (pvPtr->titles[i])
	    free(pvPtr->titles[i]);
    }
    free(pvPtr->titles);
    
    free(pvPtr->pathSeparator);
    
/*     WMRemoveNotificationObserver(pvPtr); */
    
    free(pvPtr);
}


static char*
createTruncatedString(WMFont *font, char *text, int *textLen, int width)
{
    int dLen = WMWidthOfString(font, ".", 1);
    char *textBuf = (char*)wmalloc((*textLen)+4);

    if (width >= 3*dLen) {
	int dddLen = 3*dLen;
	int tmpTextLen = *textLen;
	
	strcpy(textBuf, text);
	while (tmpTextLen
	       && (WMWidthOfString(font, textBuf, tmpTextLen)+dddLen > width))
	    tmpTextLen--;
	strcpy(textBuf+tmpTextLen, "...");
	*textLen = tmpTextLen+3;
    } else if (width >= 2*dLen) {
	strcpy(textBuf, "..");
	*textLen = 2;
    } else if (width >= dLen) {
	strcpy(textBuf, ".");
	*textLen = 1;
    } else {
	*textBuf = '\0';
	*textLen = 0;
    }
    return textBuf;
}

void
FSSetPathViewToColumn(FSPathView *pvPtr, int column)
{
    int i;
    int first = pvPtr->firstVisibleColumn;

    /* FSLoadPathViewColumnZero must be called first */
    if (!pvPtr->flags.loaded) {
	return;
    }
    i = column;

    if(column > pvPtr->usedColumnCount)
    {
	for(i = pvPtr->usedColumnCount-1; i < column-1; i++)
	{
	    FSAddPathViewColumn(pvPtr);
	    loadColumn(pvPtr, i+1);
	}
	i = pvPtr->columnCount - pvPtr->maxVisibleColumns;
	if(i < 0)
	    i = 0;
    }
    else if(column < pvPtr->usedColumnCount)
    {
	removeColumn(pvPtr, column+1);
	loadColumn(pvPtr, column);
	if(column < pvPtr->maxVisibleColumns && first != 0)
	    i = -1;
	else
	    i = first;
    }
    else
    {
	removeColumn(pvPtr, column+1);
	if(COLUMN_IS_VISIBLE(pvPtr, column))
	    i = first;
	else
	    i = first+1;

	FSAddPathViewColumn(pvPtr);
	loadColumn(pvPtr, column);
    }

    if(i >= 0)
	scrollToColumn(pvPtr, i, True);
}

void
FSSetPathViewColumnContents(FSPathView *pvPtr, int column, 
			    char *pathname, int isBranch, int backlight)
{
    FSSetFileButtonPathname(pvPtr->columns[column], pathname, isBranch);
    if(column >= 1)
    {
	if(backlight)
	    FSSetFileButtonSelected(pvPtr->columns[column], 1);
	FSSetFileButtonSelected(pvPtr->columns[column-1], 0);
    }
}

static void 
handlePVButtonDrop(XEvent *ev, void *clientData)
{
    unsigned long  Size;
    unsigned int   Type,Keys;
    unsigned char *data  = NULL;
    FileInfo      *src   = NULL;
    FileInfo      *dest  = NULL;
    FSFileButton  *btn   = (FSFileButton *) clientData;
    FSPathView    *pvPtr = WMGetHangedData(btn);
    
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

	src  = FSGetFileInfo(data);
	dest = FSGetFileInfo(FSGetFileButtonPathname(btn));

	srcPath = GetPathnameFromPathName(src->path, src->name);
	destPath = GetPathnameFromPathName(dest->path, dest->name);
	
	if(strcmp(srcPath, destPath) != 0)
	{
	    if(Keys & ShiftMask) /* Copy */
	    {
		FSCopy(src, dest); 
/* 		insertIntoColumn(bPtr, destPath, src->name); */
	    }
	    else 
	    {
		FSMove(src, dest);
/* 		removeFromColumn(bPtr, src->path); */
/* 		insertIntoColumn(bPtr, destPath, src->name); */
		/* Check if it was on the shelf??? */
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
handlePVButtonDrag(XEvent *ev, void *clientData)
{
    int           type;
    char         *path     = NULL;
    WMPixmap     *pixmap   = NULL;
    FileInfo     *fileInfo = NULL;
    FSFileButton *btn   = (FSFileButton *) clientData;
  
    if (!DragStarted) 
	return;
    
    path = FSGetFileButtonPathname(btn);
    fileInfo = FSGetFileInfo(path);
    type = FSGetDNDType(fileInfo);
    
    DndSetData(type, path, (unsigned long)(strlen(path)+1));

    pixmap = FSCreateBlurredPixmapFromFile(WMWidgetScreen(btn), 
					   fileInfo->imgName);
    DndHandleDragging(btn, ev, pixmap);

    if(pixmap)
	WMReleasePixmap(pixmap);
    if(path) 
	free(path);
}

short
FSGetPathViewLastClicked(FSPathView *pvPtr)
{
    return pvPtr->lastClicked;
}
