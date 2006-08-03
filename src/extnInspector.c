#include "FSViewer.h"
#include "FSUtils.h"
#include "FSPanel.h"

#define WIDTH               272
#define HEIGHT              272
#define LABEL_HEIGHT         16
#define LABEL_WIDTH          48

enum 
{
    DefEdit = (1 << 0),
    DefView = (1 << 1)
};

typedef struct _Panel {
    WMFrame      *frame;
    char         *sectionName;
    
    CallbackRec   callbacks;

    WMScreen	 *scr;
    FSViewer   	 *app;
    WMWindow     *win;

    WMButton     *okBtn;
    WMButton     *revertBtn;
    WMButton     *iconBtn;

    WMList       *extnList;
    WMTextField  *extnField;

    WMScrollView *sview;
    WMFrame      *sviewF;
    WMScroller   *sviewS;
    WMView       *sviewV;

    WMButton     *addBtn;
    WMButton     *removeBtn;

    FileInfo     *fileInfo;

    struct {
	unsigned int buttonPressed:1;
	unsigned int buttonWasPressed:1;
    } flags;

} _Panel;

static void
showData(_Panel *panel)
{
    int i;
    char *extn;
    WMPropList* extnList;
    WMPropList* val;
    WMPropList* dict;

    WMClearList(panel->extnList);

    extn = NULL;

    if(panel->fileInfo->fileType == ROOT)
    {
	extn = (char *)wmalloc(5);
	strcpy(extn, "ROOT");
    }
    else
	extn = wstrdup(GetFileExtn(panel->fileInfo->name));

    dict = WMGetFromPLDictionary(filesDB, WMCreatePLString(extn));
    if(dict && WMIsPLDictionary(dict))
    {
	extnList = WMGetFromPLDictionary(dict, WMCreatePLString("extn"));
	if (extnList && WMIsPLArray(extnList)) 
	{
	    char *text;
	    WMListItem *item;

	    for (i=0; i<WMGetPropListItemCount(extnList); i++) 
	    {
		val = WMGetFromPLArray(extnList, i);
		text = WMGetFromPLString(val);
		item = WMAddListItem(panel->extnList, text);
		item->clientData = GetIconStringForExtn(text);
	    }
	}
    }

    if(extn)
	free(extn);

    WMSetButtonImagePosition(panel->iconBtn, WIPNoImage);
}

WMPropList*
RemoveArrayElement(WMPropList* array, WMPropList* val)
{
    int i, numElem;
    WMPropList* tmp;

    numElem = WMGetPropListItemCount(array);
    if (array && WMIsPLArray(array)) 
    {
	for(i = 0; i < numElem; i++)
	{
	    tmp = WMGetFromPLArray(array, i);
	    if(WMIsPropListEqualTo(val, tmp))
	    {
		WMDeleteFromPLArray(array, i);
		WMReleasePropList(tmp);
		break;
	    }
	}
    }

    return array;
}

static void
storeData(_Panel *panel)
{
    int i, j, numRows, numElem;
    char *extn;
    Bool notFound = True;
    WMPropList* dictKey = NULL;
    WMPropList* val = NULL;
    WMPropList* dict = NULL;
    WMPropList* exeArray = NULL;
    WMPropList* valDict = NULL;
    WMPropList* tmp = NULL;
    WMPropList* dictElem = NULL;
    WMPropList* extnArray = NULL;
    WMPropList* extnKey = WMCreatePLString("extn");

    extn = NULL;

    numRows = WMGetListNumberOfRows(panel->extnList);

    if(panel->fileInfo->fileType == ROOT)
    {
	extn = (char *)wmalloc(5);
	strcpy(extn, "ROOT");
    }
    else
	extn = wstrdup(GetFileExtn(panel->fileInfo->name));
    dictKey = WMCreatePLString(extn);

    dict = WMGetFromPLDictionary(filesDB, dictKey);
    if(!dict)
    {
	dict = WMCreatePLDictionary(NULL, NULL);
	WMPutInPLDictionary(filesDB, dictKey, dict);
    }

    extnArray = WMGetFromPLDictionary(dict, extnKey);
    if(!extnArray)
    {
	extnArray = WMCreatePLArray(NULL, NULL);
	WMPutInPLDictionary(dict, extnKey, extnArray);
    }
    numElem = WMGetPropListItemCount(extnArray);

    for (i=0; i < numRows; i++) 
    {
	val = WMCreatePLString(WMGetListItem(panel->extnList, i)->text);
	for(j = 0; j < numElem; j++)
	{
	    dictElem = WMGetFromPLArray(extnArray, j);
	    if(WMIsPropListEqualTo(val, dictElem))
	    {
		notFound = False;
		break;
	    }
	}
	if(notFound)
	{
	    valDict = WMGetFromPLDictionary(filesDB, val);
	    if (!valDict)
	    {
		valDict = WMCreatePLDictionary(NULL, NULL);	
		WMPutInPLDictionary(filesDB, val, valDict);
	    }
	    tmp = WMCreatePLString("EXE");
	    exeArray = WMGetFromPLDictionary(valDict, tmp);
	    if (!exeArray)
	    {
		exeArray = WMCreatePLArray(NULL, NULL);	
		WMPutInPLDictionary(valDict, tmp, exeArray);
	    }	
	    InsertArrayElement(exeArray, dictKey);
	    InsertArrayElement(extnArray, val);
	}
	notFound = True;
    }

    numElem = WMGetPropListItemCount(extnArray);
    notFound = True;
    for (j=0; j < numElem; j++) 
    {
	dictElem = WMGetFromPLArray(extnArray, j);
	for(i = 0; i < numRows; i++)
	{
	    val = WMCreatePLString(WMGetListItem(panel->extnList, i)->text);
	    if(WMIsPropListEqualTo(val, dictElem))
	    {
		notFound = False;
		break;
	    }
	}
	if(notFound)
	{
	    valDict = WMGetFromPLDictionary(filesDB, dictElem);
	    if (valDict && WMIsPLDictionary(valDict))
	    {
		tmp = WMCreatePLString("EXE");
		exeArray = WMGetFromPLDictionary(valDict, tmp);
		if (exeArray && WMIsPLArray(exeArray))
		{
		    RemoveArrayElement(exeArray, dictKey);
		}	
	    }
	    RemoveArrayElement(extnArray, dictElem);
	    numElem--;
	    j--;
	}
	notFound = True;
    }

    if(numRows > 0)
	WMWritePropListToFile(filesDB,
	  wdefaultspathfordomain("FSViewer"), True);

   if(extn)
	free(extn);
}

static void
listClick(WMWidget *self, void *clientData)
{
    WMListItem *item;
    WMPixmap   *pixmap;
    WMList     *lPtr  = (WMList *)self;
    Panel      *panel = (Panel *)clientData;

    item = WMGetListSelectedItem(lPtr);

    if(item->clientData)
    {
	FSSetButtonImageFromFile(panel->iconBtn, item->clientData);
	WMSetButtonImagePosition(panel->iconBtn, WIPImageOnly);
    }
    else
	WMSetButtonImagePosition(panel->iconBtn, WIPNoImage);	
    
}

static void
buttonClick(WMWidget *self, void *data)
{
    Panel *panel = (Panel *)data;

    if ((WMButton *)self == panel->okBtn) 
	storeData(panel);
    else if ((WMButton *)self == panel->revertBtn) 
    {
/* 	WMSetButtonEnabled(panel->okBtn, False); */
	WMSetButtonEnabled(panel->revertBtn, False);
    }
    else if ((WMButton *)self == panel->addBtn) 
    {
	char *contents;
	int len;
	int i;
	Bool nonAlpha = False;

	contents = WMGetTextFieldText(panel->extnField);
	
	len = strlen(contents);

	for (i=0; i<WMGetListNumberOfRows(panel->extnList); i++) 
	{
	    if(strcmp(contents, WMGetListItem(panel->extnList, i)->text) == 0)
		nonAlpha = True;
	}

	if(contents[0] == '.' && !nonAlpha)
	    for(i = 1; i < len; i++)
	    {
		if( (contents[i] < '0' || contents[i] > '9') 
		    && (contents[i] < 'a' || contents[i] > 'z') 
		    && (contents[i] < 'A' || contents[i] > 'Z'))
		{
		    nonAlpha = True;
		    break;
		}
	    }
	else
	    nonAlpha = True;

	if(!nonAlpha && len != 0)
	{
	    WMAddListItem(panel->extnList, contents);	    
	}
	WMSetTextFieldText(panel->extnField, "");

	if(contents)
	    free(contents);
    }
    else if ((WMButton *)self == panel->removeBtn) 
    {
	int selected;

	selected = WMGetListSelectedItemRow(panel->extnList);
	WMRemoveListItem(panel->extnList, selected);
	WMSetButtonImagePosition(panel->iconBtn, WIPNoImage);
    }
    else if ((WMButton *)self == panel->iconBtn) 
    {
	char *extn = NULL;
	int selected;

	selected = WMGetListSelectedItemRow(panel->extnList);
	if(selected >= 0)
	{
	    WMListItem *item;
	    static int cnt = 0;
	    item = WMGetListSelectedItem(panel->extnList);
	    extn = wstrdup(item->text);
	
	    if(extn)
	    {
		cnt++;
		FSRunIconSelectPanel(panel->app, "", extn);
		/*
		  Using storeData here is another good reason to
		  start implementing the revert button
		*/
		cnt--;
		if(cnt == 0)
		{
		    storeData(panel);
		    showData(panel);
		}
	    }
	    if(extn)
		free(extn);
	}
    }

}

static void
createInfoLabel(Panel *panel)
{
   WMLabel *l;

//    l = WMCreateLabel(panel->frame);
//    WMResizeWidget(l, WIDTH-20, LABEL_HEIGHT*2);
//    WMMoveWidget(l, 10, 0);
//    WMSetLabelText(l, "Add: enter ext'n into the textfield, click \"Add\".\n"\
//		   "Remove: select ext'n in the list, click \"Remove\"");
//    WMSetLabelTextAlignment(l, WALeft);
//    WMSetLabelRelief(l, WRFlat);
//    WMSetLabelTextColor(l, WMDarkGrayColor(panel->scr));
}

static void
createSetDefaultLabels(Panel *panel)
{
    WMLabel *l;

    l = WMCreateLabel(panel->frame);
    WMResizeWidget(l, WIDTH-20, LABEL_HEIGHT*2);
    WMMoveWidget(l, 10, 206);
    WMSetLabelText(l, _("Click \"OK\" to update this application's "\
		   "file extension list"));
    WMSetLabelTextAlignment(l, WACenter);
    WMSetLabelRelief(l, WRFlat);
    WMSetLabelTextColor(l, WMDarkGrayColor(panel->scr));

}    

static void
endedEditingObserver(void *observerData, WMNotification *notification)
{
    Panel *panel = (Panel *)observerData;

    if((int)WMGetNotificationClientData(notification) == WMReturnTextMovement) 
    {
        WMPerformButtonClick(panel->addBtn);
    }
}

static void
createExtnField(Panel *panel)
{

    panel->extnField = WMCreateTextField(panel->frame);
    WMResizeWidget(panel->extnField, 180, 20);
    WMMoveWidget(panel->extnField, 8, 148);
    WMAddNotificationObserver(endedEditingObserver, panel,
                              WMTextDidEndEditingNotification, 
			      panel->extnField);
}

static void
createButtons(Panel *panel)
{

    panel->revertBtn = WMCreateCommandButton(panel->frame);
    WMMoveWidget(panel->revertBtn, 16, HEIGHT-24);
    WMResizeWidget(panel->revertBtn, 115, 24);
    WMSetButtonText(panel->revertBtn, _("Revert"));
    WMSetButtonEnabled(panel->revertBtn, True);
    WMSetButtonAction(panel->revertBtn, buttonClick, panel);
   
    panel->okBtn = WMCreateCommandButton(panel->frame);
    WMMoveWidget(panel->okBtn, 140, HEIGHT-24);
    WMResizeWidget(panel->okBtn, 115, 24);
    WMSetButtonText(panel->okBtn, _("OK"));
    WMSetButtonImage(panel->okBtn, 
		     WMGetSystemPixmap(panel->scr,WSIReturnArrow));
    WMSetButtonAltImage(panel->okBtn, 
			WMGetSystemPixmap(panel->scr, 
					  WSIHighlightedReturnArrow));
    WMSetButtonImagePosition(panel->okBtn, WIPRight);
    WMSetButtonEnabled(panel->okBtn, True);
    WMSetButtonAction(panel->okBtn, buttonClick, panel);   

    panel->addBtn = WMCreateCommandButton(panel->frame);
    WMMoveWidget(panel->addBtn, 104, 174);
    WMResizeWidget(panel->addBtn, 80, 22);
    WMSetButtonText(panel->addBtn, _("Add"));
    WMSetButtonEnabled(panel->addBtn, True);
    WMSetButtonAction(panel->addBtn, buttonClick, panel);

    panel->removeBtn = WMCreateCommandButton(panel->frame);
    WMMoveWidget(panel->removeBtn, 12, 174);
    WMResizeWidget(panel->removeBtn, 80, 22);
    WMSetButtonText(panel->removeBtn, _("Remove"));
    WMSetButtonEnabled(panel->removeBtn, True);
    WMSetButtonAction(panel->removeBtn, buttonClick, panel);

    panel->iconBtn = WMCreateCommandButton(panel->frame);
    WMMoveWidget(panel->iconBtn, 196, 36);
    WMResizeWidget(panel->iconBtn, 68, 68);
    WMSetButtonText(panel->iconBtn, _("Click To Set Icon"));
    WMSetButtonAction(panel->iconBtn, buttonClick, panel);
    WMSetButtonImagePosition(panel->iconBtn, WIPNoImage);
    
}

static void
createSView(Panel *panel)
{
    /* creates a scrollable view inside the top-level window */
    panel->sview = WMCreateScrollView(panel->frame);
    WMResizeWidget(panel->sview, 180, 140);
    WMMoveWidget(panel->sview, 8, 0);
    WMSetScrollViewRelief(panel->sview, WRSunken);
    WMSetScrollViewHasVerticalScroller(panel->sview, True);
    WMSetScrollViewHasHorizontalScroller(panel->sview, False);

    /* create a frame with a bunch of labels */
    panel->sviewF = WMCreateFrame(panel->frame);
    WMResizeWidget(panel->sviewF, 165, 200);
    WMSetFrameRelief(panel->sviewF, WRFlat);

    panel->sviewV = WMWidgetView(panel->sviewF);
    WMSetScrollViewContentView(panel->sview, panel->sviewV);

    panel->sviewS = WMGetScrollViewVerticalScroller(panel->sview);
/*     WMSetScrollerAction(panel->sviewS, scrollCallback, panel); */
    WMSetScrollViewLineScroll(panel->sview, LABEL_HEIGHT);

}

static void
createExtnList(Panel *panel)
{
    panel->extnList = WMCreateList(panel->frame);

    WMMoveWidget(panel->extnList, 8, 0);
    WMResizeWidget(panel->extnList, 180, 140);
    WMSetListAction(panel->extnList, listClick, panel);
}

static void
createPanel(Panel *p)
{
    _Panel *panel = (_Panel*)p;
    panel->frame = WMCreateFrame(panel->win);

    WMResizeWidget(panel->frame, WIDTH, HEIGHT);
    WMMoveWidget(panel->frame, 0, 138);
    WMSetFrameRelief(panel->frame, WRFlat);

    createExtnList(panel);
/*     createSView(panel); */
    createExtnField(panel);
    createInfoLabel(panel);
    createSetDefaultLabels(panel);
    createButtons(panel);

    WMRealizeWidget(panel->frame);
    WMMapSubwidgets(panel->frame);

}

Panel*
InitExtn(WMWindow *win, FSViewer *app, FileInfo *fileInfo)
{
    _Panel *panel;

    panel = wmalloc(sizeof(_Panel));
    memset(panel, 0, sizeof(_Panel));

    panel->sectionName = (char*)wmalloc(
      strlen(_("File Extensions Inspector"))+1);
    strcpy(panel->sectionName, _("File Extensions Inspector"));

    panel->win = win;
    panel->app = app;
    panel->scr = WMWidgetScreen(win);

    panel->callbacks.createWidgets = createPanel;
    panel->callbacks.updateDomain = storeData;
    panel->callbacks.updateDisplay = showData;

    panel->fileInfo = fileInfo;
    return panel;
}
