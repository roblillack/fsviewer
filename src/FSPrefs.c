#include <X11/Xatom.h>
#include <X11/keysym.h>
#include <WINGs/WINGsP.h>

#include "FSViewer.h"
#include "FSUtils.h"
#include "FSPanel.h"
#include "FSPrefs.h"

#define  WIN_WIDTH  572
#define  WIN_HEIGHT 322
#define  PADY       5
#define  PADX       5
#define  TABVIEW_HEIGHT WIN_HEIGHT-50
#define  TABVIEW_WIDTH  WIN_WIDTH-PADX*2
#define  VARS_TAB   0
#define  APPS_TAB   1
#define  DISCS_TAB  2
#define  NUM_TABS   3


typedef struct _DISK
{
/*     FSViewer *app; */
    /* Nickname for mount point */
    char     *name;
    /* Mount point */
    char     *point;
    /* Device name */
    char     *device;
    /* Mount Cmd */
    char     *mnt;
    /* UMount cmd */
    char     *umnt;
    /* Eject Cmd */
    char     *eject;
    /* Close Cmd */
    char     *close;

} _DISK;

typedef struct _DISK Disc;

static	FSPreferencesPanel     *FSCreatePreferencesPanel(FSViewer*, char*);
static	void  FSClosePreferencesPanel(WMWidget*, void*);
static	void  FSDestroyPreferencesPanel(FSPreferencesPanel*);
static  void  createVarsTab(FSPreferencesPanel *prefs);
static  void  createDiskTab(FSPreferencesPanel *prefs);
static  void  createTypesTab(FSPreferencesPanel *prefs);
static  void  genericListClick(WMWidget *self, void *data);
static  void  genericButtonActionHandler(WMWidget *self, void *clientData);
static  void  setVarsImage(WMWidget *self, void *data);
static  void  populateVarsTab(FSPreferencesPanel *prefs);
static  void  populateTypesTab(FSPreferencesPanel *prefs);
static  void  populateDisksTab(FSPreferencesPanel *prefs);
static  void  populateDisksFields(FSPreferencesPanel *prefs);
static  void  populateAppsFields(FSPreferencesPanel *prefs);
static  void  populateTypesFields(FSPreferencesPanel *prefs);
static  void  applyBtnCB(WMWidget *self, void *client);
static  void  tfDidChange(struct WMTextFieldDelegate *self,
			  WMNotification *notif);
static  void  setTFDelegate(FSPreferencesPanel *prefs, WMTextField *tf);
static  void  setTVIDelegate(FSPreferencesPanel *prefs, WMTabView *tv);
static  void  tvDidSelectItem(struct WMTabViewDelegate *self, 
			      WMTabView *tabView, WMTabViewItem *item);
static  void  saveTab(FSPreferencesPanel *prefs, int tabID);
static  void  saveVarsTab(FSPreferencesPanel *prefs);
static  void  saveAppsTab(FSPreferencesPanel *prefs);
static  void  saveDiscsTab(FSPreferencesPanel *prefs);
static  void  saveAllTabsAndClose(WMWidget *self, void *client);
static  void  genericListDrawProc(WMList *lPtr, int index, Drawable d, 
				  char *text, int state, WMRect *rect);



static	FSPreferencesPanel     *preferences = NULL;

void FSRunPreferencesPanel(FSViewer *app, char *title)
{ 
    if (preferences)
	return;

    if (!(preferences = FSCreatePreferencesPanel(app, title))) { return; }
    
    WMMapWidget(preferences->win);
    
    while (!(preferences->flags.done))
    {
	XEvent event;
	
	WMNextEvent(preferences->dpy, &event);
	WMHandleEvent(&event);
    }
    FSDestroyPreferencesPanel(preferences);
    preferences = NULL;
}

static FSPreferencesPanel *FSCreatePreferencesPanel(FSViewer *app, char *title)
{
    FSPreferencesPanel	  *prefs;
    int		           height, width, offset;
    WMPixmap              *appicon;
    WMFrame               *f;
    WMLabel               *l;
    WMTabViewItem         *tab;

    /* allocate memory for the prefernces panel */
    if (!( prefs= (FSPreferencesPanel *)malloc(sizeof(FSPreferencesPanel))))
    { 
	return NULL; 
    }
    memset((void *) prefs, 0, sizeof(FSPreferencesPanel));

    /* Take in some settings */
    prefs->app = app;
    prefs->scr = FSGetFSViewerScreen(app);
    prefs->dpy = WMScreenDisplay(prefs->scr);

    
    height = WIN_HEIGHT;
    width  = WIN_WIDTH;

    prefs->win = WMCreateWindow(prefs->scr, "prefs");
    WMResizeWidget(prefs->win, width, height);
    WMSetWindowTitle(prefs->win, title);
    WMSetWindowCloseAction(prefs->win, FSClosePreferencesPanel, 
			   (void *) prefs);

    prefs->frame = WMCreateFrame(prefs->win);
    WMResizeWidget(prefs->frame, width, height);
    WMMoveWidget(prefs->frame, 0, 0);
    WMSetFrameRelief(prefs->frame, WRFlat);

    prefs->tabV = WMCreateTabView(prefs->frame);
    WMMoveWidget(prefs->tabV, 5, 5);
    WMResizeWidget(prefs->tabV, TABVIEW_WIDTH, TABVIEW_HEIGHT);
    setTVIDelegate(prefs, prefs->tabV);

    /* variables Tab */
    createVarsTab(prefs);
    populateVarsTab(prefs);

    /* App/File Types Tab*/
    createTypesTab(prefs);
    populateTypesTab(prefs);

    /* Disks Tab*/
    createDiskTab(prefs);
    populateDisksTab(prefs);

    /* Ok/Apply/Cancel Buttons */
    prefs->cancelBtn = WMCreateCommandButton(prefs->frame);
    WMSetButtonText(prefs->cancelBtn, _("Cancel"));
    WMResizeWidget(prefs->cancelBtn, 100, 24);
    WMMoveWidget(prefs->cancelBtn, 
		 WIN_WIDTH-330, (WIN_HEIGHT+PADY+TABVIEW_HEIGHT-24)/2);
    WMSetButtonAction(prefs->cancelBtn, FSClosePreferencesPanel, prefs);
    WMMapWidget(prefs->cancelBtn);

    prefs->applyBtn = WMCreateCommandButton(prefs->frame);
    WMSetButtonText(prefs->applyBtn, _("Apply"));
    WMResizeWidget(prefs->applyBtn, 100, 24);
    WMMoveWidget(prefs->applyBtn, 
		 WIN_WIDTH-215, (WIN_HEIGHT+PADY+TABVIEW_HEIGHT-24)/2);
    WMSetButtonAction(prefs->applyBtn, applyBtnCB, prefs);
    WMSetButtonEnabled(prefs->applyBtn, False);
    WMMapWidget(prefs->applyBtn);

    prefs->okBtn = WMCreateCommandButton(prefs->frame);
    WMSetButtonText(prefs->okBtn, _("OK"));
    WMResizeWidget(prefs->okBtn, 100, 24);
    WMMoveWidget(prefs->okBtn, 
		 WIN_WIDTH-105, (WIN_HEIGHT+PADY+TABVIEW_HEIGHT-24)/2);
    WMSetButtonAction(prefs->okBtn, saveAllTabsAndClose, prefs);
    WMSetButtonEnabled(prefs->okBtn, False);
    WMMapWidget(prefs->okBtn);
    
    prefs->flags.done = 0;
    prefs->flags.evars = False;
    prefs->flags.eapps = False;
    prefs->flags.ediscs = False;
    
    WMRealizeWidget(prefs->win);
    
    FSAddWindow(prefs->app, WMWidgetXID(prefs->win));
    WMMapSubwidgets(prefs->frame);
    WMMapSubwidgets(prefs->win);

    return prefs;
}

static void
createDiskTab(FSPreferencesPanel *prefs)
{
    WMFrame       *f;
    WMFrame       *f2;
    WMLabel       *l;
    WMButton      *b;
    WMList        *list;
    WMTextField   *tf;
    WMTabViewItem *tab;
    int frame_width  = 255;
    int frame_height = 220;
    int padx = (TABVIEW_WIDTH-PADX*8-frame_width*2);
    WMTextFieldDelegate *delegate = NULL;
    f = WMCreateFrame(prefs->frame);
    WMSetFrameRelief(f, WRFlat);

    /* Start Disks */
    f2 = WMCreateFrame(f);
    WMSetFrameRelief(f2, WRGroove);
    WMSetFrameTitle(f2, _(" Disks "));
    WMMoveWidget(f2, PADX*2, 10);
    WMResizeWidget(f2, TABVIEW_WIDTH-23, frame_height);
    WMMapWidget(f2);

    list = WMCreateList(f2);
    WMMoveWidget(list, 10, 30);
    WMResizeWidget(list, frame_width-20, 100);
    WMSetListAction(list, genericListClick, prefs);
    WMMapWidget(list);
    prefs->disksList = list;

    l = WMCreateLabel(f2);
    WMResizeWidget(l, 45, 18);
    WMMoveWidget(l, 10, 140);
    WMSetLabelText(l, _("Name:"));
    WMSetLabelTextColor(l, WMDarkGrayColor(prefs->scr));
    WMMapWidget(l);

    tf = WMCreateTextField(f2);
    WMResizeWidget(tf, 110, 18);
    WMMoveWidget(tf, 50, 140);
    WMSetTextFieldText(tf, _("Menu Name"));
    setTFDelegate(prefs, tf);
    WMMapWidget(tf);
    prefs->disksNameTF = tf;


    l = WMCreateLabel(f2);
    WMResizeWidget(l, 45, 18);
    WMMoveWidget(l, 10, 166);
    WMSetLabelText(l, _("Mnt:"));
    WMSetLabelTextColor(l, WMDarkGrayColor(prefs->scr));
    WMMapWidget(l);

    tf = WMCreateTextField(f2);
    WMResizeWidget(tf, 110, 18);
    WMMoveWidget(tf, 50, 166);
    WMSetTextFieldText(tf, _("Mount Point"));
/*     WMSetTextFieldDelegate(tPtr, delegate); */
    setTFDelegate(prefs, tf);
    WMMapWidget(tf);
    prefs->disksMntTF = tf;


    l = WMCreateLabel(f2);
    WMResizeWidget(l, 45, 18);
    WMMoveWidget(l, 10, 192);
    WMSetLabelText(l, _("Dev:"));
    WMSetLabelTextColor(l, WMDarkGrayColor(prefs->scr));
    WMMapWidget(l);

    tf = WMCreateTextField(f2);
    WMResizeWidget(tf, 110, 18);
    WMMoveWidget(tf, 50, 192);
    WMSetTextFieldText(tf, _("Device"));
    setTFDelegate(prefs, tf);
/*     WMSetTextFieldDelegate(tPtr, delegate); */
    WMMapWidget(tf);
    prefs->disksDevTF = tf;


    b = WMCreateCommandButton(f2);
    WMResizeWidget(b, 68, 18);
    WMMoveWidget(b, 175, 140);
    WMSetButtonText(b, _("New"));
    WMSetButtonAction(b, genericButtonActionHandler, prefs);
    WMMapWidget(b);
    prefs->disksNewBtn = b;


    b = WMCreateCommandButton(f2);
    WMResizeWidget(b, 68, 18);
    WMMoveWidget(b, 175, 166);
    WMSetButtonText(b, _("Update"));
    WMSetButtonAction(b, genericButtonActionHandler, prefs);
    WMMapWidget(b);
    prefs->disksUpdateBtn = b;


    b = WMCreateCommandButton(f2);
    WMResizeWidget(b, 68, 18);
    WMMoveWidget(b, 175, 192);
    WMSetButtonText(b, _("Remove"));
    WMSetButtonAction(b, genericButtonActionHandler, prefs);
    WMMapWidget(b);
    prefs->disksRemoveBtn = b;
    /* End Disks */


    /* Start Commands */
/*     f2 = WMCreateFrame(f); */
/*     WMSetFrameRelief(f2, WRGroove); */
/*     WMSetFrameTitle(f2, " Commands "); */
/*     WMMoveWidget(f2, frame_width+PADX*2+padx, 10); */
/*     WMResizeWidget(f2, frame_width, frame_height); */
/*     WMMapWidget(f2); */

    l = WMCreateLabel(f2);
    WMResizeWidget(l, frame_width-20, 18);
    WMMoveWidget(l, frame_width+PADX*2+padx+10, 30);
    WMSetLabelText(l, _("Mount:"));
    WMSetLabelTextColor(l, WMDarkGrayColor(prefs->scr));
    WMMapWidget(l);

    tf = WMCreateTextField(f2);
    WMResizeWidget(tf, frame_width-20, 18);
    WMMoveWidget(tf, frame_width+PADX*2+padx+10, 48);
    WMSetTextFieldText(tf, _("App"));
/*     WMSetTextFieldDelegate(tPtr, delegate); */
    setTFDelegate(prefs, tf);
    WMMapWidget(tf);
    prefs->cmdMntTF = tf;


    l = WMCreateLabel(f2);
    WMResizeWidget(l, frame_width-20, 18);
    WMMoveWidget(l, frame_width+PADX*2+padx+10, 76);
    WMSetLabelText(l, _("Unmount:"));
    WMSetLabelTextColor(l, WMDarkGrayColor(prefs->scr));
    WMMapWidget(l);

    tf = WMCreateTextField(f2);
    WMResizeWidget(tf, frame_width-20, 18);
    WMMoveWidget(tf, frame_width+PADX*2+padx+10, 94);
    WMSetTextFieldText(tf, _("App"));
/*     WMSetTextFieldDelegate(tPtr, delegate); */
    setTFDelegate(prefs, tf);
    WMMapWidget(tf);
    prefs->cmdUMntTF = tf;


    l = WMCreateLabel(f2);
    WMResizeWidget(l, frame_width-20, 18);
    WMMoveWidget(l, frame_width+PADX*2+padx+10, 122);
    WMSetLabelText(l, _("Eject:"));
    WMSetLabelTextColor(l, WMDarkGrayColor(prefs->scr));
    WMMapWidget(l);

    tf = WMCreateTextField(f2);
    WMResizeWidget(tf, frame_width-20, 18);
    WMMoveWidget(tf, frame_width+PADX*2+padx+10, 140);
    WMSetTextFieldText(tf, _("App"));
/*     WMSetTextFieldDelegate(tPtr, delegate); */
    setTFDelegate(prefs, tf);
    WMMapWidget(tf);
    prefs->cmdEjectTF = tf;


    l = WMCreateLabel(f2);
    WMResizeWidget(l, frame_width-20, 18);
    WMMoveWidget(l, frame_width+PADX*2+padx+10, 168);
    WMSetLabelText(l, _("Close:"));
    WMSetLabelTextColor(l, WMDarkGrayColor(prefs->scr));
    WMMapWidget(l);

    tf = WMCreateTextField(f2);
    WMResizeWidget(tf, frame_width-20, 18);
    WMMoveWidget(tf, frame_width+PADX*2+padx+10, 186);
    WMSetTextFieldText(tf, _("close tray"));
/*     WMSetTextFieldDelegate(tPtr, delegate); */
    setTFDelegate(prefs, tf);
    WMMapWidget(tf);
    prefs->cmdCloseTF = tf;
    /* End Commands */

    tab = WMCreateTabViewItemWithIdentifier(DISCS_TAB);
    WMSetTabViewItemView(tab, WMWidgetView(f));
    WMAddItemInTabView(prefs->tabV, tab);
    WMSetTabViewItemLabel(tab, _("Disks"));
}

static void
createTypesTab(FSPreferencesPanel *prefs)
{
    WMFrame       *f;
    WMFrame       *f2;
    WMLabel       *l;
    WMButton      *b;
    WMList        *list;
    WMTextField   *tf;
    WMTabViewItem *tab;
    int frame_width  = 255;
    int frame_height = 220;
    int padx = (TABVIEW_WIDTH-PADX*4-frame_width*2);

    f = WMCreateFrame(prefs->frame);
    WMSetFrameRelief(f, WRFlat);

    /* Start Apps */
    f2 = WMCreateFrame(f);
    WMSetFrameRelief(f2, WRGroove);
    WMSetFrameTitle(f2, _(" Applications "));
    WMMoveWidget(f2, PADX*2, 10);
    WMResizeWidget(f2, frame_width, frame_height);
    WMMapWidget(f2);

    list = WMCreateList(f2);
    WMMoveWidget(list, 10, 30);
    WMResizeWidget(list, frame_width-100, 100);
    WMSetListAction(list, genericListClick, prefs);
    WMSetListUserDrawProc(list, genericListDrawProc);
    WMMapWidget(list);
    prefs->appsList = list;

    b = WMCreateCommandButton(f2);
    WMMoveWidget(b, 175, 46);
    WMResizeWidget(b, 68, 68);
    WMSetButtonEnabled(b, True);
    WMSetButtonAction(b, genericButtonActionHandler, prefs);
    WMSetButtonImagePosition(b, WIPImageOnly);
    WMMapWidget(b);
    prefs->appsIconBtn = b;


    l = WMCreateLabel(f2);
    WMResizeWidget(l, 40, 18);
    WMMoveWidget(l, 10, 140);
    WMSetLabelText(l, _("Exec:"));
    WMSetLabelTextColor(l, WMDarkGrayColor(prefs->scr));
    WMMapWidget(l);

    tf = WMCreateTextField(f2);
    WMResizeWidget(tf, 115, 18);
    WMMoveWidget(tf, 50, 140);
    WMSetTextFieldText(tf, _("App"));
/*     WMSetTextFieldDelegate(tPtr, delegate); */
    setTFDelegate(prefs, tf);
    WMMapWidget(tf);
    prefs->appsExecTF = tf;


    b = WMCreateCommandButton(f2);
    WMResizeWidget(b, 68, 18);
    WMMoveWidget(b, 175, 140);
    WMSetButtonText(b, _("Browse..."));
    WMSetButtonAction(b, genericButtonActionHandler, prefs);
    WMMapWidget(b);
    prefs->appsBrowseBtn = b;


    b = WMCreateCommandButton(f2);
    WMResizeWidget(b, 73, 18);
    WMMoveWidget(b, 10, 166);
    WMSetButtonText(b, _("Set"));
    WMSetButtonAction(b, genericButtonActionHandler, prefs);
    WMMapWidget(b);
    prefs->appsSetBtn = b;

    b = WMCreateCommandButton(f2);
    WMResizeWidget(b, 73, 18);
    WMMoveWidget(b, 93, 166);
    WMSetButtonText(b, _("Delete"));
    WMSetButtonAction(b, genericButtonActionHandler, prefs);
    WMMapWidget(b);
    prefs->appsRemoveBtn = b;
    /* End Apps */

    /* Start File Types */
    f2 = WMCreateFrame(f);
    WMSetFrameRelief(f2, WRGroove);
    WMSetFrameTitle(f2, _(" File Types "));
    WMMoveWidget(f2, frame_width+PADX*2+padx, 10);
    WMResizeWidget(f2, frame_width, frame_height);
    WMMapWidget(f2);

    list = WMCreateList(f2);
    WMMoveWidget(list, 10, 30);
    WMResizeWidget(list, frame_width-100, 100);
    WMSetListAction(list, genericListClick, prefs);
    WMSetListUserDrawProc(list, genericListDrawProc);
    WMMapWidget(list);
    prefs->typesList = list;


    b = WMCreateCommandButton(f2);
    WMMoveWidget(b, 175, 46);
    WMResizeWidget(b, 68, 68);
    WMSetButtonEnabled(b, True);
    WMSetButtonAction(b, genericButtonActionHandler, prefs);
    WMSetButtonImagePosition(b, WIPImageOnly);
    WMMapWidget(b);
    prefs->typesIconBtn = b;


    l = WMCreateLabel(f2);
    WMResizeWidget(l, 40, 18);
    WMMoveWidget(l, 10, 140);
    WMSetLabelText(l, _("View:"));
    WMSetLabelTextColor(l, WMDarkGrayColor(prefs->scr));
    WMMapWidget(l);

    tf = WMCreateTextField(f2);
    WMResizeWidget(tf, 115, 18);
    WMMoveWidget(tf, 50, 140);
    WMSetTextFieldText(tf, _("App"));
/*     WMSetTextFieldDelegate(tPtr, delegate); */
    setTFDelegate(prefs, tf);
    WMMapWidget(tf);
    prefs->typesViewTF = tf;


    b = WMCreateCommandButton(f2);
    WMResizeWidget(b, 68, 18);
    WMMoveWidget(b, 175, 140);
    WMSetButtonText(b, _("Browse..."));
    WMSetButtonAction(b, genericButtonActionHandler, prefs);
    WMMapWidget(b);
    /* Could use WMHangData instead!!! */
    prefs->typesViewBrowseBtn = b;

    l = WMCreateLabel(f2);
    WMResizeWidget(l, 40, 18);
    WMMoveWidget(l, 10, 166);
    WMSetLabelText(l, _("Edit:"));
    WMSetLabelTextColor(l, WMDarkGrayColor(prefs->scr));
    WMMapWidget(l);

    tf = WMCreateTextField(f2);
    WMResizeWidget(tf, 115, 18);
    WMMoveWidget(tf, 50, 166);
    WMSetTextFieldText(tf, _("App"));
/*     WMSetTextFieldDelegate(tPtr, delegate); */
    setTFDelegate(prefs, tf);
    WMMapWidget(tf);
    prefs->typesEditTF = tf;


    b = WMCreateCommandButton(f2);
    WMResizeWidget(b, 68, 18);
    WMMoveWidget(b, 175, 166);
    WMSetButtonText(b, _("Browse..."));
    WMSetButtonAction(b, genericButtonActionHandler, prefs);
    WMMapWidget(b);
    prefs->typesEditBrowseBtn = b;


    b = WMCreateCommandButton(f2);
    WMResizeWidget(b, 73, 18);
    WMMoveWidget(b, 10, 192);
    WMSetButtonText(b, _("Set"));
    WMSetButtonAction(b, genericButtonActionHandler, prefs);
    WMMapWidget(b);
    prefs->typesSetBtn = b;

    b = WMCreateCommandButton(f2);
    WMResizeWidget(b, 73, 18);
    WMMoveWidget(b, 93, 192);
    WMSetButtonText(b, _("Delete"));
    WMSetButtonAction(b, genericButtonActionHandler, prefs);
    WMMapWidget(b);
    prefs->typesRemoveBtn = b;
    /* End Executables */


    tab = WMCreateTabViewItemWithIdentifier(APPS_TAB);
    WMSetTabViewItemView(tab, WMWidgetView(f));
    WMAddItemInTabView(prefs->tabV, tab);
    WMSetTabViewItemLabel(tab, _("App/File Types"));

}

static void
createVarsTab(FSPreferencesPanel *prefs)
{
    WMFrame       *f;
    WMFrame       *f2;
    WMLabel       *l;
    WMButton      *b;
    WMList        *list;
    WMTextField   *tf;
    WMTabViewItem *tab;
    int frame_width  = 155;
    int frame_height = 220;
    int padx = (TABVIEW_WIDTH-PADX*4-frame_width*3)/2;

    f = WMCreateFrame(prefs->frame);
    WMSetFrameRelief(f, WRFlat);

    /* Start Images */
    f2 = WMCreateFrame(f);
    WMSetFrameRelief(f2, WRGroove);
    WMSetFrameTitle(f2, _(" Images "));
    WMMoveWidget(f2, PADX*2, 10);
    WMResizeWidget(f2, frame_width, frame_height);
    WMMapWidget(f2);

    list = WMCreateList(f2);
    WMMoveWidget(list, 10, 30);
    WMResizeWidget(list, frame_width-20, 100);
    WMSetListAction(list, genericListClick, prefs);
    WMMapWidget(list);
    prefs->imgList = list;


    b = WMCreateCommandButton(f2);
    WMMoveWidget(b, (frame_width-68)/2, 140);
    WMResizeWidget(b, 68, 68);
    WMSetButtonEnabled(b, True);
    WMSetButtonAction(b, genericButtonActionHandler, prefs);
    WMSetButtonImagePosition(b, WIPImageOnly);
    WMMapWidget(b);
    prefs->imgIconBtn = b;
    /* End Images */

    /* Start Executables */
    f2 = WMCreateFrame(f);
    WMSetFrameRelief(f2, WRGroove);
    WMSetFrameTitle(f2, _(" Executables "));
    WMMoveWidget(f2, frame_width+PADX*2+padx, 10);
    WMResizeWidget(f2, frame_width, frame_height);
    WMMapWidget(f2);

    list = WMCreateList(f2);
    WMMoveWidget(list, 10, 30);
    WMResizeWidget(list, frame_width-20, 100);
    WMSetListAction(list, genericListClick, prefs);
    WMMapWidget(list);
    prefs->execList = list;


    l = WMCreateLabel(f2);
    WMMoveWidget(l, 10, 140);
    WMResizeWidget(l, frame_width-20, 18);
    WMSetLabelText(l, _("Exec Str:"));
    WMSetLabelTextColor(l, WMDarkGrayColor(prefs->scr));
    WMMapWidget(l);

    tf = WMCreateTextField(f2);
    WMMoveWidget(tf, 10, 160);
    WMResizeWidget(tf, frame_width-20, 18);
    WMSetTextFieldText(tf, _("Exes:"));
/*     WMSetTextFieldDelegate(tPtr, delegate); */
    setTFDelegate(prefs, tf);
    WMMapWidget(tf);
    prefs->execTF = tf;


    b = WMCreateCommandButton(f2);
    WMResizeWidget(b, 80, 18);
    WMMoveWidget(b, (frame_width-80)/2, 188);
    WMSetButtonText(b, _("Set"));
    WMSetButtonAction(b, genericButtonActionHandler, prefs);
    WMMapWidget(b);
    prefs->execSetBtn = b;
    /* End Executables */

    /* Start Display */
    f2 = WMCreateFrame(f);
    WMSetFrameRelief(f2, WRGroove);
    WMSetFrameTitle(f2, _(" Display "));
    WMMoveWidget(f2, PADX*2+frame_width*2+padx*2, 10);
    WMResizeWidget(f2, frame_width, frame_height);
    WMMapWidget(f2);

    list = WMCreateList(f2);
    WMMoveWidget(list, 10, 30);
    WMResizeWidget(list, frame_width-20, 100);
    WMSetListAction(list, genericListClick, prefs);
    WMMapWidget(list);
    prefs->dpyList = list;


    l = WMCreateLabel(f2);
    WMMoveWidget(l, 10, 140);
    WMResizeWidget(l, frame_width-20, 18);
    WMSetLabelText(l, _("Display:"));
    WMSetLabelTextColor(l, WMDarkGrayColor(prefs->scr));
    WMMapWidget(l);

    tf = WMCreateTextField(f2);
    WMMoveWidget(tf, 10, 160);
    WMResizeWidget(tf, frame_width-20, 18);
    WMSetTextFieldText(tf, _("Display"));
/*     WMSetTextFieldDelegate(tPtr, delegate); */
    setTFDelegate(prefs, tf);
    WMMapWidget(tf);
    prefs->dpyTF = tf;


    b = WMCreateCommandButton(f2);
    WMResizeWidget(b, 80, 18);
    WMMoveWidget(b, (frame_width-80)/2, 188);
    WMSetButtonText(b, _("Set"));
    WMSetButtonAction(b, genericButtonActionHandler, prefs);
    WMMapWidget(b);
    prefs->dpySetBtn = b;
    /* End Display */

    tab = WMCreateTabViewItemWithIdentifier(VARS_TAB);
    WMSetTabViewItemView(tab, WMWidgetView(f));
    WMAddItemInTabView(prefs->tabV, tab);
    WMSetTabViewItemLabel(tab, _("Variables"));
}


static void
populateVarsTab(FSPreferencesPanel *prefs)
{
    WMListItem *item;

    item = WMAddListItem(prefs->imgList, "ROOT");
    if (item)
	item->clientData = FSGetStringForNameKey("ROOT", "icon");

    item = WMAddListItem(prefs->imgList, "DIRECTORY");
    if (item)
	item->clientData = FSGetStringForName("DIRECTORY");

    item = WMAddListItem(prefs->imgList, "DEFAULT_IMG");
    if (item)
	item->clientData = FSGetStringForName("DEFAULT_IMG");

    item = WMAddListItem(prefs->imgList, "HOME");
    if (item)
	item->clientData = FSGetStringForName("HOME");

    item = WMAddListItem(prefs->execList, "CONSOLE");
    if (item)
	item->clientData = FSGetStringForNameKey("CONSOLE", "exec");
    
    item = WMAddListItem(prefs->execList, "PROCESS");
    if (item)
	item->clientData = FSGetStringForNameKey("PROCESS", "exec");
    
    item = WMAddListItem(prefs->execList, "MAGICASCII");
    if (item)
	item->clientData = FSGetStringForNameKey("MAGICASCII", "exec");
    
    item = WMAddListItem(prefs->execList, "MAGICIMAGE");
    if (item)
	item->clientData = FSGetStringForNameKey("MAGICIMAGE", "exec");
    
    item = WMAddListItem(prefs->execList, "MAGICPS");
    if (item)
	item->clientData = FSGetStringForNameKey("MAGICPS", "exec");
    
    item = WMAddListItem(prefs->dpyList, "ColumnWidth");
    if (item)
    {
	char buf[10];

	sprintf(buf, "%d", FSGetIntegerForName("ColumnWidth"));
	item->clientData = wstrdup(buf);
    }
    
/*     item = WMAddListItem(prefs->dpyList, "IconWidth"); */
/*     if (item) */
/*     { */
/* 	char buf[10]; */

/* 	sprintf(buf, "%d", WMGetIntegerForKey(defaultsDB, "IconWidth")); */
/* 	item->clientData = wstrdup(buf); */
/*     } */
    
    item = WMAddListItem(prefs->dpyList, "ICONDIR");
    if (item)
	item->clientData = FSGetStringForName("ICONDIR");
    
    item = WMAddListItem(prefs->dpyList, "SortDisplay");
    if (item)
    {
	char buf[10];

	sprintf(buf, "%d", FSGetIntegerForName("SortDisplay"));
	item->clientData = wstrdup(buf);
    }
    
    item = WMAddListItem(prefs->dpyList, "SortOrder");
    if (item)
    {
	char buf[10];

	sprintf(buf, "%d", FSGetIntegerForName("SortOrder"));
	item->clientData = wstrdup(buf);
    }
    
    item = WMAddListItem(prefs->dpyList, "DisplaySVBG");
    if (item)
    {
	char buf[10];

	sprintf(buf, "%d", FSGetIntegerForName("DisplaySVBG"));
	item->clientData = wstrdup(buf);
    }
    
    item = WMAddListItem(prefs->dpyList, "DisplayMCListPixmap");
    if (item)
    {
	char buf[10];

	sprintf(buf, "%d", FSGetIntegerForName("DisplayMCListPixmap"));
	item->clientData = wstrdup(buf);
    }
    WMSortListItems(prefs->imgList);
    WMSortListItems(prefs->execList);
    WMSortListItems(prefs->dpyList);
}

	
static void
populateTypesTab(FSPreferencesPanel *prefs)
{
    int numElem, i;
    WMListItem *item   = NULL;
    WMPropList* array   = NULL;
    WMPropList* element = NULL;
    
    WMClearList(prefs->typesList);
    WMSetTextFieldText(prefs->appsExecTF, "");
    WMSetButtonImage(prefs->appsIconBtn, NULL);

    WMClearList(prefs->appsList);
    WMSetTextFieldText(prefs->typesViewTF, "");
    WMSetTextFieldText(prefs->typesEditTF, "");
    WMSetButtonImage(prefs->typesIconBtn, NULL);

    array = FSGetUDObjectForKey(defaultsDB, "EXTN");
    if (array && WMIsPLArray(array))
    {
      
	numElem = WMGetPropListItemCount(array);
	for(i = 0; i < numElem; i++)	
	{
	    element = WMGetFromPLArray(array, i);
	    
	    if (element)
	    {
		char *str = WMGetPropListDescription(element, False);

		if (str)
		    item = WMAddListItem(prefs->typesList, str);
		else
		    continue;

		if (item)
		{
		    WMPropList* tmp = NULL;

		    tmp = FSGetUDObjectForKey(defaultsDB, str);
		    if (tmp)
			item->clientData = WMDeepCopyPropList(tmp);
		}
		if (str)
		    free(str);
	    }
	}
    } 

    array = FSGetUDObjectForKey(defaultsDB, "EXE");
    if (array && WMIsPLArray(array))
    {
	numElem = WMGetPropListItemCount(array);
	for(i = 0; i < numElem; i++)
	{
	    element = WMGetFromPLArray(array, i);
	    
	    if (element)
	    {
		char *str = WMGetPropListDescription(element, False);

		if (str)
		    item = WMAddListItem(prefs->appsList, str);
		else
		    continue;

		if (item)
		{
		    WMPropList* tmp = NULL;

		    tmp = FSGetUDObjectForKey(defaultsDB, str);
		    if (tmp)
			item->clientData = WMDeepCopyPropList(tmp);
		}
		if (str)
		    free(str);
	    }
	}
    } 
    /* new in 0.2.5 */
    WMSortListItems(prefs->appsList);
    WMSortListItems(prefs->typesList);
}

static void
populateDisksTab(FSPreferencesPanel *prefs)
{
    WMPropList* devArray = NULL;
    WMPropList* array    = NULL;

    devArray = FSGetUDObjectForKey(defaultsDB, "DISCS");

    if (devArray && WMIsPLArray(devArray))
    {
	int i, numElem;
	WMPropList* tmp;

	numElem = WMGetPropListItemCount(devArray);
	for(i = 0; i < numElem; i++)
	{
	    array = WMGetFromPLArray(devArray, i);
	    if (array && WMIsPLArray(array))
	    {
		Disc *disk       = NULL;
		WMListItem *item = NULL;

		disk = (Disc *) wmalloc(sizeof(Disc));
		memset(disk, 0, sizeof(Disc));

		tmp = WMGetFromPLArray(array, 0);
		if (WMIsPLString(tmp))
		{
		    disk->name = wstrdup(WMGetFromPLString(tmp));
		    item = WMAddListItem(prefs->disksList, disk->name);
		}
		else
		{
		    continue;
		}

		tmp = WMGetFromPLArray(array, 1);
		if (WMIsPLString(tmp))
		    disk->point = wstrdup(WMGetFromPLString(tmp));
		
		tmp = WMGetFromPLArray(array, 2);
		if (WMIsPLString(tmp))
		    disk->device = wstrdup(WMGetFromPLString(tmp));
		
		tmp = WMGetFromPLArray(array, 3);
		if (WMIsPLString(tmp))
		    disk->mnt = wstrdup(WMGetFromPLString(tmp));
		
		tmp = WMGetFromPLArray(array, 4);
		if (WMIsPLString(tmp))
		    disk->umnt = wstrdup(WMGetFromPLString(tmp));
		
		tmp = WMGetFromPLArray(array, 5);
		if (WMIsPLString(tmp))
		    disk->eject = wstrdup(WMGetFromPLString(tmp));
	       
		tmp = WMGetFromPLArray(array, 6);
		if (WMIsPLString(tmp))
		    disk->close = wstrdup(WMGetFromPLString(tmp));

		if (item)
		    item->clientData = disk;		
	    }
	}
    }
    WMSortListItems(prefs->disksList);
}

static void
populateDisksFields(FSPreferencesPanel *prefs)
{
    WMListItem *item    = WMGetListSelectedItem(prefs->disksList);
    Disc *disk          = (Disc *)item->clientData;

    if (disk->name)
	WMSetTextFieldText(prefs->disksNameTF, disk->name);
    else
	WMSetTextFieldText(prefs->disksNameTF, "");

    if (disk->point)
	WMSetTextFieldText(prefs->disksMntTF, disk->point);
    else
	WMSetTextFieldText(prefs->disksMntTF, "");

    if (disk->device)
	WMSetTextFieldText(prefs->disksDevTF, disk->device);
    else
	WMSetTextFieldText(prefs->disksDevTF, "");

    if (disk->mnt)
	WMSetTextFieldText(prefs->cmdMntTF, disk->mnt);
    else
	WMSetTextFieldText(prefs->cmdMntTF, "");

    if (disk->umnt)
	WMSetTextFieldText(prefs->cmdUMntTF, disk->umnt);
    else
	WMSetTextFieldText(prefs->cmdUMntTF, "");

    if (disk->eject)
	WMSetTextFieldText(prefs->cmdEjectTF, disk->eject);
    else
	WMSetTextFieldText(prefs->cmdEjectTF, "");

    if (disk->close)
	WMSetTextFieldText(prefs->cmdCloseTF, disk->close);
    else
	WMSetTextFieldText(prefs->cmdCloseTF, "");
		
}

static void
populateAppsFields(FSPreferencesPanel *prefs)
{
    char       *str   = NULL;
    WMPropList*  val   = NULL;
    WMListItem *item  = WMGetListSelectedItem(prefs->appsList);

    if (!item)
	return;

    val = FSGetDBObjectForKey(item->clientData, "exec");
    if (val && WMIsPLString(val))
	WMSetTextFieldText(prefs->appsExecTF, WMGetFromPLString(val));

    val = FSGetDBObjectForKey(item->clientData, "icon");
    if (val && WMIsPLString(val))
    {
	char *img = LocateImage(WMGetFromPLString(val));
	
	if (img)
	{
	    FSSetButtonImageFromFile(prefs->appsIconBtn, img);
	    WMHangData(prefs->appsIconBtn, WMGetFromPLString(val));
	    free(img);
	}
    }
    else
	WMSetButtonImage(prefs->appsIconBtn, NULL);

    /*
     * uflags indicates whether or not the list item should be
     * removed.
     */
    if (item->uflags)
	WMSetButtonText(prefs->appsRemoveBtn, _("Undelete"));
    else
	WMSetButtonText(prefs->appsRemoveBtn, _("Delete"));
}

static void
populateTypesFields(FSPreferencesPanel *prefs)
{
    char       *str   = NULL;
    WMPropList*  val   = NULL;
    WMListItem *item  = WMGetListSelectedItem(prefs->typesList);

    if (!item)
	return;

    val = FSGetDBObjectForKey(item->clientData, "editor");
    if (val && WMIsPLString(val))
	WMSetTextFieldText(prefs->typesEditTF, WMGetFromPLString(val));
    else
	WMSetTextFieldText(prefs->typesEditTF, "");

    val = FSGetDBObjectForKey(item->clientData, "viewer");
    if (val && WMIsPLString(val))
	WMSetTextFieldText(prefs->typesViewTF, WMGetFromPLString(val));
    else
	WMSetTextFieldText(prefs->typesViewTF, "");

    val = FSGetDBObjectForKey(item->clientData, "icon");
    if (val && WMIsPLString(val))
    {
	char *img = LocateImage(WMGetFromPLString(val));
	
	if (img)
	{
	    FSSetButtonImageFromFile(prefs->typesIconBtn, img);
	    WMHangData(prefs->typesIconBtn, WMGetFromPLString(val));
	    free(img);
	}
    }
    else
	WMSetButtonImage(prefs->typesIconBtn, NULL);

    if (item->uflags)
    {
	WMSetButtonText(prefs->typesRemoveBtn, _("Undelete"));
    }
    else
    {
	WMSetButtonText(prefs->typesRemoveBtn, _("Delete"));
    }

}

static clearDisksFields(FSPreferencesPanel *prefs)
{
    WMSetTextFieldText(prefs->disksNameTF, "");
    WMSetTextFieldText(prefs->disksMntTF,  "");
    WMSetTextFieldText(prefs->disksDevTF,  "");
    WMSetTextFieldText(prefs->cmdMntTF,    "");
    WMSetTextFieldText(prefs->cmdUMntTF,   "");
    WMSetTextFieldText(prefs->cmdEjectTF,  "");
    WMSetTextFieldText(prefs->cmdCloseTF,  "");
}

static void
updateDisksList(FSPreferencesPanel *prefs)
{
    WMListItem *item = WMGetListSelectedItem(prefs->disksList);
    Disc *disk = NULL;
    char *name = WMGetTextFieldText(prefs->disksNameTF);

    if (item)
    { 
	/* If an item is selected, update it's settings */
	disk = (Disc *) item->clientData;
	item->text = name;
    }
    else
    {
	/* If no item has been selected, its a new entry */
	disk = (Disc *) wmalloc(sizeof(Disc));
	memset(disk, 0, sizeof(Disc));

	item = WMAddListItem(prefs->disksList, name);
	item->clientData = disk;
    }

    /* 
     * This could be a problem if an empty entry is
     * created. I wonder what will happen!!
     */
    if (disk)
    {
	disk->name   = WMGetTextFieldText(prefs->disksNameTF);
	disk->point  = WMGetTextFieldText(prefs->disksMntTF);
	disk->device = WMGetTextFieldText(prefs->disksDevTF);
	disk->mnt    = WMGetTextFieldText(prefs->cmdMntTF);
	disk->umnt   = WMGetTextFieldText(prefs->cmdUMntTF);
	disk->eject  = WMGetTextFieldText(prefs->cmdEjectTF);
	disk->close  = WMGetTextFieldText(prefs->cmdCloseTF);
    }
}

static void
genericListClick(WMWidget *self, void *data)
{
    WMList             *list  = (WMList *) self;
    WMListItem         *item  = WMGetListSelectedItem(list);
    FSPreferencesPanel *prefs = (FSPreferencesPanel *) data;

    if (list  == prefs->imgList)
    {
	char *img = NULL;

	img = LocateImage(item->clientData);
	
	if (img)
	{
	    FSSetButtonImageFromFile(prefs->imgIconBtn, img);
	    free(img);
	}
    }
    else if (list == prefs->execList)
    {
	WMSetTextFieldText(prefs->execTF, item->clientData);
    }
    else if (list == prefs->dpyList)
    {
	WMSetTextFieldText(prefs->dpyTF, item->clientData);
    }
    else if (list == prefs->disksList)
    {
	populateDisksFields(prefs);
    }
    else if (list == prefs->typesList)
    {
	populateTypesFields(prefs);
    }
    else if (list == prefs->appsList)
    {
	populateAppsFields(prefs);
    }
}

static void
genericButtonActionHandler(WMWidget *self, void *clientData)
{
    Bool edited = False;
    WMOpenPanel *oPanel = NULL;
    WMButton    *btn    = (WMButton *) self;
    FSPreferencesPanel *prefs = (FSPreferencesPanel *) clientData;

    if (btn == prefs->imgIconBtn)
    {
	char *img = NULL;
	char *tmp = NULL;
	WMListItem *item = WMGetListSelectedItem(prefs->imgList);

	if (!item || !item->clientData)
	    return;

	tmp = FSRunSelectIconPanel(prefs->win, _("Icon Select"),item->clientData);
	if (tmp == NULL)
	    return;
	
	img = LocateImage(tmp);	
	if (img)
	{
	    FSSetButtonImageFromFile(prefs->imgIconBtn, img);
	    free(img);
	}
	free(item->clientData);
	item->clientData = tmp;
	edited = True;
    }
    else if (btn == prefs->execSetBtn)
    {
	WMListItem *item = WMGetListSelectedItem(prefs->execList);

	if (!item)
	    return;

	item->clientData = WMGetTextFieldText(prefs->execTF);
	edited = True;
/* 	WMSetTextFieldText(prefs->execTF, ""); */
/* 	WMSelectListItem(prefs->execList, -1); */
    }
    else if (btn == prefs->dpySetBtn)
    {
	WMListItem *item = WMGetListSelectedItem(prefs->dpyList);

	if (!item)
	    return;

	item->clientData = WMGetTextFieldText(prefs->dpyTF);
/* 	WMSetTextFieldText(prefs->dpyTF, ""); */
/* 	WMSelectListItem(prefs->dpyList, -1); */
	edited = True;
    }
    else if (btn == prefs->disksNewBtn)
    {
	clearDisksFields(prefs);
	WMSelectListItem(prefs->disksList, -1);
    }
    else if (btn == prefs->disksRemoveBtn)
    {
	int row = -1;
	WMListItem *item = NULL;

	clearDisksFields(prefs);
	item = WMGetListSelectedItem(prefs->disksList);
	if (item)
	{
	    free(item->clientData);
	    item->clientData = NULL;
	}
	row = WMGetListSelectedItemRow(prefs->disksList);
	WMRemoveListItem(prefs->disksList, row);
	edited = True;
    }
    else if (btn == prefs->disksUpdateBtn)
    {
	updateDisksList(prefs);

/* 	clearDisksFields(prefs); */
/* 	WMSelectListItem(prefs->disksList, -1); */
	edited = True;
    }
    else if (btn == prefs->appsSetBtn)
    {
	char *appStr     = WMGetTextFieldText(prefs->appsExecTF);
	char *imgStr     = WMGetHangedData(prefs->appsIconBtn);
	WMListItem *item = WMGetListSelectedItem(prefs->appsList);
	WMPropList* array = FSGetUDObjectForKey(defaultsDB, "EXE");


	if (!item)
	    return;
        
	if (appStr) /* crash in 0.2.3e*/
	{
	  if (item->clientData) {
	    WMPutInPLDictionary(item->clientData, 
	      WMCreatePLString("exec"), WMCreatePLString(appStr));
	  }
	  /* new in V 0.2.3f */
	  else {
	    item->clientData = WMCreatePLDictionary(
	      WMCreatePLString("exec"), WMCreatePLString(appStr), NULL);
	  }
	}
	if (imgStr)
	{
	  if (item->clientData) {
	    WMPutInPLDictionary(item->clientData, 
	      WMCreatePLString("icon"), WMCreatePLString(imgStr));
	  }
	  /* new in V 0.2.3f */
	  else {
	    item->clientData = WMCreatePLDictionary(
	      WMCreatePLString("icon"), WMCreatePLString(imgStr), NULL);
	  }
	}

	if (array && WMIsPLArray(array))
	{
	    InsertArrayElement(array, WMCreatePLString(item->text));
	}

	edited = True;
    }
    else if (btn == prefs->appsRemoveBtn)
    {
	WMListItem *item = WMGetListSelectedItem(prefs->appsList);

	if (!item)
	    return;

	if (!item->uflags)
	{
	    item->uflags = 1;
	    WMSetButtonText(btn, _("Undelete"));
	}
	else
	{
	    item->uflags = 0;
	    WMSetButtonText(btn, _("Delete"));
	}
	/* Update of ListView? */
	WMSelectListItem(prefs->appsList,
			 WMGetListSelectedItemRow(prefs->appsList));

	edited = True;
    }
    else if (btn == prefs->typesSetBtn)
    {
	char *viewStr    = WMGetTextFieldText(prefs->typesViewTF);
	char *editStr    = WMGetTextFieldText(prefs->typesEditTF);
	char *imgStr     = WMGetHangedData(prefs->typesIconBtn);
	WMListItem *item = WMGetListSelectedItem(prefs->typesList);
	WMPropList* array = FSGetUDObjectForKey(defaultsDB, "EXTN");

	if (!item)
	    return;

	if (viewStr)
	{
	  if (item->clientData) {
	    WMPutInPLDictionary(item->clientData, 
	      WMCreatePLString("viewer"), WMCreatePLString(viewStr));
	  }
	  /* new in V 0.2.3f */
	  else {
	    item->clientData = WMCreatePLDictionary(
	      WMCreatePLString("viewer"), WMCreatePLString(viewStr), NULL);
	  }
	}

	if (editStr)
	{
	  if (item->clientData) {
	    WMPutInPLDictionary(item->clientData, 
	      WMCreatePLString("editor"), WMCreatePLString(editStr));
	  }
	  /* new in V 0.2.3f */
	  else {
	    item->clientData = WMCreatePLDictionary(
	      WMCreatePLString("editor"), WMCreatePLString(editStr), NULL);
	  }
	}
	if (imgStr)
	{
	  if (item->clientData) {
	    WMPutInPLDictionary(item->clientData, 
	      WMCreatePLString("icon"), WMCreatePLString(imgStr));
	  }
	  /* new in V 0.2.3f */
	  else {
	    item->clientData = WMCreatePLDictionary(
	      WMCreatePLString("icon"), WMCreatePLString(imgStr), NULL);
	  }
	}

	if (array && WMIsPLArray(array))
	{
	    InsertArrayElement(array, WMCreatePLString(item->text));
	}

	edited = True;
    }
    else if (btn == prefs->typesRemoveBtn)
    {
	WMListItem *item = WMGetListSelectedItem(prefs->typesList);

	if (!item)
	    return;

	if (!item->uflags)
	{
	    item->uflags = 1;
	    WMSetButtonText(btn, _("Undelete"));
	}
	else
	{
	    item->uflags = 0;
	    WMSetButtonText(btn, _("Delete"));
	}
	WMSelectListItem(prefs->typesList,
			 WMGetListSelectedItemRow(prefs->typesList));

	edited = True;
    }
    else if (btn == prefs->appsIconBtn)
    {
	char *img = NULL;
	char *tmp = NULL;
	WMPropList* val;
	WMListItem *item = WMGetListSelectedItem(prefs->appsList);

	if (!item || !item->clientData)
	    return;

	val = FSGetDBObjectForKey(item->clientData, "icon");
	if (val && WMIsPLString(val))
	{
	    tmp = FSRunSelectIconPanel(prefs->win, _("Icon Select"),
				       WMGetFromPLString(val));
	    if (tmp == NULL)
		return;
	    
	    img = LocateImage(tmp);	
	    if (img)
	    {
		FSSetButtonImageFromFile(prefs->appsIconBtn, img);
		WMHangData(prefs->appsIconBtn, tmp);
	    }
	}
	else
	    WMSetButtonImage(prefs->appsIconBtn, NULL);
/* 	edited = True; */
    }
    else if (btn == prefs->typesIconBtn)
    {
	char *img = NULL;
	char *tmp = NULL;
	WMPropList* val;
	WMListItem *item = WMGetListSelectedItem(prefs->typesList);

	if (!item || !item->clientData)
	    return;

	val = FSGetDBObjectForKey(item->clientData, "icon");
	if (val && WMIsPLString(val))
	{
	    tmp = FSRunSelectIconPanel(prefs->win, _("Icon Select"),
				       WMGetFromPLString(val));
	    if (tmp == NULL)
		return;
	    
	    img = LocateImage(tmp);	
	    if (img)
	    {
		FSSetButtonImageFromFile(prefs->typesIconBtn, img);
		WMHangData(prefs->typesIconBtn, tmp);
	    }
	}
	else
	    WMSetButtonImage(prefs->typesIconBtn, NULL);
/* 	edited = True; */
    }
    else if (btn == prefs->appsBrowseBtn)
    {
	WMOpenPanel *oPanel = NULL;
	
	oPanel = WMGetOpenPanel(prefs->scr);
        if (WMRunModalFilePanelForDirectory(oPanel, prefs->win, 
					   "/", _("Browse -> App"), NULL))
	{
	    char *str = WMGetFilePanelFileName(oPanel);

	    WMSetTextFieldText(prefs->appsExecTF, str);
	    WMInsertTextFieldText(prefs->appsExecTF, " %s", strlen(str));
	}
    }
    else if (btn == prefs->typesViewBrowseBtn)
    {
	oPanel = WMGetOpenPanel(prefs->scr);
        if (WMRunModalFilePanelForDirectory(oPanel, prefs->win, 
					   "/", _("Browse -> Viewer"), NULL))
	{
	    char *str = WMGetFilePanelFileName(oPanel);

	    WMSetTextFieldText(prefs->typesViewTF, str);
	    WMInsertTextFieldText(prefs->typesViewTF, " %s", strlen(str));
	}
    }
    else if (btn == prefs->typesEditBrowseBtn)
    {
	oPanel = WMGetOpenPanel(prefs->scr);
        if (WMRunModalFilePanelForDirectory(oPanel, prefs->win, 
					   "/", _("Browse -> Editor"), NULL))
	{
	    char *str = WMGetFilePanelFileName(oPanel);

	    WMSetTextFieldText(prefs->typesEditTF, str);
	    WMInsertTextFieldText(prefs->typesEditTF, " %s", strlen(str));
	}
    }

    if (edited)
    {
	int id = -1;
	WMTabViewItem *item = NULL;
	
	item = WMGetSelectedTabViewItem(prefs->tabV);
	if (!item)
	    return;
	
	id = WMGetTabViewItemIdentifier(item);
	
	switch(id)
	{
	case VARS_TAB :  
	    prefs->flags.evars = True;
	    break;
	case APPS_TAB :  
	    prefs->flags.eapps = True;
	    break;
	case DISCS_TAB :  
	    prefs->flags.ediscs = True;
	    break;
	default:  
	    break;
	}
	WMSetButtonEnabled(prefs->applyBtn, True);
	WMSetButtonEnabled(prefs->okBtn, True);
    }
}

static void
setVarsImage(WMWidget *self, void *data)
{
}

static void FSClosePreferencesPanel(WMWidget *self, void *client)
{
    FSPreferencesPanel	*prefs = (FSPreferencesPanel *) client;
    
    prefs->flags.done = 1;
}

static void 
saveAllTabsAndClose(WMWidget *self, void *client)
{
    int i;
    FSPreferencesPanel	*prefs = (FSPreferencesPanel *) client;
    
    for (i = 0; i < NUM_TABS; i++)
	saveTab(prefs, i);

    FSClosePreferencesPanel(self, client);
}

static void
applyBtnCB(WMWidget *self, void *client)
{
    int id = -1;
    WMTabViewItem *item = NULL;
    FSPreferencesPanel	*prefs = (FSPreferencesPanel *) client;
    
    item = WMGetSelectedTabViewItem(prefs->tabV);
    if (!item)
	return;
    id = WMGetTabViewItemIdentifier(item);
	
    switch(id)
    {
    case VARS_TAB :  
	prefs->flags.evars = False;
	break;
    case APPS_TAB :  
	prefs->flags.eapps = False;
	break;
    case DISCS_TAB :  
	prefs->flags.ediscs = False;
	break;
    default:  
	break;
    }
    WMSetButtonEnabled(prefs->applyBtn, False);
    saveTab(prefs, id);
}

static void
saveTab(FSPreferencesPanel *prefs, int tabID)
{
    switch(tabID)
    {
    case VARS_TAB  : saveVarsTab(prefs);
	break;
    case APPS_TAB  : saveAppsTab(prefs);
	break;
    case DISCS_TAB : saveDiscsTab(prefs);
	break;
    default:
	break;
    }
    WMSynchronizeUserDefaults(defaultsDB);
}

static void 
FSDestroyPreferencesPanel(FSPreferencesPanel *preferences)
{
    WMUnmapWidget(preferences->win);
    WMDestroyWidget(preferences->win);
    free((void *) preferences);
    preferences = NULL;
}

static void
tfDidChange(struct WMTextFieldDelegate *self, WMNotification *notif)
{

}

static void
setTFDelegate(FSPreferencesPanel *prefs, WMTextField *tf)
{
    WMTextFieldDelegate *delegate = NULL;

    delegate = (WMTextFieldDelegate *) wmalloc(sizeof(WMTextFieldDelegate));
    memset(delegate, 0, sizeof(WMTextFieldDelegate));

    if (delegate)
    {
	delegate->data = prefs;
	delegate->didChange = tfDidChange;
	WMSetTextFieldDelegate(tf, delegate);
    }
}

static void
setTVIDelegate(FSPreferencesPanel *prefs, WMTabView *tv)
{
    WMTabViewDelegate *delegate = NULL;

    delegate = (WMTabViewDelegate *) wmalloc(sizeof(WMTabViewDelegate));
    memset(delegate, 0, sizeof(WMTabViewDelegate));

    if (delegate)
    {
	delegate->data = prefs;
	delegate->didSelectItem = tvDidSelectItem;
	WMSetTabViewDelegate(tv, delegate);
    }
}

static void
tvDidSelectItem(struct WMTabViewDelegate *self, WMTabView *tabView,
		WMTabViewItem *item)
{
    int id = -1;
    Bool edited = False;
    WMTabViewDelegate  *delegate = (WMTabViewDelegate *)self;
    FSPreferencesPanel *prefs    = (FSPreferencesPanel *) delegate->data;

    id = WMGetTabViewItemIdentifier(item);

    switch(id)
    {
    case VARS_TAB : 
	if (prefs->flags.evars)
	    edited = True;
	break;
    case APPS_TAB : 
	if (prefs->flags.eapps)
	    edited = True;
	break;
    case DISCS_TAB :  
	if (prefs->flags.ediscs)
	    edited = True;
	break;
    default:  edited = False;
	break;
    }

    WMSetButtonEnabled(prefs->applyBtn, edited);
}

static void
saveVarsTab(FSPreferencesPanel *prefs)
{
    int row;
    WMListItem *item = NULL;

    row = WMFindRowOfListItemWithTitle(prefs->imgList, "ROOT");
    if (row >= 0)
    {
	item = WMGetListItem(prefs->imgList, row);
	if (item)
	    FSSetStringForNameKey("ROOT", "icon", item->clientData);
    }

    row = WMFindRowOfListItemWithTitle(prefs->imgList, "DIRECTORY");
    if (row >= 0)
    {
	item = WMGetListItem(prefs->imgList, row);
	if (item)
	    FSSetStringForName("DIRECTORY", item->clientData);
    }

    row = WMFindRowOfListItemWithTitle(prefs->imgList, "DEFAULT_IMG");
    if (row >= 0)
    {
	item = WMGetListItem(prefs->imgList, row);
	if (item)
	    FSSetStringForName("DEFAULT_IMG", item->clientData);
    }

    row = WMFindRowOfListItemWithTitle(prefs->imgList, "HOME");
    if (row >= 0)
    {
	item = WMGetListItem(prefs->imgList, row);
	if (item)
	    FSSetStringForName("HOME", item->clientData);
    }

    row = WMFindRowOfListItemWithTitle(prefs->execList, "CONSOLE");
    if (row >= 0)
    {
	item = WMGetListItem(prefs->execList, row);
	if (item)
	    FSSetStringForNameKey("CONSOLE", "exec", item->clientData);
    }

    row = WMFindRowOfListItemWithTitle(prefs->execList, "PROCESS");
    if (row >= 0)
    {
	item = WMGetListItem(prefs->execList, row);
	if (item)
	    FSSetStringForNameKey("PROCESS", "exec", item->clientData);
    }

    row = WMFindRowOfListItemWithTitle(prefs->execList, "MAGICASCII");
    if (row >= 0)
    {
	item = WMGetListItem(prefs->execList, row);
	if (item)
	    FSSetStringForNameKey("MAGICASCII", "exec", item->clientData);
    }

    row = WMFindRowOfListItemWithTitle(prefs->execList, "MAGICIMAGE");
    if (row >= 0)
    {
	item = WMGetListItem(prefs->execList, row);
	if (item)
	    FSSetStringForNameKey("MAGICIMAGE", "exec", item->clientData);
    }
    
    row = WMFindRowOfListItemWithTitle(prefs->execList, "MAGICPS");
    if (row >= 0)
    {
	item = WMGetListItem(prefs->execList, row);
	if (item)
	    FSSetStringForNameKey("MAGICPS", "exec", item->clientData);
    }

    row = WMFindRowOfListItemWithTitle(prefs->dpyList, "ColumnWidth");
    if (row >= 0)
    {
	item = WMGetListItem(prefs->dpyList, row);
	if (item) {
            int value = atoi(item->clientData);
            if (FSGetIntegerForName("ColumnWidth") != value) {
                if (value < 169) /*MAGIC*/
                    value = 169;
                FSSetIntegerForName("ColumnWidth", value);
                /* TODO: update viewer width */
            }
        }
    }    

/*     row = WMFindRowOfListItemWithTitle(prefs->imgList, "IconWidth"); */
/*     if (row >= 0) */
/*     { */
/* 	item = WMGetListItem(prefs->dpyList, row); */
/* 	if (item) */
/* 	    FSSetIntegerForName("IconWidth", atoi(item->clientData)); */
/*     }     */

    
    row = WMFindRowOfListItemWithTitle(prefs->dpyList, "ICONDIR");
    if (row >= 0)
    {
	item = WMGetListItem(prefs->dpyList, row);
	if (item)
	    FSSetStringForName("ICONDIR", item->clientData);
    }

    row = WMFindRowOfListItemWithTitle(prefs->dpyList, "SortDisplay");
    if (row >= 0)
    {
	item = WMGetListItem(prefs->dpyList, row);
	if (item) {
            int value = atoi(item->clientData);
            if (FSGetIntegerForName("SortDisplay") != value) {
                FSSetIntegerForName("SortDisplay", value);
                FSUpdateFileViewPath(
                        FSGetFSViewerCurrentView(prefs->app),
                        FileSync, NULL, NULL);
            }
        }
    }
    
    row = WMFindRowOfListItemWithTitle(prefs->dpyList, "SortOrder");
    if (row >= 0)
    {
        item = WMGetListItem(prefs->dpyList, row);
        if (item) {
            int value = atoi(item->clientData);
            if (FSGetIntegerForName("SortOrder") != value) {
                FSSetIntegerForName("SortOrder", value);
                FSUpdateFileViewPath(
                        FSGetFSViewerCurrentView(prefs->app),
                        FileSync, NULL, NULL);
            }
        }
    }
    
    row = WMFindRowOfListItemWithTitle(prefs->dpyList, "DisplaySVBG");
    if (row >= 0)
    {
	item = WMGetListItem(prefs->dpyList, row);
	if (item) {
            int value = atoi(item->clientData);
            if (FSGetIntegerForName("DisplaySVBG") != value) {
                FSSetIntegerForName("DisplaySVBG", value);
                /* TODO: update view*/
            }
        }
    }

    row = WMFindRowOfListItemWithTitle(prefs->dpyList, "DisplayMCListPixmap");
    if (row >= 0)
    {
	item = WMGetListItem(prefs->dpyList, row);
	if (item) {
            int value = atoi(item->clientData);
            if (FSGetIntegerForName("DisplayMCListPixmap") != value) {
                FSSetIntegerForName("DisplayMCListPixmap", value);
                /* TODO: update view*/
            }
        }
    }
}

static void
saveAppsTab(FSPreferencesPanel *prefs)
{
    int row = -1;
    int numRows = -1;
    WMListItem *item = NULL;
    WMPropList* exeArray = FSGetUDObjectForKey(defaultsDB, "EXE");
    WMPropList* extnArray = FSGetUDObjectForKey(defaultsDB, "EXTN");


    numRows = WMGetListNumberOfRows(prefs->appsList);

    for (row = 0; row < numRows; row++)
    {
	item = WMGetListItem(prefs->appsList, row);
	if (item && !item->uflags)
	    WMSetUDObjectForKey(defaultsDB, item->clientData, item->text);
	else
	{
	    if (exeArray && WMIsPLArray(exeArray))
	    {
		FSRemoveArrayElement(exeArray, WMCreatePLString(item->text));
		WMRemoveUDObjectForKey(defaultsDB, item->text);
	    }
	}
    }

    numRows = WMGetListNumberOfRows(prefs->typesList);
    for (row = 0; row < numRows; row++)
    {
	item = WMGetListItem(prefs->typesList, row);
	if (item && !item->uflags)
	{
	    WMSetUDObjectForKey(defaultsDB, item->clientData, item->text);
	}
	else
	{
	    if (extnArray && WMIsPLArray(extnArray))
	    {
		FSRemoveArrayElement(extnArray, WMCreatePLString(item->text));
		WMRemoveUDObjectForKey(defaultsDB, item->text);
	    }
	}
    }
    populateTypesTab(prefs);
}

static void
saveDiscsTab(FSPreferencesPanel *prefs)
{
    int row = -1;
    int numRows = -1;
    WMListItem *item = NULL;
    WMPropList* array = NULL;
    WMPropList* diskArray = NULL;
    Disc *disk = NULL;

    diskArray = WMCreatePLArray(NULL, NULL);

    numRows = WMGetListNumberOfRows(prefs->disksList);
    for (row = 0; row < numRows; row++)
    {
	item = WMGetListItem(prefs->disksList, row);
	disk = (Disc *)item->clientData;

	if (item)
	{
	    array = WMCreatePLArray(WMCreatePLString(disk->name), 
					     WMCreatePLString(disk->point), 
					     WMCreatePLString(disk->device),
					     WMCreatePLString(disk->mnt),
					     WMCreatePLString(disk->umnt),
					     WMCreatePLString(disk->eject),
					     WMCreatePLString(disk->close),
					     NULL );
	    WMAddToPLArray(diskArray, array);
	}
    }
    WMSetUDObjectForKey(defaultsDB, diskArray, "DISCS");
}

static void
genericListDrawProc(WMList *lPtr, int index, Drawable d, 
		    char *text, int state, WMRect *rect)
{
    WMView   *view   = W_VIEW(lPtr);
    W_Screen *scr    = view->screen;
    WMListItem *item = WMGetListItem(lPtr, index);

    if (item->selected)
        WMPaintColorSwatch(WMWhiteColor(scr), d,
		       rect->pos.x, rect->pos.y,
		       rect->size.width, rect->size.height);
    else
        WMPaintColorSwatch(WMGetWidgetBackgroundColor(lPtr), d,
		       rect->pos.x, rect->pos.y,
		       rect->size.width, rect->size.height);
    
    if (item->uflags)
    {
	W_PaintText(view, d, scr->boldFont, 
		    rect->pos.x+4, rect->pos.y, rect->size.width,
		    WALeft, WMBlackColor(scr), False,
		    item->text, strlen(item->text));
    }
    else
    {
	W_PaintText(view, d, scr->normalFont, 
		    rect->pos.x+4, rect->pos.y, rect->size.width,
		    WALeft, WMBlackColor(scr), False,
		    item->text, strlen(item->text));
    }
}
