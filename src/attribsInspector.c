/* attribsInspector.c */

#include <time.h>
#include <WINGs/WINGsP.h>
#include <grp.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

#include "FSViewer.h"
#include "FSUtils.h"
#include "timestampWidget.h"

/* dimensions of Panel */
#define WIDTH               272
#define HEIGHT              272
/* dimensions of PermissionsFrame */
#define PERM_WIDTH	    180
#define PERM_HEIGHT          92
#define LABEL_HEIGHT         16
#define LABEL_WIDTH          48
#define CHECK_BUTTON_WIDTH   43
#define CHECK_BUTTON_HEIGHT  16

static char *CHECK_BUTTON_OFF[] = {
"%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%",
"%.........................................%",
"%.........................................%",
"%.........................................%",
"%............... #...... #%...............%",
"%................ #.... #%................%",
"%................. #.. #%.................%",
"%...................#%#%..................%",
"%................... #%...................%",
"%.................. #%#...................%",
"%................. #%. #..................%",
"%................ #%... #.................%",
"%................#%..... #................%",
"%.........................................%",
"%.........................................%",
"%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%"};

static char *CHECK_BUTTON_ON[] = {
"%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%",
"%.........................................%",
"%.........................................%",
"%.........................................%",
"%........................ #...............%",
"%....................... #%...............%",
"%...................... #%................%",
"%................. #.. #%.................%",
"%................. #% #%..................%",
"%................. % #%...................%",
"%.................  #%....................%",
"%................. #%.....................%",
"%.................#%......................%",
"%.........................................%",
"%.........................................%",
"%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%"};

typedef struct _Panel {
    WMFrame      *frame;
    char         *sectionName;
    
    CallbackRec   callbacks;

    WMWindow     *win;
    TimeStamp    *stamp;

    WMFrame      *timeF;
    WMFrame      *permF;

    WMButton     *permBtn[3][3];
    WMButton     *okBtn;
    WMButton     *revertBtn;

    WMLabel      *linkToLabel; /*Color changes grey/black if file is link*/
    WMLabel      *linkLabel;
    WMLabel      *sizeLabel;
    WMLabel      *ownerLabel;
    WMLabel      *groupLabel;

    WMPixmap     *offPixmap;
    WMPixmap     *onPixmap;

    FileInfo     *fileInfo;
    mode_t        currentPerm;

} _Panel;

/* transfer permission data from memory to panel buttons */
static void
showPermissions(_Panel *panel)
{
    
    if (panel->currentPerm & S_IRUSR)
	WMSetButtonSelected(panel->permBtn[0][0], 1);
    else
	WMSetButtonSelected(panel->permBtn[0][0], 0);

    if (panel->currentPerm & S_IWUSR)
	WMSetButtonSelected(panel->permBtn[1][0], 1);
    else
	WMSetButtonSelected(panel->permBtn[1][0], 0);
    if (panel->currentPerm & S_IXUSR)
	WMSetButtonSelected(panel->permBtn[2][0], 1);
    else
	WMSetButtonSelected(panel->permBtn[2][0], 0);
    if (panel->currentPerm & S_IRGRP)
	WMSetButtonSelected(panel->permBtn[0][1], 1);
    else
	WMSetButtonSelected(panel->permBtn[0][1], 0);
    if (panel->currentPerm & S_IWGRP)
	WMSetButtonSelected(panel->permBtn[1][1], 1);
    else
	WMSetButtonSelected(panel->permBtn[1][1], 0);
    if (panel->currentPerm & S_IXGRP)
	WMSetButtonSelected(panel->permBtn[2][1], 1);
    else
	WMSetButtonSelected(panel->permBtn[2][1], 0);
    if (panel->currentPerm & S_IROTH)
	WMSetButtonSelected(panel->permBtn[0][2], 1);
    else
	WMSetButtonSelected(panel->permBtn[0][2], 0);
    if (panel->currentPerm & S_IWOTH)
	WMSetButtonSelected(panel->permBtn[1][2], 1);
    else
	WMSetButtonSelected(panel->permBtn[1][2], 0);
    if (panel->currentPerm & S_IXOTH)
	WMSetButtonSelected(panel->permBtn[2][2], 1);
    else
	WMSetButtonSelected(panel->permBtn[2][2], 0);

}

/* store permission data from file to memory */
static void
setPermissions(_Panel *panel, struct stat *st)
{
    panel->currentPerm &= 0;

    if (st->st_mode & S_IRUSR)
	 panel->currentPerm |= S_IRUSR;
    if (st->st_mode & S_IWUSR)
	 panel->currentPerm |= S_IWUSR;
    if (st->st_mode & S_IXUSR)
	 panel->currentPerm |= S_IXUSR;
    if (st->st_mode & S_IRGRP)
	 panel->currentPerm |= S_IRGRP;
    if (st->st_mode & S_IWGRP)
	 panel->currentPerm |= S_IWGRP;
    if (st->st_mode & S_IXGRP)
	 panel->currentPerm |= S_IXGRP;
    if (st->st_mode & S_IROTH)
	 panel->currentPerm |= S_IROTH;
    if (st->st_mode & S_IWOTH)
	 panel->currentPerm |= S_IWOTH;
    if (st->st_mode & S_IXOTH)
	 panel->currentPerm |= S_IXOTH;

    showPermissions(panel);
}

/* read permissions */
static void
showData(_Panel *panel)
{
    char  buf[100];
    char *pathname;
    struct stat   *st;
    struct passwd *psswd;
    struct group  *grp;
    int count;

    st = (struct stat *) wmalloc(sizeof(struct stat));
    
    pathname = (char *) wmalloc(strlen(panel->fileInfo->path)+
				strlen(panel->fileInfo->name)+1);
    strcpy(pathname, panel->fileInfo->path);
    if (panel->fileInfo->fileType != ROOT)
	strcat(pathname, panel->fileInfo->name);

    /* get  information of a file represented by pathname */
    /* lstat shows the link information (was: stat)*/
    if (lstat(pathname, st) == -1)
	wwarning(_("%s %d: lstat Error"), __FILE__, __LINE__);

    if (CLK)
	SetTimeStampWithTimeT(panel->stamp, (int) st->st_mtime);
    /* should only be called first time, not with revert button */  
    setPermissions(panel, st);
    
    grp = getgrgid(st->st_gid);
    if (grp)
	WMSetLabelText(panel->groupLabel, grp->gr_name);

    psswd = getpwuid(st->st_uid);
    if (psswd)
	WMSetLabelText(panel->ownerLabel, psswd->pw_name);    

    /* Directory sizes have to be calculated
    if (S_ISLNK(st->st_mode))
      WMSetLabelText(panel->sizeLabel, "");
    else {
    */
    if (st->st_size >= 10240) {           /* 10240 Bytes -> 10.0 KB */
      if (st->st_size > (1024*1024)) {
        double fsize = st->st_size/(1024.0*1024.0);
        sprintf(buf, _("%'.1lf MB\n"), fsize);
      }
      else {
        double fsize = st->st_size/1024.0;
        sprintf(buf, _("%'.1lf KB\n"), fsize);
      }
    }
    else
      sprintf(buf, _("%d Bytes\n"), (int)st->st_size);
    WMSetLabelText(panel->sizeLabel, buf);
    /* } */

    if (S_ISLNK(st->st_mode)) {
      count = readlink(pathname, buf, sizeof(buf));
      if (count != -1) {
        if (count >= 100) 
	  count = 100-1;
	/* terminate pathname string */
	buf[count] = 0;
	WMSetLabelText(panel->linkLabel, buf);
	/* Set text color to black */
        WMSetLabelTextColor(panel->linkToLabel,
          WMBlackColor(WMWidgetScreen(panel->win)));
	/* what to do with very long pathnames (wrap?) */
      }
    }
    else {
      /* linkLabel keeps text when inspector rereads with Alt-i */
      WMSetLabelText(panel->linkLabel, "");
      /* Set text color to darkgrey */
      WMSetLabelTextColor(panel->linkToLabel,
        WMDarkGrayColor(WMWidgetScreen(panel->win)));
    }
    if (st)
    {
	free(st);
	st = NULL;
    }

    if (pathname)
	free(pathname);

}

/* transfer permission data from panel to memory */
static void
storeData(_Panel *panel)
{
    char buf[100];
    char *pathname;
    struct stat *st;
    mode_t permissions;
    int seterror;
    
    st = (struct stat *) wmalloc(sizeof(struct stat));
    pathname = (char *) wmalloc(strlen(panel->fileInfo->path)+
				strlen(panel->fileInfo->name)+1);
    
    strcpy(pathname, panel->fileInfo->path);
    if (panel->fileInfo->fileType != ROOT)
	strcat(pathname, panel->fileInfo->name);
    permissions = 0; 
    
    /* User Permission */
    if (WMGetButtonSelected(panel->permBtn[0][0]))
	permissions = permissions | S_IRUSR;
    if (WMGetButtonSelected(panel->permBtn[1][0]))
	permissions = permissions | S_IWUSR;
    if (WMGetButtonSelected(panel->permBtn[2][0]))
	permissions = permissions | S_IXUSR;
    /* Group Permissions */
    if (WMGetButtonSelected(panel->permBtn[0][1]))
	permissions = permissions | S_IRGRP;
    if (WMGetButtonSelected(panel->permBtn[1][1]))
	permissions = permissions | S_IWGRP;
    if (WMGetButtonSelected(panel->permBtn[2][1]))
	permissions = permissions | S_IXGRP;
    /* Other permissions */
    if (WMGetButtonSelected(panel->permBtn[0][2]))
	permissions = permissions | S_IROTH;
    if (WMGetButtonSelected(panel->permBtn[1][2]))
	permissions = permissions | S_IWOTH;
    if (WMGetButtonSelected(panel->permBtn[2][2]))
	permissions = permissions | S_IXOTH;
    
    /* now, set the permissions for the file */
    errno = chmod(pathname, permissions); 
    
    free(pathname);	
}

/* Panel-Design - Label section

Layout:     | | Label    | | Label                 | |
Size [px]:  |4| adjusted |4| WIDTH - 12 - adjusted |4|
          ->| |          | |                       | |<- = WIDTH

Labels on the left side fit to the text width of the largest text,
to make translations easier.
*/

static void
createLabels(_Panel *panel)
{
    WMLabel *sl, *ol, *gl;
    WMScreen *scr;
    int tw, maxLW = 0;

    /* LinkToLabel */
    panel->linkToLabel = WMCreateLabel(panel->frame);
    WMMoveWidget(panel->linkToLabel, 6, 18);
    WMSetLabelText(panel->linkToLabel, _("Link To:"));
    WMSetLabelTextAlignment(panel->linkToLabel, WARight);
    WMSetLabelRelief(panel->linkToLabel, WRFlat);
    WMSetLabelTextColor(panel->linkToLabel,
      WMDarkGrayColor(WMWidgetScreen(panel->win)));

    scr = WMWidgetScreen(panel->win);
    if (scr->normalFont) { 
      tw = WMWidthOfString(scr->normalFont,
        WMGetLabelText(panel->linkToLabel),
        strlen(WMGetLabelText(panel->linkToLabel)));
    }
    else {
      tw = LABEL_WIDTH;
    }
    maxLW = tw;

    /* SizeLabel */
    sl = WMCreateLabel(panel->frame);
    WMMoveWidget(sl, 6, 56);
    WMSetLabelText(sl, _("Size:"));
    WMSetLabelTextAlignment(sl, WARight);
    WMSetLabelRelief(sl, WRFlat);
    if (scr->normalFont) { 
      tw = WMWidthOfString(scr->normalFont, WMGetLabelText(sl),
        strlen(WMGetLabelText(sl)));
    }
    else {
      tw = LABEL_WIDTH;
    }
    if (tw > maxLW) { maxLW =tw; }

    /* OwnerLabel */
    ol = WMCreateLabel(panel->frame);
    WMMoveWidget(ol, 6, 86);
    WMSetLabelText(ol, _("Owner:"));
    WMSetLabelTextAlignment(ol, WARight);
    WMSetLabelRelief(ol, WRFlat);
    if (scr->normalFont) { 
      tw = WMWidthOfString(scr->normalFont, WMGetLabelText(ol),
        strlen(WMGetLabelText(ol)));
    }
    else {
      tw = LABEL_WIDTH;
    }
    if (tw > maxLW) { maxLW = tw; }

    /* GroupLabel */
    gl = WMCreateLabel(panel->frame);
    WMMoveWidget(gl, 6, 116);
    WMSetLabelText(gl, _("Group:"));
    WMSetLabelTextAlignment(gl, WARight);
    WMSetLabelRelief(gl, WRFlat);
    if (scr->normalFont) { 
      tw = WMWidthOfString(scr->normalFont, WMGetLabelText(gl),
        strlen(WMGetLabelText(gl)));
    }
    else {
      tw = LABEL_WIDTH;
    }
    if (tw > maxLW) { maxLW = tw; }
    /* now we have the real maxLW */
    maxLW+=4;
    WMResizeWidget(panel->linkToLabel, maxLW, LABEL_HEIGHT);
    WMResizeWidget(sl, maxLW, LABEL_HEIGHT);
    WMResizeWidget(ol, maxLW, LABEL_HEIGHT);
    WMResizeWidget(gl, maxLW, LABEL_HEIGHT);

    /* the text of this label is set in "showData()" */
    panel->linkLabel = WMCreateLabel(panel->frame);
    WMSetLabelWraps(panel->linkLabel, 1);
    /* this label needs space for wraping long pathnames */
    WMResizeWidget(panel->linkLabel, WIDTH-maxLW-12, LABEL_HEIGHT*3);
    WMMoveWidget(panel->linkLabel, maxLW+4, 2);
    WMSetLabelRelief(panel->linkLabel, WRFlat);

    panel->sizeLabel = WMCreateLabel(panel->frame);
    WMResizeWidget(panel->sizeLabel, PERM_WIDTH-maxLW, LABEL_HEIGHT);
    WMMoveWidget(panel->sizeLabel, maxLW+4, 56);
    WMSetLabelRelief(panel->sizeLabel, WRFlat);

    panel->ownerLabel = WMCreateLabel(panel->frame);
    WMResizeWidget(panel->ownerLabel, PERM_WIDTH-maxLW, LABEL_HEIGHT);
    WMMoveWidget(panel->ownerLabel, maxLW+4, 86);
    WMSetLabelRelief(panel->ownerLabel, WRFlat);

    panel->groupLabel = WMCreateLabel(panel->frame);
    WMResizeWidget(panel->groupLabel, PERM_WIDTH-maxLW, LABEL_HEIGHT);
    WMMoveWidget(panel->groupLabel, maxLW+4, 116);
    WMSetLabelRelief(panel->groupLabel, WRFlat);
    
}

static void
buttonClick(WMWidget *self, void *data)
{
    _Panel *panel = (_Panel *)data;

    /*  okButton sets the new file attributes */
    if ((WMButton *)self == panel->okBtn)
    {
	storeData(panel);
 	WMSetButtonEnabled(panel->okBtn, False); 
 	WMSetButtonEnabled(panel->revertBtn, True);
    }	
    /*  revertButton resets the old file attributes */
    else {    /* revert button clicked */
      if ((WMButton *)self == panel->revertBtn) {
	showPermissions(panel);
	storeData(panel);
 	WMSetButtonEnabled(panel->okBtn, False); 
 	WMSetButtonEnabled(panel->revertBtn, False); 
      }
    /*  permission buttons only aktivate the okButton */
      else {  /* permission button clicked */
 	WMSetButtonEnabled(panel->okBtn, True); 
      }
    }
}

static void
fillPermissionsFrame(WMScreen *scr, _Panel *panel, int x, int y, 
		     int row, int col)
{
    WMButton *btn;
    WMLabel *l;
    int i;
    int j;

    char *label[] = { _("Read"), _("Write"), _("Exec"), 
		      _("Owner"), _("Group"), _("Other") };

    for(i=0; i < row; i++)
	for(j=0; j < col; j++)
	{
	    btn = WMCreateSwitchButton(panel->permF);
	    WMSetButtonImage(btn, panel->offPixmap);
	    WMSetButtonAltImage(btn, panel->onPixmap);
	    
	    WMSetButtonImagePosition(btn, WIPImageOnly);
	    WMSetButtonBordered(btn, False);
	    WMSetButtonEnabled(btn, True);
    	    WMSetButtonAction(btn, buttonClick, panel);   
	    WMResizeWidget(btn, 43, LABEL_HEIGHT);
	    WMMoveWidget(btn, 0+43+(j*(43-1)), 
			 y+(i*(LABEL_HEIGHT-1)));

	    panel->permBtn[i][j] = btn;
	}
    
    for(i = 0; i < row; i++)
    {
	l = WMCreateLabel(panel->permF);
	WMResizeWidget(l, 34, LABEL_HEIGHT);
	WMMoveWidget(l, x, y+(LABEL_HEIGHT*i));
	WMSetLabelText(l, label[i]);
	WMSetLabelTextAlignment(l, WARight);
	WMSetLabelRelief(l, WRFlat);
    }

    for(i = 0; i < col; i++)
    {
	l = WMCreateLabel(panel->permF);
	WMResizeWidget(l, 42, LABEL_HEIGHT);
	WMMoveWidget(l, 1+42+(i*43), y+(LABEL_HEIGHT*row));
	WMSetLabelText(l, label[i+3]);
	WMSetLabelTextAlignment(l, WACenter);
	WMSetLabelRelief(l, WRFlat);
    }

}

static void
createPermissionsFrame(_Panel *panel)
{

    panel->permF = WMCreateFrame(panel->frame);
    WMResizeWidget(panel->permF, PERM_WIDTH, PERM_HEIGHT);
    WMMoveWidget(panel->permF, 8, HEIGHT-33-PERM_HEIGHT);
    WMSetFrameTitle(panel->permF, _(" Permissions "));
    fillPermissionsFrame(WMWidgetScreen(panel->win), panel, 6, 20, 3, 3);

}
static void
createChangedFrame(_Panel *panel)
{

    panel->timeF = WMCreateFrame(panel->frame);
    WMResizeWidget(panel->timeF, 68, PERM_HEIGHT);
    WMMoveWidget(panel->timeF, WIDTH-76, HEIGHT-33-PERM_HEIGHT);
    WMSetFrameTitle(panel->timeF, _(" Changed "));

    if (CLK)
    {
	InitTimeStamp(WMWidgetScreen(panel->win));
	panel->stamp = CreateTimeStamp(panel->timeF);
	WMResizeWidget(panel->stamp, 64, 64);
	WMMoveWidget(panel->stamp, 2, 14);
	SetTwentyFour(panel->stamp, True);
    }

}

static void
createButtons(_Panel *panel)
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
    WMSetButtonText(panel->okBtn, _("OK"));
    WMSetButtonImage(panel->okBtn, 
		     WMGetSystemPixmap(WMWidgetScreen(panel->win), 
				       WSIReturnArrow));
    WMSetButtonAltImage(panel->okBtn, 
			WMGetSystemPixmap(WMWidgetScreen(panel->win), 
					  WSIHighlightedReturnArrow));
    WMSetButtonImagePosition(panel->okBtn, WIPRight);
    WMSetButtonEnabled(panel->okBtn, False);
    WMSetButtonAction(panel->okBtn, buttonClick, panel);   
}

static void
createPanel(_Panel *p)
{
    _Panel *panel = (_Panel*)p;
    panel->frame = WMCreateFrame(panel->win);

    WMResizeWidget(panel->frame, WIDTH, HEIGHT);
    WMMoveWidget(panel->frame, 0, 138);
    WMSetFrameRelief(panel->frame, WRFlat);

    createChangedFrame(panel);
    createPermissionsFrame(panel);
    createLabels(panel);
    createButtons(panel);

    WMRealizeWidget(panel->frame);
    WMMapSubwidgets(panel->frame);
    WMMapSubwidgets(panel->timeF);
    WMMapSubwidgets(panel->permF);

}



_Panel*
InitAttribs(WMWindow *win, FileInfo *fileInfo)
{
    _Panel *panel;

    panel = wmalloc(sizeof(_Panel));
    memset(panel, 0, sizeof(_Panel));

    panel->sectionName = (char *) wmalloc(strlen(_("Attributes Inspector"))+1);
    strcpy(panel->sectionName, _("Attributes Inspector"));

    panel->win = win;

    panel->offPixmap = FSMakePixmap(WMWidgetScreen(win), CHECK_BUTTON_OFF,
				    CHECK_BUTTON_WIDTH,
				    CHECK_BUTTON_HEIGHT);
    panel->onPixmap  = FSMakePixmap(WMWidgetScreen(win), CHECK_BUTTON_ON,
				    CHECK_BUTTON_WIDTH,
				    CHECK_BUTTON_HEIGHT);
    
    panel->callbacks.createWidgets = createPanel;
    panel->callbacks.updateDomain = storeData;
    panel->callbacks.updateDisplay = showData;
    panel->fileInfo = fileInfo;

    return panel;
}
