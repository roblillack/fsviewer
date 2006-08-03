/* iconInspector.c */

#include "FSViewer.h"
#include "FSUtils.h"

#define WIDTH               272
#define HEIGHT              272
#define LABEL_HEIGHT         16
#define LABEL_WIDTH          48

typedef struct _Panel {
    WMFrame      *frame;
    char         *sectionName;
    
    CallbackRec   callbacks;

    Display    	 *dpy;
    WMScreen	 *scr;
    FSViewer   	 *app;
    WMWindow     *win;

    WMLabel      *iconLabel;
    WMList       *pathList;
    WMList       *fileList;
    WMButton     *okBtn;
    WMButton     *revertBtn;

    FileInfo     *fileInfo;

    char         *xpmDir;
    char         *tiffDir;
    char         *iconName;

    int           x;
    int           y;

} _Panel;

static char *getSelectedFilename(_Panel *panel);

static void
setIconLabel(WMWidget *self, void *data)
{
    RColor    color;
    WMPixmap *pixmap;
    _Panel *panel = (_Panel *) data;
    
    color.red   = 0xae;
    color.green = 0xaa;
    color.blue  = 0xae;
    color.alpha = 0;
    
    if(panel->iconName)
	free(panel->iconName);
    panel->iconName = getSelectedFilename(panel);
          /* FS.. */
    pixmap = WMCreateBlendedPixmapFromFile(panel->scr, 
					   LocateImage(panel->iconName), 
					   &color);
    WMSetLabelImage(panel->iconLabel, pixmap);
}


static void
fillIconFileList(WMWidget *self, void *data)
{
    _Panel *panel = (_Panel *) data;
    char *pathname = NULL;
    WMListItem *listItem;
    FileInfo   *fileList;

    WMClearList(panel->fileList);
    WMSetLabelImage(panel->iconLabel, NULL);

    listItem = WMGetListSelectedItem(panel->pathList);
    pathname = (char *) wmalloc(strlen(listItem->text)+1);
    strcpy(pathname, listItem->text);
    strcat(pathname, "/");
    

    fileList = GetDirList(pathname);
    while(fileList != NULL)
    {
	if(DisplayFile(fileList->name, NULL, fileList->fileType))
	{
	    listItem = WMAddListItem(panel->fileList, fileList->name);
	    listItem->clientData = fileList;
	}

	fileList = fileList->next;
    }

    if(pathname)
    {
	free(pathname);
    }
    WMSortListItems(panel->fileList);
}

static char *
getSelectedFilename(_Panel *panel)
{
    WMListItem *listItem;
    FileInfo   *fileInfo;
    char       *filename;
    char       *imgName;

    imgName  = NULL;
    filename = NULL;
    listItem = NULL;

    listItem = WMGetListSelectedItem(panel->fileList);
    if(listItem)
    {
	RColor color;
	WMPixmap *pixmap;
	
	color.red   = 0xae;
	color.green = 0xaa;
	color.blue  = 0xae;
	color.alpha = 0;
	
	fileInfo = (FileInfo *) listItem->clientData;
	
	if(!strncmp(fileInfo->path, panel->xpmDir, strlen(fileInfo->path)-1) ||
	   !strncmp(fileInfo->path, panel->tiffDir, strlen(fileInfo->path)-1) )
	{
	    filename = RemoveFileExtension(fileInfo->name);
	}
	else
	    filename = GetPathnameFromPathName(fileInfo->path, fileInfo->name);
	
	imgName = LocateImage(filename);
	      /* FS..*/
 	pixmap = WMCreateBlendedPixmapFromFile(WMWidgetScreen(panel->win),
					       imgName, &color);

	WMSetLabelImage(panel->iconLabel, pixmap);
	WMReleasePixmap(pixmap);
    }
    
    if(imgName)
	free(imgName);

    return filename;
}

static void
showData(_Panel *panel)
{
    char *file;
    RColor color;
    WMPixmap *pixmap;
    
    color.red =   0xae;
    color.green = 0xaa;
    color.blue =  0xae;
    color.alpha = 0;
          /* FS..*/ 
    pixmap = WMCreateBlendedPixmapFromFile(panel->scr, 
					   panel->fileInfo->imgName, 
					   &color);
    WMClearList(panel->pathList);
    WMClearList(panel->fileList);

    FSLoadIconPaths(panel->pathList);
    WMAddListItem(panel->pathList, panel->xpmDir);
    WMAddListItem(panel->pathList, panel->tiffDir);
    WMSortListItems(panel->pathList);

    WMSetLabelImage(panel->iconLabel, pixmap);
}

static void
storeData(_Panel *panel)
{
    char *filename = NULL;

    filename = getSelectedFilename(panel);
    if(filename)
    {	
	FSSetStringForNameKey(panel->fileInfo->extn, "icon", filename);
	free(filename);
    }
}

static void
buttonClick(WMWidget *self, void *data)
{
    Panel *panel = (Panel *)data;

    if ((WMButton *)self == panel->okBtn) 
    {
	storeData(panel);
	WMSetButtonEnabled(panel->revertBtn, True);
	FSUpdateFileViewPath(FSGetFSViewerCurrentView(panel->app));
    }
    else {
      WMRunAlertPanel(panel->scr, NULL,
        _("Inspector Error"), 
	_("This function has not yet been implemented."),
	_("OK"), NULL, NULL);
        /* reloadDefaults(); */
	WMSetButtonEnabled(panel->revertBtn, False);
    }
}

static void
createButtons(Panel *panel)
{

    panel->revertBtn = WMCreateCommandButton(panel->frame);
    WMMoveWidget(panel->revertBtn, 16, HEIGHT-24);
    WMResizeWidget(panel->revertBtn, 115, 24);
    WMSetButtonText(panel->revertBtn, _("Revert"));
    WMSetButtonEnabled(panel->revertBtn, False);
    WMSetButtonAction(panel->revertBtn, buttonClick, panel);
   
    panel->okBtn = WMCreateCommandButton(panel->frame);
    WMMoveWidget(panel->okBtn, 140, HEIGHT-24);
    WMResizeWidget(panel->okBtn, 115, 24);
    WMSetButtonText(panel->okBtn, _("Set Default"));
    WMSetButtonImage(panel->okBtn, 
		     WMGetSystemPixmap(panel->scr, WSIReturnArrow));
    WMSetButtonAltImage(panel->okBtn, 
			WMGetSystemPixmap(panel->scr, 
					  WSIHighlightedReturnArrow));
    WMSetButtonImagePosition(panel->okBtn, WIPRight);
    WMSetButtonEnabled(panel->okBtn, True);
    WMSetButtonAction(panel->okBtn, buttonClick, panel);   
}

static void
createPanel(Panel *p)
{
    WMFrame *f = NULL;
    WMFont *aFont = NULL;
    WMLabel *l = NULL;

    _Panel *panel = (_Panel*)p;
    panel->frame = WMCreateFrame(panel->win);

    WMResizeWidget(panel->frame, WIDTH, HEIGHT);
    WMMoveWidget(panel->frame, panel->x, panel->y);
    WMSetFrameRelief(panel->frame, WRFlat);

    f = WMCreateFrame(panel->frame);
    WMMoveWidget(f, 0, 0);
    WMResizeWidget(f, WIDTH, HEIGHT);
    WMSetFrameRelief(f, WRFlat);

    aFont = WMBoldSystemFontOfSize(panel->scr, 12);
    l = WMCreateLabel(f);
    WMMoveWidget(l, 10, 0);
    WMResizeWidget(l, 252, 20);
    WMSetLabelText(l, _("Directories"));
    WMSetLabelRelief(l, WRSunken);
    WMSetWidgetBackgroundColor(l, WMDarkGrayColor(panel->scr));
    WMSetLabelTextColor(l, WMWhiteColor(panel->scr));
    WMSetLabelFont(l, aFont);
    WMSetLabelTextAlignment(l, WACenter);

    panel->pathList = WMCreateList(f);
    WMMoveWidget(panel->pathList, 10, 22);
    WMResizeWidget(panel->pathList, 252, 87);
    WMSetListAction(panel->pathList, fillIconFileList, panel);
    FSLoadIconPaths(panel->pathList);
    WMAddListItem(panel->pathList, panel->xpmDir);
    WMAddListItem(panel->pathList, panel->tiffDir);
   
    l = WMCreateLabel(f);
    WMMoveWidget(l, 10, 114);
    WMResizeWidget(l, 252, 20);
    WMSetLabelText(l, _("Files"));
    WMSetLabelRelief(l, WRSunken);
    WMSetWidgetBackgroundColor(l, WMDarkGrayColor(panel->scr));
    WMSetLabelTextColor(l, WMWhiteColor(panel->scr));
    WMSetLabelFont(l, aFont);
    WMSetLabelTextAlignment(l, WACenter);

    panel->fileList = WMCreateList(f);
    WMMoveWidget(panel->fileList, 10, 134);
    WMResizeWidget(panel->fileList, 181, 100);
    WMSetListAction(panel->fileList, setIconLabel, panel);

    panel->iconLabel = WMCreateLabel(f);
    WMMoveWidget(panel->iconLabel, 192, 134);
    WMResizeWidget(panel->iconLabel, 70, 100);
    WMSetLabelImagePosition(panel->iconLabel, WIPImageOnly);
    WMSetLabelRelief(panel->iconLabel, WRSunken);

    createButtons(panel);
    WMMapWidget(f);
    WMMapSubwidgets(f);
    WMRealizeWidget(panel->frame);
    WMMapSubwidgets(panel->frame);
}

Panel*
InitIcon(WMWindow *parent, FSViewer *app, FileInfo *fileInfo, int x, int y)
{
    char   *txt = NULL;
    _Panel *panel;
	  
    panel = wmalloc(sizeof(_Panel));
    memset(panel, 0, sizeof(_Panel));

    panel->sectionName = (char *) wmalloc(strlen(_("Icon Inspector"))+1);
    strcpy(panel->sectionName, _("Icon Inspector"));

    panel->win = parent;
    panel->app = app;
    panel->scr = FSGetFSViewerScreen(app);
    panel->dpy = WMScreenDisplay(panel->scr);

    panel->callbacks.createWidgets = createPanel;

    panel->callbacks.updateDomain = storeData;
    panel->callbacks.updateDisplay = showData;
    
    panel->fileInfo = fileInfo;

    txt = FSGetStringForName("ICONDIR");
    if(!txt) 
	txt = ICONDIR;
	
    panel->xpmDir = (char *) wmalloc(strlen(txt)+5);
    strcpy(panel->xpmDir, txt);
    strcat(panel->xpmDir, "/xpm");
    
    panel->tiffDir = (char *) wmalloc(strlen(txt)+6);
    strcpy(panel->tiffDir, txt);
    strcat(panel->tiffDir, "/tiff");
    
    if(txt != ICONDIR) 
	free(txt);

    panel->x = x;
    panel->y = y;

    return panel;
}
