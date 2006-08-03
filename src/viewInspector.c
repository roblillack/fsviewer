/* viewInspector.c */

#include <WINGs/WINGsP.h> /* for: scr->normalFont */

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

    WMWindow     *win;

    WMLabel      *defaultLabel;

    WMList       *appList;
    WMTextField  *execField;

    WMButton     *okBtn;
    WMButton     *revertBtn;
    WMButton     *appBtn;

    FileInfo     *fileInfo;

} _Panel;

static void
displayDefault(_Panel *panel)
{
    char *extn = NULL;
    char *exec = NULL;

    exec = FSGetStringForNameKey(panel->fileInfo->extn, "viewer");

    if(exec)
    {
	WMSetLabelText(panel->defaultLabel, exec);
	WMSetTextFieldText(panel->execField, exec);
    }
    else
    {
	WMSetLabelText(panel->defaultLabel, "");
	WMSetTextFieldText(panel->execField, "");
    }

    WMSetButtonImagePosition(panel->appBtn, WIPNoImage);
    WMSelectListItem(panel->appList, -1);

    if(extn)
	free(extn);
    if(exec)
	free(exec);
}


static void
showData(_Panel *panel)
{
    WMPropList* array   = NULL;

    WMClearList(panel->appList);

    array = FSGetUDObjectForKey(defaultsDB, "EXE");
    if(array && WMIsPLArray(array))
    {
	int numElem, i;
	
	numElem = WMGetPropListItemCount(array);
	for(i = 0; i < numElem; i++)	
	  WMAddListItem(panel->appList, 
	    WMGetPropListDescription(WMGetFromPLArray(array, i), False));
    }
    WMSortListItems(panel->appList);

    displayDefault(panel);
}


static void
storeData(_Panel *panel)
{
    char *exec;

    exec = WMGetTextFieldText(panel->execField);

    if(exec)
    {
	WMPropList* array = NULL;

	FSSetStringForNameKey(panel->fileInfo->extn, "viewer", exec);
	array = FSGetUDObjectForKey(defaultsDB, "EXTN");
	if(array && WMIsPLArray(array))
	    InsertArrayElement(array, WMCreatePLString(panel->fileInfo->extn));
	free(exec);
    }
}

static void
handleAppBtnEvents(XEvent *event, void *data)
{
/*     if(WMIsDoubleClick(event)) */
/* 	printf("Double Click\n"); */
}

static void
appListClick(WMWidget *self, void *data)
{
    char *tmp      = NULL;
    char *icon     = NULL;
    char *exec     = NULL;
    char *selected = NULL;
    Panel *panel   = (Panel *) data;

    selected = ((WMListItem *)WMGetListSelectedItem(panel->appList))->text;
 
    tmp = FSGetStringForNameKey(selected, "icon");
    icon = LocateImage(tmp);
    exec = FSGetStringForNameKey(selected, "exec");

    if(icon)
    {
	RColor color;
	WMPixmap *pixmap;
	
	color.red   = 0xae;
	color.green = 0xaa;
	color.blue  = 0xae;
	color.alpha = 0;
               /* FS.. */	
	pixmap = WMCreateBlendedPixmapFromFile(WMWidgetScreen(panel->win),
					       icon, &color);
	if(pixmap)
	{
	    WMSetButtonImagePosition(panel->appBtn, WIPImageOnly);
	    WMSetButtonImage(panel->appBtn, pixmap);
	}
	else
	    WMSetButtonImagePosition(panel->appBtn, WIPNoImage);
    }
    else
	WMSetButtonImagePosition(panel->appBtn, WIPNoImage);

    if(exec) {
	WMSetTextFieldText(panel->execField, exec);
    }
    else {
	WMSetTextFieldText(panel->execField, "");
    }

    if(tmp)
	free(tmp);
    if(icon)
	free(icon);
    if(exec)
	free(exec);
}

static void
buttonClick(WMWidget *self, void *data)
{
    Panel *panel = (Panel *)data;

    if ((WMButton *)self == panel->okBtn) 
    {
	storeData(panel);
	showData(panel);
	WMSetButtonEnabled(panel->revertBtn, True); 
    } 
    else if ((WMButton *)self == panel->revertBtn)
    {
	displayDefault(panel);
	WMSetButtonEnabled(panel->revertBtn, False); 
    }
    else if ((WMButton *)self == panel->appBtn)
    {
	char *exeStr = NULL;
	WMListItem *item = NULL;
	
	item = WMGetListSelectedItem(panel->appList);
	exeStr = WMGetTextFieldText(panel->execField);

	if (item && strcmp("", exeStr))
	{
	    char *exec = NULL;
	    char *path = NULL;

	    path = GetPathnameFromPathName(panel->fileInfo->path, 
					   panel->fileInfo->name);
	    
	    exec = FSParseExecString(path, exeStr);
 	    execCommand(exec);

	    if (path)
		free(path);
	    if (exec)
		free(exec);
	}
    }
}

static void 
createAppList(Panel *panel)
{
    panel->appList = WMCreateList(panel->frame);

    WMMoveWidget(panel->appList, 8, 36);
    WMResizeWidget(panel->appList, 180, 115);
    WMSetListAction(panel->appList, appListClick, panel);
}

static void
createClickLabel(Panel *panel)
{
    WMLabel *l;

    l = WMCreateLabel(panel->frame);
    WMSetLabelWraps(l, 1);
    WMResizeWidget(l, WIDTH-8, LABEL_HEIGHT*2);
    WMMoveWidget(l, 4, 0);
    WMSetLabelText(l, _("Click button to edit selected document"));
     /* WMSetLabelTextAlignment(l, WACenter); */
    WMSetLabelRelief(l, WRFlat);
    WMSetLabelTextColor(l, WMDarkGrayColor(WMWidgetScreen(panel->win)));
}    


static void
createDefaultLabels(Panel *panel)
{
    WMLabel *l;
    WMScreen *scr;
    int tw;

    l = WMCreateLabel(panel->frame);
    WMSetLabelText(l, _("Default:"));
    scr = WMWidgetScreen(panel->win);
    tw = WMWidthOfString(scr->normalFont, WMGetLabelText(l),
      strlen(WMGetLabelText(l)));
    WMResizeWidget(l, tw+4, LABEL_HEIGHT);
    WMMoveWidget(l, 4, 182);
    WMSetLabelRelief(l, WRFlat);
    WMSetLabelTextColor(l, WMDarkGrayColor(WMWidgetScreen(panel->win)));
    /* Position of "defaultLabel" is adjusted by width of "l"
       to make i18n support easier for translators */
    panel->defaultLabel = WMCreateLabel(panel->frame);
    /* Initilalization without text */
    WMSetLabelText(panel->defaultLabel,"");
    WMResizeWidget(panel->defaultLabel, WIDTH, LABEL_HEIGHT);
    /* x-Position should be: 4 + width("Default:") + 4 */
    WMMoveWidget(panel->defaultLabel, 8+tw, 182);
    WMSetLabelRelief(panel->defaultLabel, WRFlat);
}    

static void
createSetDefaultLabels(Panel *panel)
{
    WMLabel *l;

    l = WMCreateLabel(panel->frame);
    WMSetLabelWraps(l, 1);
    WMResizeWidget(l, WIDTH-8, LABEL_HEIGHT*2);
    WMMoveWidget(l, 4, 206);
    WMSetLabelText(l, _("Click \"Set Default\" to set default "\
      "application for all documents with this extension."));
    WMSetLabelTextAlignment(l, WALeft);
    WMSetLabelRelief(l, WRFlat);
    WMSetLabelTextColor(l, WMDarkGrayColor(WMWidgetScreen(panel->win)));

}    

static void
createExecField(Panel *panel)
{
    panel->execField = WMCreateTextField(panel->frame);
    WMMoveWidget(panel->execField, 8, 159);
    WMResizeWidget(panel->execField, 256, 18);
}

static void
createButtons(Panel *panel)
{
    panel->revertBtn = WMCreateCommandButton(panel->frame);
    WMMoveWidget(panel->revertBtn, 16, HEIGHT-24);
    WMResizeWidget(panel->revertBtn, 115, 24);
    WMSetButtonText(panel->revertBtn, _("Revert"));
    WMSetButtonAction(panel->revertBtn, buttonClick, panel);
    WMSetButtonEnabled(panel->revertBtn, False);
  
    panel->okBtn = WMCreateCommandButton(panel->frame);
    WMMoveWidget(panel->okBtn, 140, HEIGHT-24);
    WMResizeWidget(panel->okBtn, 115, 24);
    WMSetButtonText(panel->okBtn, _("Set Default"));
    WMSetButtonImage(panel->okBtn, 
		     WMGetSystemPixmap(WMWidgetScreen(panel->win), 
				       WSIReturnArrow));
    WMSetButtonAltImage(panel->okBtn, 
			WMGetSystemPixmap(WMWidgetScreen(panel->win), 
					  WSIHighlightedReturnArrow));
    WMSetButtonImagePosition(panel->okBtn, WIPRight);
    WMSetButtonEnabled(panel->okBtn, True);
    WMSetButtonAction(panel->okBtn, buttonClick, panel);   

    panel->appBtn = WMCreateCommandButton(panel->frame);
    WMMoveWidget(panel->appBtn, 196, 59); /* 36 + (115/2) - 68/2 = 59 */
    WMResizeWidget(panel->appBtn, 68, 68);
    WMSetButtonEnabled(panel->appBtn, True);
    WMSetButtonImagePosition(panel->appBtn, WIPImageOnly);
    WMSetButtonAction(panel->appBtn, buttonClick, panel);   
    WMCreateEventHandler(WMWidgetView(panel->appBtn), ButtonPressMask, 
			 handleAppBtnEvents, panel);
}

static void
createPanel(Panel *p)
{
    _Panel *panel = (_Panel*)p;
    panel->frame = WMCreateFrame(panel->win);

    WMResizeWidget(panel->frame, WIDTH, HEIGHT);
    WMMoveWidget(panel->frame, 0, 138);
    WMSetFrameRelief(panel->frame, WRFlat);

    createSetDefaultLabels(panel);
    createAppList(panel);
    createDefaultLabels(panel);
    createClickLabel(panel);
    createExecField(panel);
    createButtons(panel);

    WMRealizeWidget(panel->frame);
    WMMapSubwidgets(panel->frame);

}

Panel*
InitViewer(WMWindow *win, FileInfo *fileInfo)
{
    _Panel *panel;

    panel = wmalloc(sizeof(_Panel));
    memset(panel, 0, sizeof(_Panel));

    panel->sectionName = (char *) wmalloc(strlen(_("Viewer Inspector"))+1);
    strcpy(panel->sectionName, _("Viewer Inspector"));

    panel->win = win;

    panel->callbacks.createWidgets = createPanel;
    panel->callbacks.updateDomain = storeData;
    panel->callbacks.updateDisplay = showData;

    panel->fileInfo = fileInfo;

    return panel;
}
