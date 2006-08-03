#include <stdarg.h>
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

    WMLabel      *nameLabel;
    WMLabel      *pathLabel;

    WMButton     *okBtn;
    WMButton     *revertBtn;

    WMTextField  *cmdField;

    FileInfo     *fileInfo;

} _Panel;

static void
showData(_Panel *panel)
{
    char *exec = NULL;

    WMSetLabelText(panel->nameLabel, panel->fileInfo->name);
    WMSetLabelText(panel->pathLabel, panel->fileInfo->path);

    exec = FSGetStringForNameKey(panel->fileInfo->name, "exec");
    if(exec)
	WMSetTextFieldText(panel->cmdField, exec);
    else
	WMSetTextFieldText(panel->cmdField, "");
   
    if(exec)
	free(exec);
}

char *
getCmdFieldText(_Panel *panel)
{
    char *txt;
    char *buf;

    txt = WMGetTextFieldText(panel->cmdField);
    buf = FSParseCmdField(panel->fileInfo, txt);

    return buf;
}

static void
storeData(_Panel *panel)
{
    char *txt = NULL;
    WMPropList* dictKey = NULL;
    WMPropList* array   = NULL;

    dictKey = WMCreatePLString(panel->fileInfo->extn);
    array = FSGetUDObjectForKey(defaultsDB, "EXE");
    if(!array || !WMIsPLArray(array))
       return;

    txt = getCmdFieldText(panel);
    if( txt && strcmp("", txt) )
    {
	WMSetTextFieldText(panel->cmdField, txt);
	FSSetStringForNameKey(panel->fileInfo->extn, "exec", txt);
	InsertArrayElement(array, dictKey);
    }
    else
    {
	FSSetStringForNameKey(panel->fileInfo->extn, "exec", "");
	FSRemoveArrayElement(array, dictKey);
    }

    WMSetUDObjectForKey(defaultsDB, array, "EXE");

    if(txt)
	free(txt);
}

static void
createInfoLabel(Panel *panel)
{
    WMLabel *l;

    l = WMCreateLabel(panel->frame);
    WMSetLabelWraps(l, 1);
    WMResizeWidget(l, WIDTH-8, LABEL_HEIGHT*2);
    WMMoveWidget(l, 4, 0);
    WMSetLabelText(l, _("Enter command plus arguments into the "\
                        "textfield below:"));
    WMSetLabelTextAlignment(l, WALeft);
    WMSetLabelRelief(l, WRFlat);
    WMSetLabelTextColor(l, WMDarkGrayColor(WMWidgetScreen(panel->win)));

}    

static void
createShortcutLabel(Panel *panel)
{
    WMLabel *l;

    l = WMCreateLabel(panel->frame);
    WMResizeWidget(l, LABEL_WIDTH*2, LABEL_HEIGHT);
    WMMoveWidget(l, 4, 79);
    WMSetLabelText(l, _("Shortcuts:"));
    WMSetLabelRelief(l, WRFlat);
    WMSetLabelTextColor(l, WMDarkGrayColor(WMWidgetScreen(panel->win)));
}    

static void
createNameLabel(Panel *panel)
{
    WMLabel *l;

    l = WMCreateLabel(panel->frame);
    WMResizeWidget(l, LABEL_WIDTH, LABEL_HEIGHT);
    WMMoveWidget(l, 10, 110);
    WMSetLabelText(l, "\%f:");
    WMSetLabelTextAlignment(l, WARight);
    WMSetLabelRelief(l, WRFlat);

    panel->nameLabel = WMCreateLabel(panel->frame);
    WMResizeWidget(panel->nameLabel, WIDTH-LABEL_WIDTH-20, LABEL_HEIGHT);
    WMMoveWidget(panel->nameLabel, 10+LABEL_WIDTH, 110);
    WMSetLabelRelief(panel->nameLabel, WRFlat);

}    

static void
createPathLabel(Panel *panel)
{
    WMLabel *l;

    l = WMCreateLabel(panel->frame);
    WMResizeWidget(l, LABEL_WIDTH, LABEL_HEIGHT*2);
    WMMoveWidget(l, 10, 136);
    WMSetLabelText(l, "\%p:");
    WMSetLabelTextAlignment(l, WARight);
    WMSetLabelRelief(l, WRFlat);

    panel->pathLabel = WMCreateLabel(panel->frame);
    WMResizeWidget(panel->pathLabel, WIDTH-LABEL_WIDTH-20, LABEL_HEIGHT*2);
    WMMoveWidget(panel->pathLabel, 10+LABEL_WIDTH, 136);
    WMSetLabelRelief(panel->pathLabel, WRFlat);

}    

static void
createFileStringLabel(Panel *panel)
{
    WMLabel *l;

    l = WMCreateLabel(panel->frame);
    WMResizeWidget(l, LABEL_WIDTH, LABEL_HEIGHT);
    WMMoveWidget(l, 10, 178);
    WMSetLabelText(l, "\%s:");
    WMSetLabelTextAlignment(l, WARight);
    WMSetLabelRelief(l, WRFlat);

    l = WMCreateLabel(panel->frame);
    WMResizeWidget(l, WIDTH-LABEL_WIDTH-20, LABEL_HEIGHT);
    WMMoveWidget(l, 10+LABEL_WIDTH, 178);
    WMSetLabelText(l, _("associated file to open"));
    WMSetLabelRelief(l, WRFlat);

}    

static void
createCmdField(Panel *panel)
{

    panel->cmdField = WMCreateTextField(panel->frame);
    WMResizeWidget(panel->cmdField, 256, 20);
    WMMoveWidget(panel->cmdField, 8, 43);

}
static void
buttonClick(WMWidget *self, void *data)
{
    Panel *panel = (Panel *)data;

    if ((WMButton *)self == panel->okBtn) {
	storeData(panel);
	WMSetButtonEnabled(panel->revertBtn, True);
    }
    else {
      WMRunAlertPanel(WMWidgetScreen(panel->win), NULL,
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
		     WMGetSystemPixmap(WMWidgetScreen(panel->win), 
				       WSIReturnArrow));
    WMSetButtonAltImage(panel->okBtn, 
			WMGetSystemPixmap(WMWidgetScreen(panel->win), 
					  WSIHighlightedReturnArrow));
    WMSetButtonImagePosition(panel->okBtn, WIPRight);
    WMSetButtonEnabled(panel->okBtn, True);
    WMSetButtonAction(panel->okBtn, buttonClick, panel);   
}

static void
createPanel(Panel *p)
{
    _Panel *panel = (_Panel*)p;
    panel->frame = WMCreateFrame(panel->win);

    WMResizeWidget(panel->frame, WIDTH, HEIGHT);
    WMMoveWidget(panel->frame, 0, 138);
    WMSetFrameRelief(panel->frame, WRFlat);

    createInfoLabel(panel);
    createCmdField(panel);
    createShortcutLabel(panel);
    createNameLabel(panel);
    createPathLabel(panel);
    createFileStringLabel(panel);
    createButtons(panel);

    WMRealizeWidget(panel->frame);
    WMMapSubwidgets(panel->frame);

}


Panel*
InitExecutable(WMWindow *win, FileInfo *fileInfo)
{
    _Panel *panel;

    panel = wmalloc(sizeof(_Panel));
    memset(panel, 0, sizeof(_Panel));

    panel->sectionName = (char *) wmalloc(strlen(_("Executable Inspector"))+1);
    strcpy(panel->sectionName, _("Executable Inspector"));

    panel->win = win;

    panel->callbacks.createWidgets = createPanel;
    panel->callbacks.updateDomain = storeData;
    panel->callbacks.updateDisplay = showData;

    panel->fileInfo = fileInfo;

    return panel;
}
