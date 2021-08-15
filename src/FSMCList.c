#include <WMaker.h>
#include <WINGs/WINGsP.h>
#include <grp.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>

#include "FSViewer.h"
#include "FSMCList.h"
#include "FSUtils.h"
#include "FSBrowser.h"
#include "DnD.h"
#include "files.h"
#include "xpm/list_file.xpm"
#include "xpm/list_dir.xpm"

#define DEFAULT_SEPARATOR	"/"
#define LISTITEM_HEIGHT         20

typedef struct _FSMCList {

    WMFrame       *parent;
    WMScrollView  *scrollV;
    WMFrame       *mainF;
    FSBrowser     *browser;
    WMSize         size;

    WMLabel       *titles[6];
    char          *filter;

    void          *clientData;
    WMAction      *action;
    void          *doubleClientData;
    WMAction      *doubleAction;

} _FSMCList;

static void  handleDoubleAction(WMWidget *self, void *data);
static void  handleItemSelection(WMWidget *self, void *clientData);
static void  fillBrowserColumn(FSBrowser *bPtr, int column);
static void  paintItem(WMList *lPtr, int index, Drawable d, char *text,
		       int state, WMRect *rect);
static char *createTruncatedString(WMFont *font, char *text, 
				   int *textLen, int width);
char *parseItem(WMListItem *item);

FSMCList *
FSCreateMCList(WMFrame *parent)
{
    FSMCList *mPtr;

    mPtr = (FSMCList *) wmalloc(sizeof(FSMCList));
    memset(mPtr, 0, sizeof(FSMCList));
    
    mPtr->parent = parent;

    mPtr->scrollV = WMCreateScrollView(mPtr->parent);
    WMResizeWidget(mPtr->scrollV, 510, 275);
    WMMoveWidget(mPtr->scrollV, 0, 0);
    WMSetScrollViewRelief(mPtr->scrollV, WRSunken);
    WMSetScrollViewHasVerticalScroller(mPtr->scrollV, False);
    WMSetScrollViewHasHorizontalScroller(mPtr->scrollV, True);

    mPtr->mainF = WMCreateFrame(mPtr->parent);
    WMResizeWidget(mPtr->mainF, 637, 255);
    WMMoveWidget(mPtr->mainF, 0, 0);
    WMSetFrameRelief(mPtr->mainF, WRFlat);

    mPtr->size.width = 637;
    mPtr->size.height = 255;

    mPtr->titles[0] = WMCreateLabel(mPtr->mainF);
    WMSetLabelText(mPtr->titles[0], _("Name"));
    WMSetLabelTextAlignment(mPtr->titles[0], WACenter);
    WMSetLabelRelief(mPtr->titles[0], WRGroove);
    WMResizeWidget(mPtr->titles[0], 214, 20);
    WMMoveWidget(mPtr->titles[0], 0, 0);
    
    mPtr->titles[1] = WMCreateLabel(mPtr->mainF);
    WMSetLabelText(mPtr->titles[1], _("Size"));
    WMSetLabelTextAlignment(mPtr->titles[1], WACenter);
    WMSetLabelRelief(mPtr->titles[1], WRGroove);
    WMResizeWidget(mPtr->titles[1], 82, 20);
    WMMoveWidget(mPtr->titles[1], 213, 0);

    mPtr->titles[2] = WMCreateLabel(mPtr->mainF);
    WMSetLabelText(mPtr->titles[2], _("Last Changed"));
    WMSetLabelTextAlignment(mPtr->titles[2], WACenter);
    WMSetLabelRelief(mPtr->titles[2], WRGroove);
    WMResizeWidget(mPtr->titles[2], 100, 20);
    WMMoveWidget(mPtr->titles[2], 294, 0);

    mPtr->titles[3] = WMCreateLabel(mPtr->mainF);
    WMSetLabelText(mPtr->titles[3], _("Permissions"));
    WMSetLabelTextAlignment(mPtr->titles[3], WACenter);
    WMSetLabelRelief(mPtr->titles[3], WRGroove);
    WMResizeWidget(mPtr->titles[3], 82, 20);
    WMMoveWidget(mPtr->titles[3], 393, 0);

    mPtr->titles[4] = WMCreateLabel(mPtr->mainF);
    WMSetLabelText(mPtr->titles[4], _("Owner"));
    WMSetLabelTextAlignment(mPtr->titles[4], WACenter);
    WMSetLabelRelief(mPtr->titles[4], WRGroove);
    WMResizeWidget(mPtr->titles[4], 82, 20);
    WMMoveWidget(mPtr->titles[4], 474, 0);

    mPtr->titles[5] = WMCreateLabel(mPtr->mainF);
    WMSetLabelText(mPtr->titles[5], _("Group"));
    WMSetLabelTextAlignment(mPtr->titles[5], WACenter);
    WMSetLabelRelief(mPtr->titles[5], WRGroove);
    WMResizeWidget(mPtr->titles[5], 82, 20);
    WMMoveWidget(mPtr->titles[5], 555, 0);

    mPtr->browser = FSCreateBrowser(mPtr->mainF);
    FSSetBrowserHasScroller(mPtr->browser, False);
    FSSetBrowserTitled(mPtr->browser, False);
    FSSetBrowserRelief(mPtr->browser, WRFlat);
    WMMoveWidget(mPtr->browser, 0, 20); 
    WMResizeWidget(mPtr->browser, 637, 235);  
    FSSetBrowserFillColumnProc(mPtr->browser, fillBrowserColumn); 
    FSSetBrowserGetItemNameProc(mPtr->browser, parseItem);
    FSSetBrowserPathSeparator(mPtr->browser, DEFAULT_SEPARATOR);
    FSSetBrowserAction(mPtr->browser, handleItemSelection, mPtr);
    FSSetBrowserDoubleAction(mPtr->browser, handleDoubleAction, mPtr);
    FSSetBrowserDisplayFileCol(mPtr->browser, 0);
    WMHangData(mPtr->browser, mPtr);
    FSSetBrowserMaxVisibleColumns(mPtr->browser, 1);

    WMMapSubwidgets(mPtr->mainF);
    WMMapWidget(mPtr->mainF);
    WMMapWidget(mPtr->scrollV);

    WMSetScrollViewContentView(mPtr->scrollV, WMWidgetView(mPtr->mainF));

    return mPtr;
}

char * 
parseItem(WMListItem *item)
{
    int i;
    char *str;
    char *tmp;
    char *text = NULL;


    i = 0;
    str = wstrdup(item->text);
    tmp = strtok(str, "==");
    while (tmp) 
    {
	if(i==1)
	    text = wstrdup(tmp);

	tmp = strtok(NULL, "==");
	i++;
    }
    free(str);

    if(!text)
	text = wstrdup(item->text);

    return text;
}

static char *
formatFileInfo(WMScreen *scr, FileInfo *fileInfo)
{
    char sep[3] = "||";
    char *img   = NULL;
    char *text  = NULL;
    char *name  = NULL;
    char size[128];
    char time[128];
    char mode[11]  = "----------";
    char *owner = NULL;
    char *group = NULL;
    struct passwd *psswd;
    struct group  *grp;
    struct tm *st_tm;

    if(isDirectory(fileInfo->fileType))
    {
	img = wstrdup("0");
	mode[0] = 'd';
    }
    else
	img = wstrdup("1");

    if(fileInfo->name)
	name = wstrdup(fileInfo->name);
    else
	name = wstrdup("nameless");
    
    snprintf(size, 128, "%ld", (long) fileInfo->st->st_size);
    st_tm = localtime(&(fileInfo->st->st_ctime));
    strftime(time, 128, "%b %d %H:%M", st_tm);

    if(fileInfo->st->st_mode & S_IRUSR)
	 mode[1] = 'r';
    if(fileInfo->st->st_mode & S_IWUSR)
	 mode[2] = 'w';
    if(fileInfo->st->st_mode & S_IXUSR)
	 mode[3] = 'x';
    if(fileInfo->st->st_mode & S_IRGRP)
	 mode[4] = 'r';
    if(fileInfo->st->st_mode & S_IWGRP)
	 mode[5] = 'w';
    if(fileInfo->st->st_mode & S_IXGRP)
	 mode[6] = 'x';
    if(fileInfo->st->st_mode & S_IROTH)
	 mode[7] = 'r';
    if(fileInfo->st->st_mode & S_IWOTH)
	 mode[8] = 'w';
    if(fileInfo->st->st_mode & S_IXOTH)
	 mode[9] = 'x';

    grp = getgrgid(fileInfo->st->st_gid);
    if(grp)
	group = wstrdup(grp->gr_name);
    else
	group = wstrdup("nogroup");

    psswd = getpwuid(fileInfo->st->st_uid);
    if(psswd)
	owner = wstrdup(psswd->pw_name); 
    else
	owner = wstrdup("ownerless");
    
    text = (char *) wmalloc(strlen(img) + strlen(name) + strlen(size) + 
			    strlen(time) + strlen(mode) + strlen(group) +
			    strlen(owner)+13);
/*     strcpy(text, sep); */
/*     strcat(text, img); */
    strcpy(text, img);
    strcat(text, sep);
    strcat(text, name);
    strcat(text, sep);
    strcat(text, size);
    strcat(text, sep);
    strcat(text, time);
    strcat(text, sep);
    strcat(text, mode);
    strcat(text, sep);
    strcat(text, owner);
    strcat(text, sep);
    strcat(text, group);
/*     strcat(text, sep); */
    
    return text;
}

static void 
fillBrowserColumn(FSBrowser *bPtr, int column)
{
    char        *path;
    FileInfo    *fileList;
    FileInfo    *start = NULL;
    WMListItem  *listItem;
    FSMCList    *mPtr = (FSMCList *)(WMGetHangedData(bPtr));
    WMList      *list = FSGetBrowserListInColumn(bPtr, column);
    WMScreen    *scr  = WMWidgetScreen(list);

    /*
     * Override the FSBrowser drawing procedure
     */
    if(list)
    {
	WMSetListUserDrawProc(list, paintItem);	
	WMSetListUserDrawItemHeight(list, LISTITEM_HEIGHT);
    }

    FSSetBusyCursor(bPtr, True);

    path = FSGetBrowserPathToColumn(bPtr, column);
    if(path)
    {
	start = GetDirList(path);
	fileList = start;

	while(fileList != NULL)
	{
	    if( DisplayFile(fileList->name, mPtr->filter, fileList->fileType) )
	    {
		listItem = FSInsertBrowserItem(bPtr, column, -1, 
					       fileList->name,
					       isDirectory(fileList->fileType));
		listItem->clientData = formatFileInfo(scr, fileList);
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
/*     int col; */
/*     char *path; */
/*     FileBrowser *bPtr = (FileBrowser *) clientData; */

/*     col = FSGetFileBrowserColumnCount(bPtr); */

/*     FSSetPathViewToColumn(bPtr->pathView, col-1); */
/*     if(bPtr->text) */
/*     { */
/* 	wwarning("%s %d: %s", __FILE__, __LINE__, bPtr->text); */
/* 	free(bPtr->text); */
/* 	bPtr->text = NULL; */
/*     } */
}

static void 
handleDoubleAction(WMWidget *self, void *clientData)
{
/*     FileBrowser *bPtr = (FileBrowser *) clientData; */

/*     if (bPtr->doubleAction)  */
/* 	(*bPtr->doubleAction)(bPtr, bPtr->doubleClientData); */
}

void
FSLoadMCListColumnZero(FSMCList *mPtr)
{
    FSLoadBrowserColumnZero(mPtr->browser);
}

void
FSSetMCListPath(FSMCList *mPtr, char *path)
{
    FSSetBrowserPath(mPtr->browser, path);
}

int
FSGetMCListNumberOfColumns(FSMCList *mPtr)
{
    return FSGetBrowserNumberOfColumns(mPtr->browser);
}

void
FSResizeMCList(FSMCList *mPtr, unsigned int width, unsigned int height)
{
    if(mPtr->size.width < width)
    {
	WMResizeWidget(mPtr->mainF, width-3, height);
	WMResizeScrollViewContent(mPtr->scrollV, width-3, height);
	WMResizeWidget(mPtr->scrollV, width, height);
	WMResizeWidget(mPtr->browser, width-3, height-40);
    }
    else
    {
	WMResizeScrollViewContent(mPtr->scrollV, mPtr->size.width-3, height);
	WMResizeWidget(mPtr->scrollV, width, height);
	WMResizeWidget(mPtr->browser, mPtr->size.width, height-40);
	WMResizeWidget(mPtr->mainF, mPtr->size.width, height);
    }

}

char *
FSGetMCListPath(FSMCList *mPtr)
{
    int   len;
    char *path  = NULL;
    char *npath = NULL;

    path  = FSGetBrowserPath(mPtr->browser);
    len = strlen(path);
    if(len > 1 && path[len-1] == '/')
    {
	npath = (char *)wmalloc(len); 
	strncpy(npath, path, len-1);
	npath[len-1] = '\0';
    }
    else
	npath = wstrdup(path);

    if(path)
	free(path);
    return npath;
}

char *
FSGetMCListPathToColumn(FSMCList *mPtr, int column)
{
    return FSGetBrowserPathToColumn(mPtr->browser, column);
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

static void
paintItem(WMList *lPtr, int index, Drawable d,
	  char *text, int state, WMRect *rect)
{
    int i = 0;
    int j = 0;
    int k = 0;
    int img = 1;
    int fontHeight = 14;
    char tmp[512];
    char *items[7];
    char     *str    = NULL;
    WMView   *view   = W_VIEW(lPtr);
    W_Screen *scr    = view->screen;
    int width, height, x, y, len;
    WMListItem *item = WMGetListItem(lPtr, index);

    width = rect->size.width;
    height = rect->size.height;
    x = rect->pos.x;
    y = rect->pos.y;
    fontHeight = WMFontHeight(scr->normalFont);

    /* Highlight the selected area */
    if (state & WLDSSelected)
        WMPaintColorSwatch(WMWhiteColor(scr), d, x, y,
		       width, height);
    else
        WMPaintColorSwatch(WMGetWidgetBackgroundColor(lPtr), d, x, y,
		       width, height);

    if(item->clientData)
    {
	str = wstrdup(item->clientData);
	len  = strlen(str);
    }
    else
    {
	str = NULL;
	len = 0;
    }

    /* simple parse of the clientdata */
    for(i = 0; i < len; i++)
    {
	if(/*  str[i-1] != '=' &&  */str[i] != '|')
	{
	    tmp[k] = str[i];
	    k++;
	}
	else
	{
	    tmp[k] = '\0';
	    i +=1;
	    k = 0;
	    items[j++] = wstrdup(tmp);
	}
    }
    tmp[k] = '\0';
    items[j++] = wstrdup(tmp);

    /* iterate through items and draw each one */
    for(i = 0; i < j; i++)
    {
	if(i == 0)
	{
	    if(strcmp(items[i], "0") == 0)
		img = 0;
	    else
		img = 1;
	    /* do nothing for the moment */
	}
	else if(i==1)
	{
	    WMSize size;
	    WMPixmap *image;
	    int widthC  = 168; /*194 - 26*/
	    int textLen = strlen(items[i]);

	    if(FSGetIntegerForName("DisplayMCListPixmap"))
	    {
		if(img == 0)
		    image = WMCreatePixmapFromXPMData(scr, list_dir_xpm);
		else
		    image = WMCreatePixmapFromXPMData(scr, list_file_xpm);
		size  = WMGetPixmapSize(image);
		WMDrawPixmap(image, d,
			     x+6+(20-size.width)/2, (height-size.height)/2+y);
		WMReleasePixmap(image);

	    }
	    else
	    {
		char *tmpStr = NULL;

		if(img == 0)
		    tmpStr = wstrdup("D");
		else
		    tmpStr = wstrdup("F");

		W_PaintText(view, d, scr->boldFont,
			    x+6, y+(height-fontHeight)/2, widthC,
			    WALeft, WMDarkGrayColor(scr), False,
			    tmpStr, 1);

		free(tmpStr);
	    }

	    if (WMWidthOfString(scr->normalFont, items[i], textLen) > widthC)
	    {
		char *textBuf = createTruncatedString(scr->normalFont,
						      items[i], &textLen,
						      widthC);

		W_PaintText(view, d, scr->normalFont,
			    x+26, y+(height-fontHeight)/2, widthC,
			    WALeft, WMBlackColor(scr), False,
			    textBuf, textLen);

		free(textBuf);
	    }
	    else
	    {
		W_PaintText(view, d, scr->normalFont,
			    x+26, y+(height-fontHeight)/2, widthC,
			    WALeft, WMBlackColor(scr), False, items[i],
			    textLen);
	    }
	    x += 194;
	}
	else if(i==3)
	{
	    W_PaintText(view, d, scr->normalFont,
			x+6, y+(height-fontHeight)/2, 100,
			WALeft, WMBlackColor(scr), False, items[i],
			strlen(items[i]));
	    x += 100;
	}
	else
	{
	    W_PaintText(view, d, scr->normalFont,
			x+6, y+(height-fontHeight)/2, 82,
			WALeft, WMBlackColor(scr), False, items[i],
			strlen(items[i]));
	    x += 82;
	}
	free(items[i]);
	items[i] = NULL;
    }
    free(str);
}

void
FSSetMCListFilter(FSMCList *mPtr, char *filter)
{
    mPtr->filter = filter;
}

void
FSScrollMCListToColumn(FSMCList *mPtr, int col, Bool updateScroller)
{
    FSScrollBrowserToColumn(mPtr->browser, col, updateScroller);
}

void
FSSetMCListAction(FSMCList *mPtr, WMAction *action, void *clientData)
{
    FSSetBrowserAction(mPtr->browser, action, clientData);
}

void
FSSetMCListDoubleAction(FSMCList *mPtr, WMAction *action, void *clientData)
{
    FSSetBrowserDoubleAction(mPtr->browser, action, clientData);
}

WMListItem*
FSGetMCListSelectedItemInColumn(FSMCList *mPtr, int column)
{
    return FSGetBrowserSelectedItemInColumn(mPtr->browser, column);
}

WMList*
FSGetMCListListInColumn(FSMCList *mPtr, int column)
{
    return FSGetBrowserListInColumn(mPtr->browser, column);
}

char *
FSMCListFormatFileInfo(FSMCList *mPtr, FileInfo *fileInfo)
{
    return formatFileInfo(WMWidgetScreen(mPtr), fileInfo);
}
