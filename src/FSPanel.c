// FSPanel.c 

#include <X11/Xatom.h>
#include <X11/keysym.h>
#include <WINGs/WINGsP.h>
#include "FSViewer.h"
#include "FSUtils.h"
#include "FSPanel.h"

static	FSSelectIconPanel      *FSCreateSelectIconPanel(WMWindow*, 
							char*, char*);
static	void		       	FSCloseSelectIconPanel(WMWidget*, void*);
static	char 		       *FSDestroySelectIconPanel(FSSelectIconPanel*);
static  void                    selIconButtonClick(WMWidget *self, void *data);
static  void                    setIconLabel(WMWidget *self, void *data);
static  void                    fillIconFileList(WMWidget *self, void *data);
static  char                   *getSelectedFilename(FSSelectIconPanel *panel);

static	FSInfoPanel	       *FSCreateInfoPanel(FSViewer*, char*, char*);
static	void		       	FSCloseInfoPanel(WMWidget*, void*);
static	void		       	FSDestroyInfoPanel(FSInfoPanel*);
static	FSLegalPanel	       *FSCreateLegalPanel(FSViewer*, char*, char*);
static	void		       	FSCloseLegalPanel(WMWidget*, void*);
static	void		       	FSDestroyLegalPanel(FSLegalPanel*);
static  void                    FSUpdateIconSelectPanel(FSIconSelectPanel *, 
							char *);
static	void		       	FSCloseIconSelectPanel(WMWidget*, void*);
static	void		       	FSDestroyIconSelectPanel(FSIconSelectPanel*);
static  FSIconSelectPanel      *FSCreateIconSelectPanel(FSViewer *app, char *, 
							char *extn);
static  void                    FSShowIconSelectPanel(Panel *panel);
/* static	FSPreferencesPanel     *FSCreatePreferencesPanel(FSViewer*, char*); */
/* static  void                    prefsButtonClick(WMWidget *self, void *data); */
/* static  void                    FSSetPreferencesContents(FSPreferencesPanel *prefs); */
/* static  void                    FSStorePreferencesContents(FSPreferencesPanel *prefs); */
/* static	void		       	FSClosePreferencesPanel(WMWidget*, void*); */
/* static	void		       	FSDestroyPreferencesPanel(FSPreferencesPanel*); */
static	FSAppInputPanel	       *FSCreateAppInputPanel(FSViewer*, FileInfo*, 
						      char*);
static	void		       	FSCloseAppInputPanel(WMWidget*, void*);
static	void		       	FSDestroyAppInputPanel(FSAppInputPanel*);
static  void                    buttonClick(WMWidget *self, void *data);
static  char                   *FSStoreAppInputData(FSAppInputPanel *appInput);
static  void                    FSStoreAppInputPropListData(FSAppInputPanel *appInput);
static void                     endedEditingObserver(void *observerData, 
						     WMNotification *notification);
static  FSAlertPanel*           FSCreateAlertPanel(FSViewer *app,
						   char *title, char *msg, 
						   char *defaultButton,
						   char *alternateButton, 
						   char *otherButton);
static void                     FSDestroyAlertPanel(FSAlertPanel *alertPanel);
static void                     handleKeyPress(XEvent *event, 
					       void *clientData);
static void                     alertPanelOnClick(WMWidget *self, 
						  void *clientData);

static	FSIconSelectPanel      *icon        = NULL;
static	FSInfoPanel	       *info        = NULL;
static	FSLegalPanel	       *legal       = NULL;
/* static	FSPreferencesPanel     *preferences = NULL; */
static	FSAppInputPanel        *appInput    = NULL;
static	FSAlertPanel           *alertPanel  = NULL;
static	FSSelectIconPanel      *selIcon     = NULL;

void FSRunInfoPanel(FSViewer *app, char *title, char *msg)
{
    if (!(info = FSCreateInfoPanel(app, title, msg))) { return; }
    
    WMMapWidget(info->win);
    
    while (!(info->done))
    {
	XEvent event;
	
	WMNextEvent(info->dpy, &event);
	WMHandleEvent(&event);
    }
    FSDestroyInfoPanel(info);
}

// [Almost verbatim from WMAlertPanel.]
static FSInfoPanel *
FSCreateInfoPanel(FSViewer *app, char *title, char *msg)
{
    FSInfoPanel	  *info;
    WMPixmap	  *appicon;
    int		   height, offset;
    char	   buf[MAX_LEN + 1];
    WMFont	  *aFont;

    if (!(info = (FSInfoPanel *) malloc(sizeof(FSInfoPanel))))
	{ return NULL; }
    memset((void *) info, 0, sizeof(FSInfoPanel));
    info->app = app;
    info->scr = FSGetFSViewerScreen(app);
    info->dpy = WMScreenDisplay(info->scr);

    height = 250;
    info->win = WMCreateWindow(info->scr, "info");
    WMResizeWidget(info->win, 420, height);
    WMSetWindowTitle(info->win, _("Info"));
    WMSetWindowCloseAction(info->win, FSCloseInfoPanel, (void *) info);
    
    if ((appicon = WMGetApplicationIconPixmap(info->scr)))
    {
	WMSize	appIconSize;
	
	appIconSize = WMGetPixmapSize(appicon);
	info->iconLabel = WMCreateLabel(info->win);
	WMResizeWidget(info->iconLabel, appIconSize.width, appIconSize.height);
	WMMoveWidget(info->iconLabel,
		     // 8 + (64 - appIconSize.width) / 2,
		     // (75 - appIconSize.height) / 2);
		     80 - appIconSize.width - 5,
		     80 - appIconSize.height - 5);
	WMSetLabelImage(info->iconLabel, appicon);
	WMSetLabelImagePosition(info->iconLabel, WIPImageOnly);
    }

    offset = 55;
    if ((title))
    {
	WMFont	*font;
	
	font = WMCreateFont(info->scr, "-*-times-bold-r-*-*-24-*-*-*-*-*-*-*");
	if (!font)
	    font = WMBoldSystemFontOfSize(info->scr, 24);

	info->titleLabel = WMCreateLabel(info->win);
	WMMoveWidget(info->titleLabel, 80, (80 - 24 /*font->height*/) / 2);
	WMResizeWidget(info->titleLabel, 400 - 70,
		       2 * (24 /*font->height*/ + 6));
	WMSetLabelText(info->titleLabel, title);
	WMSetLabelTextAlignment(info->titleLabel, WACenter);
	WMSetLabelFont(info->titleLabel, font);
	offset += (2 * (24 /*font->height*/ + 6));
	WMReleaseFont(font);
    }

    // Author(s) here.
    aFont = WMBoldSystemFontOfSize(info->scr, 12);
    info->author = WMCreateLabel(info->win);
    WMMoveWidget(info->author, 10, height - offset - 40);
    WMResizeWidget(info->author, 400, 20);
    WMSetLabelText(info->author, "Author: George Clernon");
    WMSetLabelTextAlignment(info->author, WACenter);
    WMSetLabelFont(info->author, aFont);
    
    info->frame = WMCreateFrame(info->win);
    WMMoveWidget(info->frame, 10, height - offset - 10);
    WMResizeWidget(info->frame, 400, 90);
    WMSetFrameRelief(info->frame, WRGroove);
    
    info->sysRelease = WMCreateLabel(info->frame);
    WMMoveWidget(info->sysRelease, 10, 10);
    WMResizeWidget(info->sysRelease, 380 / 2 - 10, 20);
    snprintf(buf, MAX_LEN, _("System Release %s"), FSSystemRelease());
    WMSetLabelText(info->sysRelease, buf);
    WMSetLabelTextAlignment(info->sysRelease, WALeft);
    
    info->fsRelease = WMCreateLabel(info->frame);
    WMMoveWidget(info->fsRelease, 10, 35);
    WMResizeWidget(info->fsRelease, 380 / 2 - 10, 20);
    snprintf(buf, MAX_LEN, _("FSViewer Release %s"), VERSION);
    WMSetLabelText(info->fsRelease, buf);
    WMSetLabelTextAlignment(info->fsRelease, WALeft);
    
    info->xRelease = WMCreateLabel(info->frame);
    WMMoveWidget(info->xRelease, 10, 60);
    WMResizeWidget(info->xRelease, 380 / 2 - 10, 20);
    snprintf(buf, MAX_LEN, _("Window Server %d"), VendorRelease(info->dpy));
    WMSetLabelText(info->xRelease, buf);
    WMSetLabelTextAlignment(info->xRelease, WALeft);
    
    info->machine = WMCreateLabel(info->frame);
    WMMoveWidget(info->machine, 380 / 2 + 10, 10);
    WMResizeWidget(info->machine, 380 / 2, 20);
    snprintf(buf, MAX_LEN, _("Processor: %s"), FSProcessor());
    WMSetLabelText(info->machine, buf);
    WMSetLabelTextAlignment(info->machine, WARight);
    
    info->memSize = WMCreateLabel(info->frame);
    WMMoveWidget(info->memSize, 380 / 2 + 10, 35);
    WMResizeWidget(info->memSize, 380 / 2, 20);
    snprintf(buf, MAX_LEN, _("Memory: %s"), FSMemory());
    WMSetLabelText(info->memSize, buf);
    WMSetLabelTextAlignment(info->memSize, WARight);
    
    info->discSize = WMCreateLabel(info->frame);
    WMMoveWidget(info->discSize, 380 / 2 + 10, 60);
    WMResizeWidget(info->discSize, 380 / 2, 20);
    snprintf(buf, MAX_LEN, _("Disk: %s"), FSDisk());
    WMSetLabelText(info->discSize, buf);
    WMSetLabelTextAlignment(info->discSize, WARight);
    
    info->copyright = WMCreateLabel(info->win);
    WMMoveWidget(info->copyright, 10, height - 30);
    WMResizeWidget(info->copyright, 400, 20);
    WMSetLabelText(info->copyright, "Copyright 1998, 1999 George Clernon");
    WMSetLabelTextAlignment(info->copyright, WACenter);
    WMSetLabelFont(info->copyright, aFont);
    WMSetLabelTextColor(info->copyright, WMDarkGrayColor(info->scr));
    
    WMMapSubwidgets(info->frame);
    
    // Copyright here. DarkGray.
    
    info->done = 0;
    WMRealizeWidget(info->win);
    FSSetFSViewerTransientWindow(info->app, WMWidgetXID(info->win));
    WMMapSubwidgets(info->win);
    WMReleaseFont(aFont);

    return info;
}

static void FSCloseInfoPanel(WMWidget *self, void *client)
{
    FSInfoPanel	*info = (FSInfoPanel *) client;
    
    info->done = 1;
}

static void FSDestroyInfoPanel(FSInfoPanel *info)
{
    WMUnmapWidget(info->win);
    WMDestroyWidget(info->win);
    free((void *) info);
    info = NULL;
}

void FSRunLegalPanel(FSViewer *app, char *title, char *msg)
{
    if (!(legal = FSCreateLegalPanel(app, title, msg))) 
	return;
    
    WMMapWidget(legal->win);
    
    while (!(legal->done))
    {
	XEvent event;
	
	WMNextEvent(legal->dpy, &event);
	WMHandleEvent(&event);
    }
    FSDestroyLegalPanel(legal);
}

#define	License	\
        "   FSViewer is free software; you can redistribute it and/or modify "\
	"it under the terms of the GNU General Public License as published "\
	"by the Free Software Foundation; either version 2 of the License, "\
	"or (at your option) any later version.\n\n"\
	"   FSViewer is distributed in the hope that it will be useful, but "\
	"WITHOUT ANY WARRANTY; without even the implied warranty of "\
	"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU "\
	"General Public License for more details.\n\n"\
	"   You should have received a copy of the GNU General Public License "\
	"along with this program; if not, write to the Free Software "\
	"Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA "\
	"02111-1307, USA."

// [Needs work on the layout. Probably need an array of labels here for
// different fonts/sizes and justifications];
static FSLegalPanel *FSCreateLegalPanel(FSViewer *app, char *title, char *msg)
{
    FSLegalPanel  *legal;

    if (!(legal = (FSLegalPanel *) malloc(sizeof(FSLegalPanel))))
	return NULL;
    memset((void *) legal, 0, sizeof(FSLegalPanel));
    
    legal->app = app;
    legal->scr = FSGetFSViewerScreen(app);
    legal->dpy = WMScreenDisplay(legal->scr);
    
    legal->win = WMCreateWindow(legal->scr, "legal");
    WMResizeWidget(legal->win, 420, 250);
    WMSetWindowTitle(legal->win, _("Legal"));
    WMSetWindowCloseAction(legal->win, FSCloseLegalPanel, (void *) legal);
    
    legal->license = WMCreateLabel(legal->win);
    WMSetLabelWraps(legal->license, 1);
    WMResizeWidget(legal->license, WMWidgetWidth(legal->win) - 20,
		   WMWidgetHeight(legal->win) - 20);
    WMMoveWidget(legal->license, 10, 10);
    WMSetLabelTextAlignment(legal->license, WALeft);
    WMSetLabelText(legal->license, License);
    WMSetLabelRelief(legal->license, WRGroove);
    
    legal->done = 0;
    WMRealizeWidget(legal->win);
    
    FSSetFSViewerTransientWindow(legal->app, WMWidgetXID(legal->win));
    WMMapSubwidgets(legal->win);
    return legal;
}

static void FSCloseLegalPanel(WMWidget *self, void *client)
{
    FSLegalPanel	*legal = (FSLegalPanel *) client;
    
    legal->done = 1;
}

static void FSDestroyLegalPanel(FSLegalPanel *legal)
{
    WMUnmapWidget(legal->win);
    WMDestroyWidget(legal->win);
    free((void *) legal);
    legal = NULL;
}

/* void FSRunPreferencesPanel(FSViewer *app, char *title) */
/* {  */
/*     if (!(preferences = FSCreatePreferencesPanel(app, title))) { return; } */
    
/*     WMMapWidget(preferences->win); */
    
/*     while (!(preferences->done)) */
/*     { */
/* 	XEvent event; */
	
/* 	WMNextEvent(preferences->dpy, &event); */
/* 	WMHandleEvent(&event); */
/*     } */
/*     FSDestroyPreferencesPanel(preferences); */
/* } */

/* static FSPreferencesPanel *FSCreatePreferencesPanel(FSViewer *app, char *title) */
/* { */
/*     FSPreferencesPanel	  *prefs; */
/*     int		           height, width, offset; */
/*     WMPixmap              *appicon; */
/*     WMFrame               *f; */
/*     WMLabel               *l; */

/*     if(!(prefs=(FSPreferencesPanel *)malloc(sizeof(FSPreferencesPanel)))) */
/* 	{ return NULL; } */
/*     memset((void *) prefs, 0, sizeof(FSPreferencesPanel)); */
/*     prefs->app = app; */
/*     prefs->scr = FSGetFSViewerScreen(app); */
/*     prefs->dpy = WMScreenDisplay(prefs->scr); */

/*     height = 422; */
/*     width  = 272; */
/*     prefs->win = WMCreateWindow(prefs->scr, "prefs"); */
/*     WMResizeWidget(prefs->win, width, height); */
/*     WMSetWindowTitle(prefs->win, title); */
/*     WMSetWindowCloseAction(prefs->win, FSClosePreferencesPanel,  */
/* 			   (void *) prefs); */
    
/*     prefs->frame = WMCreateFrame(prefs->win); */
/*     WMResizeWidget(prefs->frame, 282, 95); */
/*     WMMoveWidget(prefs->frame, -5, -5); */
/*     WMSetFrameRelief(prefs->frame, WRGroove); */

/*     if ((appicon = WMGetApplicationIconImage(prefs->scr))) */
/*     { */
/* 	WMSize	appIconSize; */
	
/* 	appIconSize = WMGetPixmapSize(appicon); */
/* 	prefs->iconLabel = WMCreateLabel(prefs->frame); */
/* 	WMResizeWidget(prefs->iconLabel,  */
/* 		       appIconSize.width, appIconSize.height); */
/* 	WMMoveWidget(prefs->iconLabel, */
		     // 8 + (64 - appIconSize.width) / 2,
		     // (75 - appIconSize.height) / 2);
/* 		     85 - appIconSize.width - 5, */
/* 		     85 - appIconSize.height - 5); */
/* 	WMSetLabelImage(prefs->iconLabel, appicon); */
/* 	WMSetLabelImagePosition(prefs->iconLabel, WIPImageOnly); */
/*     } */

/*     offset = 55; */
/*     if ((title)) */
/*     { */
/* 	WMFont	*font; */
	
/* 	font = WMCreateFont(prefs->scr,"-*-times-bold-r-*-*-24-*-*-*-*-*-*-*"); */
/* 	if (!font) */
/* 	    font = WMBoldSystemFontOfSize(prefs->scr, 24); */

/* 	prefs->titleLabel = WMCreateLabel(prefs->frame); */
/* 	WMMoveWidget(prefs->titleLabel, 85, 20); */
/* 	WMResizeWidget(prefs->titleLabel, width - 70, */
		       /* 2 * (24  *//*font->height*//*  + 6)); */
/* 	WMSetLabelText(prefs->titleLabel, title); */
/* 	WMSetLabelTextAlignment(prefs->titleLabel, WALeft); */
/* 	WMSetLabelFont(prefs->titleLabel, font); */
/* 	offset += (2 * (24  *//*font->height*//*  + 6)); */
/* 	WMReleaseFont(font); */
/*     } */

/*     prefs->dotFileBtn = WMCreateSwitchButton(prefs->win); */
/*     WMMoveWidget(prefs->dotFileBtn, 16, 100); */
/*     WMResizeWidget(prefs->dotFileBtn, 200, 16); */
/*     WMSetButtonEnabled(prefs->dotFileBtn, True); */
/*     WMSetButtonText(prefs->dotFileBtn, "Display Dot Files"); */
    
/*     prefs->scrollviewBtn = WMCreateSwitchButton(prefs->win); */
/*     WMMoveWidget(prefs->scrollviewBtn, 16, 125); */
/*     WMResizeWidget(prefs->scrollviewBtn, 200, 16); */
/*     WMSetButtonEnabled(prefs->scrollviewBtn, True); */
/*     WMSetButtonText(prefs->scrollviewBtn, "Display Scrollview Background"); */
    
/*     l = WMCreateLabel(prefs->win); */
/*     WMResizeWidget(l, 100, 16); */
/*     WMMoveWidget(l, 16, 155); */
/*     WMSetLabelText(l, "Sort Order:"); */
/*     WMSetLabelRelief(l, WRFlat); */
/*     WMSetLabelTextColor(l, WMDarkGrayColor(WMWidgetScreen(prefs->win))); */
/*     WMSetLabelTextAlignment(l, WALeft); */
/*     WMMapWidget(l); */
    
    
/*     f = WMCreateFrame(prefs->win); */
/*     WMResizeWidget(f, 105, 45); */
/*     WMMoveWidget(f, 40, 175); */
/*     WMSetFrameRelief(f, WRGroove); */

/*     prefs->orderBtn[0] = WMCreateRadioButton(f); */
/*     WMResizeWidget(prefs->orderBtn[0], 90, 16); */
/*     WMMoveWidget(prefs->orderBtn[0], 5, 5); */
/*     WMSetButtonEnabled(prefs->orderBtn[0], True); */
/*     WMSetButtonText(prefs->orderBtn[0], "Descending"); */

/*     prefs->orderBtn[1] = WMCreateRadioButton(f); */
/*     WMResizeWidget(prefs->orderBtn[1], 90, 16); */
/*     WMMoveWidget(prefs->orderBtn[1], 5, 25); */
/*     WMSetButtonEnabled(prefs->orderBtn[1], True); */
/*     WMSetButtonText(prefs->orderBtn[1], "Ascending"); */

/*     WMGroupButtons(prefs->orderBtn[0], prefs->orderBtn[1]); */
/*     WMMapSubwidgets(f); */
   
/*     l = WMCreateLabel(prefs->win); */
/*     WMResizeWidget(l, 100, 16); */
/*     WMMoveWidget(l, 16, 230); */
/*     WMSetLabelText(l, "Sort Display:"); */
/*     WMSetLabelRelief(l, WRFlat); */
/*     WMSetLabelTextColor(l, WMDarkGrayColor(WMWidgetScreen(prefs->win))); */
/*     WMSetLabelTextAlignment(l, WALeft); */
/*     WMMapWidget(l); */
    
/*     f = WMCreateFrame(prefs->win); */
/*     WMResizeWidget(f, 130, 65); */
/*     WMMoveWidget(f, 40, 250); */
/*     WMSetFrameRelief(f, WRGroove); */

/*     prefs->sortBtn[0] = WMCreateRadioButton(f); */
/*     WMResizeWidget(prefs->sortBtn[0], 120, 16); */
/*     WMMoveWidget(prefs->sortBtn[0], 5, 5); */
/*     WMSetButtonEnabled(prefs->sortBtn[0], True); */
/*     WMSetButtonText(prefs->sortBtn[0], "Mixed"); */

/*     prefs->sortBtn[1] = WMCreateRadioButton(f); */
/*     WMResizeWidget(prefs->sortBtn[1], 120, 16); */
/*     WMMoveWidget(prefs->sortBtn[1], 5, 25); */
/*     WMSetButtonEnabled(prefs->sortBtn[1], True); */
/*     WMSetButtonText(prefs->sortBtn[1], "Files First"); */

/*     prefs->sortBtn[2] = WMCreateRadioButton(f); */
/*     WMResizeWidget(prefs->sortBtn[2], 120, 16); */
/*     WMMoveWidget(prefs->sortBtn[2], 5, 45); */
/*     WMSetButtonEnabled(prefs->sortBtn[2], True); */
/*     WMSetButtonText(prefs->sortBtn[2], "Directories First"); */

/*     WMGroupButtons(prefs->sortBtn[0], prefs->sortBtn[1]); */
/*     WMGroupButtons(prefs->sortBtn[0], prefs->sortBtn[2]); */
/*     WMMapSubwidgets(f); */
   
/*     l = WMCreateLabel(prefs->win); */
/*     WMResizeWidget(l, 200, 16); */
/*     WMMoveWidget(l, 16, 325); */
/*     WMSetLabelText(l, "Icon Resuource Directory:"); */
/*     WMSetLabelRelief(l, WRFlat); */
/*     WMSetLabelTextColor(l, WMDarkGrayColor(WMWidgetScreen(prefs->win))); */
/*     WMSetLabelTextAlignment(l, WALeft); */
/*     WMMapWidget(l); */

/*     prefs->iconDirField = WMCreateTextField(prefs->win); */
/*     WMMoveWidget(prefs->iconDirField, 40, 345); */
/*     WMResizeWidget(prefs->iconDirField, 216, 20); */

/*     prefs->revertBtn = WMCreateCommandButton(prefs->win); */
/*     WMMoveWidget(prefs->revertBtn, 16, height-35); */
/*     WMResizeWidget(prefs->revertBtn, 115, 24); */
/*     WMSetButtonText(prefs->revertBtn, "Revert"); */
/*     WMSetButtonEnabled(prefs->revertBtn, True); */
/*     WMSetButtonAction(prefs->revertBtn, prefsButtonClick, prefs);    */
  
/*     prefs->okBtn = WMCreateCommandButton(prefs->win); */
/*     WMMoveWidget(prefs->okBtn, 141, height-35); */
/*     WMResizeWidget(prefs->okBtn, 115, 24); */
/*     WMSetButtonText(prefs->okBtn, "OK"); */
/*     WMSetButtonEnabled(prefs->okBtn, True); */
/*     WMSetButtonAction(prefs->okBtn, prefsButtonClick, prefs);    */

/*     FSSetPreferencesContents(prefs); */
/*     prefs->done = 0; */
/*     WMRealizeWidget(prefs->win); */
    
/*     FSSetFSViewerTransientWindow(prefs->app, WMWidgetXID(prefs->win)); */
/*     WMMapSubwidgets(prefs->frame); */
/*     WMMapSubwidgets(prefs->win); */

/*     return prefs; */
/* } */

/* static void */
/* FSStorePreferencesContents(FSPreferencesPanel *prefs) */
/* { */
/*     int i = 0; */
/*     char *txt = NULL; */

/*     txt = WMGetTextFieldText(prefs->iconDirField); */
/*     if(txt) */
/*     { */
/* 	FSSetStringForName("ICONDIR", txt); */
/* 	WMSetResourcePath(txt); */
/* 	free(txt); */
/*     } */

/*     for(i = 0; i < 3; i++) */
/* 	if( WMGetButtonSelected(prefs->sortBtn[i]) ) */
/* 	{ */
/* 	   FSSetIntegerForName("SortDisplay", i); */
/* 	   break; */
/* 	} */
    
/*     for(i = 0; i < 2; i++) */
/* 	if( WMGetButtonSelected(prefs->orderBtn[i]) ) */
/* 	{ */
/* 	   FSSetIntegerForName("SortOrder", i); */
/* 	   break; */
/* 	} */

/*     if( WMGetButtonSelected(prefs->dotFileBtn) ) */
/* 	FSSetIntegerForName("DisplayDotFiles", 1); */
/*     else */
/* 	FSSetIntegerForName("DisplayDotFiles", 0); */
	
/*     if( WMGetButtonSelected(prefs->scrollviewBtn) ) */
/* 	FSSetIntegerForName("DisplaySVBG", 1);     */
/*     else */
/* 	FSSetIntegerForName("DisplaySVBG", 0); */
/* } */

/* static void */
/* FSSetPreferencesContents(FSPreferencesPanel *prefs) */
/* { */
/*     int index = 0; */

/*     WMSetTextFieldText( prefs->iconDirField, FSGetStringForName("ICONDIR") ); */

/*     index = FSGetIntegerForName("SortDisplay"); */
/*     WMPerformButtonClick(prefs->sortBtn[index]); */

/*     index = FSGetIntegerForName("SortOrder"); */
/*     WMPerformButtonClick(prefs->orderBtn[index]); */

/*     index = FSGetIntegerForName("DisplayDotFiles"); */
/*     WMSetButtonSelected(prefs->dotFileBtn, index);      */

/*     index = FSGetIntegerForName("DisplaySVBG"); */
/*     WMSetButtonSelected(prefs->scrollviewBtn, index);      */
/* } */

/* static void */
/* prefsButtonClick(WMWidget *self, void *data) */
/* { */
/*     FSPreferencesPanel *prefs = (FSPreferencesPanel *)data; */

/*     if( (WMButton *)self == prefs->okBtn ) */
/*     { */
/* 	FSStorePreferencesContents(prefs); */
/* 	FSClosePreferencesPanel(self, prefs); */
/* 	FSUpdateFileViewPath(FSGetFSViewerCurrentView(prefs->app));	 */
/*     } */
/*     else if( (WMButton *)self == prefs->revertBtn ) */
/*     { */
/* 	FSSetPreferencesContents(prefs); */
/*     } */
/* }     */

/* static void FSClosePreferencesPanel(WMWidget *self, void *client) */
/* { */
/*     FSPreferencesPanel	*prefs = (FSPreferencesPanel *) client; */
    
/*     prefs->done = 1; */
/* } */

/* static void FSDestroyPreferencesPanel(FSPreferencesPanel *preferences) */
/* { */
/*     WMUnmapWidget(preferences->win); */
/*     WMDestroyWidget(preferences->win); */
/*     free((void *) preferences); */
/*     preferences = NULL; */
/* } */

void FSInvokeHelpBrowser(WMWidget *app)
{
    // Display		*dpy = FSAppDisplay(app);
    // WMScreen	*scr = FSAppScreen(app);
}

short FSRunDestroyConfirm(FSViewer *app, char *file)
{
    WMScreen       *scr = FSGetFSViewerScreen(app);
    Display    	   *dpy = FSGetFSViewerDisplay(app);
    WMAlertPanel   *confirm;
    short           result;
    char            msg[MAX_LEN + MAX_LEN + 2];
    
    snprintf(msg, MAX_LEN + MAX_LEN,
	     _("Do you really want to destroy %s?"),
	     file) ;
    if (!(confirm = WMCreateAlertPanel(scr, NULL, "FSViewer", msg,
				       _("Destroy"), _("Cancel"), NULL)))
	{ return -1; }
    
    FSSetFSViewerConfirmWindow(app, WMWidgetXID(confirm->win));
    WMMapWidget(confirm->win);

    WMRunModalLoop(scr, W_VIEW(confirm->win)); 
    /*while (!(confirm->done))
    {
	XEvent event;
	
	WMNextEvent(dpy, &event);
	WMHandleEvent(&event);
    }*/
    result = confirm->result;
    
    WMDestroyAlertPanel(confirm);
    return result;
} 

void 
FSRunIconSelectPanel(FSViewer *app, char *title, char *extn)
{
    if(icon && !(icon->done))
	FSUpdateIconSelectPanel(icon, extn);
    else
    {
	if (!(icon = FSCreateIconSelectPanel(app, title, extn))) 
	    return;
	WMMapWidget(icon->win);
    }

    while (!(icon->done))
    {
	XEvent event;
	
	WMNextEvent(icon->dpy, &event);
	WMHandleEvent(&event);
    }

    FSDestroyIconSelectPanel(icon);
}


static FSIconSelectPanel *
FSCreateIconSelectPanel(FSViewer *app, char *title, char *extn)
{
    FSIconSelectPanel  *icon;

    if (!(icon = (FSIconSelectPanel *) malloc(sizeof(FSIconSelectPanel))))
	return NULL;
    memset((void *) icon, 0, sizeof(FSIconSelectPanel));
    
    icon->app = app;
    icon->scr = FSGetFSViewerScreen(app);
    icon->dpy = WMScreenDisplay(icon->scr);
    
    icon->fileInfo = FSCreateFileInfo();
    icon->fileInfo->extn = extn;
    icon->fileInfo->name = extn;
    icon->fileInfo->imgName = GetFileImgName(extn, NORMAL);

    icon->win = WMCreateWindow(icon->scr, "icon");
    WMResizeWidget(icon->win, 272, 292);
    WMSetWindowTitle(icon->win, _("Icon Selector"));
    WMSetWindowCloseAction(icon->win, FSCloseIconSelectPanel, (void *) icon);
   
    icon->panel = InitIcon(icon->win, icon->app, icon->fileInfo, 0, 10);

    icon->cnt = 0;
    icon->done = 0;
    WMRealizeWidget(icon->win);
    
    FSSetFSViewerTransientWindow(icon->app, WMWidgetXID(icon->win));
    FSShowIconSelectPanel(icon->panel);
    WMMapSubwidgets(icon->win);

    return icon;
}

static void
FSShowIconSelectPanel(Panel *panel)
{
    PanelRec *rec = (PanelRec*)panel;
    
    if (!(rec->callbacks.flags & INITIALIZED_PANEL)) {
	(*rec->callbacks.createWidgets)(panel);
	rec->callbacks.flags |= INITIALIZED_PANEL;
    }

    (*rec->callbacks.updateDisplay)(panel);
    WMMapWidget(rec->frame);
}

static void 
FSCloseIconSelectPanel(WMWidget *self, void *client)
{
    FSIconSelectPanel	*icon = (FSIconSelectPanel *) client;
    
    icon->done = 1;
}

static void 
FSDestroyIconSelectPanel(FSIconSelectPanel *icon)
{
    if(!(icon->cnt))
    {
	WMUnmapWidget(icon->win);
	WMDestroyWidget(icon->win);
	free((void *) icon);
	icon = NULL;
    }  
    else
	icon->cnt--;
}

static void 
FSUpdateIconSelectPanel(FSIconSelectPanel *icon, char *extn)
{
    icon->cnt++;
    icon->fileInfo->name = extn;
    icon->fileInfo->imgName = GetFileImgName(extn, NORMAL);
    FSShowIconSelectPanel(icon->panel);
}

char *
FSRunAppInputPanel(FSViewer *app, FileInfo *fileInfo, char *title)
{
    char *result = NULL;

    if (!(appInput = FSCreateAppInputPanel(app, fileInfo, title))) { return; }
    
    WMMapWidget(appInput->win);

    
    while (!(appInput->done))
    {
	XEvent event;
	
	WMNextEvent(appInput->dpy, &event);
	WMHandleEvent(&event);
    }

    if(appInput->exec)
	result = wstrdup(appInput->exec);

    FSDestroyAppInputPanel(appInput);

    return result;
}

// [Almost verbatim from WMAlertPanel.]
static FSAppInputPanel *
FSCreateAppInputPanel(FSViewer *app, FileInfo *fileInfo, char *title)
{
    FSAppInputPanel *appInput;
    WMPixmap	    *appicon;
    int		     height, width, offset;
    char	     buf[MAX_LEN + 1];
    WMLabel         *l;

    if (!(appInput = (FSAppInputPanel *) malloc(sizeof(FSAppInputPanel))))
	{ return NULL; }
    memset((void *) appInput, 0, sizeof(FSAppInputPanel));
    appInput->app = app;
    appInput->scr = FSGetFSViewerScreen(app);
    appInput->dpy = WMScreenDisplay(appInput->scr);
    appInput->fileInfo = fileInfo;

    height = 272;
    width = 272;
    appInput->win = WMCreateWindow(appInput->scr, "appInput");
    WMResizeWidget(appInput->win, width, height);
    WMSetWindowTitle(appInput->win, title);
    WMSetWindowCloseAction(appInput->win, FSCloseAppInputPanel, 
			   (void *) appInput);
    
    appInput->frame = WMCreateFrame(appInput->win);
    WMMoveWidget(appInput->frame, -1, -1);
    WMResizeWidget(appInput->frame, width+2, height+2);
    WMSetFrameRelief(appInput->frame, WRFlat);
    
    appInput->iconBtn = WMCreateCommandButton(appInput->frame);
    WMMoveWidget(appInput->iconBtn, 16, 16);
    WMResizeWidget(appInput->iconBtn, 68, 68);
    WMSetButtonImagePosition(appInput->iconBtn, WIPImageOnly);
    WMSetButtonAction(appInput->iconBtn, buttonClick, appInput);   
    FSSetButtonImageFromFile(appInput->iconBtn, appInput->fileInfo->imgName);

    l = WMCreateLabel(appInput->frame);
    WMSetLabelWraps(l, 1);
    WMResizeWidget(l, 150, 16*3);
    WMMoveWidget(l, 90, 40);
    WMSetLabelText(l, _("Click to set icon for the selected file type."));
    WMSetLabelRelief(l, WRFlat);
    WMSetLabelTextColor(l, WMDarkGrayColor(WMWidgetScreen(appInput->win)));
    WMSetLabelTextAlignment(l, WALeft);

/*     l = WMCreateLabel(appInput->frame); */ 
/*     WMSetLabelText(l, _("Enter App Name:")); */
/*     WMMoveWidget(l, 15, 110); */
/*     WMResizeWidget(l, 200, 16); */
/*     WMSetLabelRelief(l, WRFlat); */
/*     WMSetLabelTextColor(l, WMDarkGrayColor(WMWidgetScreen(appInput->win))); */

/*     appInput->nameField = WMCreateTextField(appInput->frame); */
/*     WMMoveWidget(appInput->nameField, 15, 130); */
/*     WMResizeWidget(appInput->nameField, 200, 18); */
/*     WMAddNotificationObserver(endedEditingObserver, appInput, */
/*                               WMTextDidEndEditingNotification,  */
/* 			      appInput->nameField); */

/*     appInput->browseBtn = WMCreateCommandButton(appInput->frame); */
/*     WMMoveWidget(appInput->browseBtn, 225, 130); */
/*     WMResizeWidget(appInput->browseBtn, 60, 18); */
/*     WMSetButtonText(appInput->browseBtn, _("Browse...")); */
/*     WMSetButtonEnabled(appInput->browseBtn, True); */
/*     WMSetButtonAction(appInput->browseBtn, buttonClick, appInput);    */
    
    l = WMCreateLabel(appInput->frame);
    WMSetLabelText(l, _("Enter App Executable String:"));
    WMMoveWidget(l, 16, 110);
    WMResizeWidget(l, 240, 16);
    WMSetLabelRelief(l, WRFlat);
    WMSetLabelTextColor(l, WMDarkGrayColor(WMWidgetScreen(appInput->win)));

    appInput->execField = WMCreateTextField(appInput->frame);
    WMMoveWidget(appInput->execField, 16, 130);
    WMResizeWidget(appInput->execField, 240, 18);

    appInput->viewBtn = WMCreateSwitchButton(appInput->frame);
    WMMoveWidget(appInput->viewBtn, 16, 160);
    WMResizeWidget(appInput->viewBtn, 240, 16);
    WMSetButtonEnabled(appInput->viewBtn, True);
    WMSetButtonText(appInput->viewBtn, _("Set As Default Viewer"));

    appInput->edBtn = WMCreateSwitchButton(appInput->frame);
    WMMoveWidget(appInput->edBtn, 16, 190);
    WMResizeWidget(appInput->edBtn, 240, 16);
    WMSetButtonEnabled(appInput->edBtn, True);
    WMSetButtonText(appInput->edBtn, _("Set As Default Editor"));

    appInput->cancelBtn = WMCreateCommandButton(appInput->frame);
    WMMoveWidget(appInput->cancelBtn, 16, height-35);
    WMResizeWidget(appInput->cancelBtn, 115, 24);
    WMSetButtonText(appInput->cancelBtn, _("Cancel"));
    WMSetButtonEnabled(appInput->cancelBtn, True);
    WMSetButtonAction(appInput->cancelBtn, buttonClick, appInput);   
  
    appInput->okBtn = WMCreateCommandButton(appInput->frame);
    WMMoveWidget(appInput->okBtn, 141, height-35);
    WMResizeWidget(appInput->okBtn, 115, 24);
    WMSetButtonText(appInput->okBtn, _("OK"));
    WMSetButtonImage(appInput->okBtn, 
		     WMGetSystemPixmap(WMWidgetScreen(appInput->win), 
				       WSIReturnArrow));
    WMSetButtonAltImage(appInput->okBtn, 
			WMGetSystemPixmap(WMWidgetScreen(appInput->win), 
					  WSIHighlightedReturnArrow));
    WMSetButtonImagePosition(appInput->okBtn, WIPRight);
    WMSetButtonEnabled(appInput->okBtn, True);
    WMSetButtonAction(appInput->okBtn, buttonClick, appInput);   

    WMMapSubwidgets(appInput->frame);
        
    appInput->done = 0;

    WMRealizeWidget(appInput->win);
/*     WMChangePanelOwner(appInput->win, ); */
    FSAddWindow(appInput->app, WMWidgetXID(appInput->win));
/*     FSSetFSViewerTransientWindow(appInput->app, WMWidgetXID(appInput->win)); */
    WMMapSubwidgets(appInput->win);

    return appInput;
}

static void FSCloseAppInputPanel(WMWidget *self, void *client)
{
    FSAppInputPanel	*appInput = (FSAppInputPanel *) client;
    
    appInput->done = 1;
}

static void FSDestroyAppInputPanel(FSAppInputPanel *appInput)
{
    WMUnmapWidget(appInput->win);
    WMDestroyWidget(appInput->win);
    free((void *) appInput);
    appInput = NULL;
}

static void
buttonClick(WMWidget *self, void *data)
{
    FSAppInputPanel *appInput = (FSAppInputPanel *)data;

    if ((WMButton *)self == appInput->okBtn) 
    {
	if( FSStoreAppInputData(appInput) )
	    FSCloseAppInputPanel(self, appInput);
    }
    else if ((WMButton *)self == appInput->cancelBtn)
    {
	FSCloseAppInputPanel(self, appInput);	
    }
    else if ((WMButton *)self == appInput->iconBtn)
    {
	char *tmp = NULL;
	/*
	  Could lead to a core dump if the button is
	  pressed again.
	*/
/* 	FileInfo *tmp = FSCreateFileInfo(); */

/* 	FSRunIconSelectPanel(appInput->app, "",  */
/* 			     GetFileExtn(appInput->fileInfo->name)); */
	
/* 	GetFileInfo(appInput->fileInfo->path, appInput->fileInfo->name, tmp); */

	tmp = FSRunSelectIconPanel(appInput->win, _("Icon Select"),
				   GetFileExtn(appInput->fileInfo->name));
	if(tmp == NULL)
	    return;
	
	if(appInput->fileInfo->imgName)
	{
	    free(appInput->fileInfo->imgName);
	    appInput->fileInfo->imgName = NULL;
	}
	
	if(tmp)
	{
	    appInput->fileInfo->imgName = LocateImage(tmp);
	    free(tmp);
	}
	FSSetButtonImageFromFile(appInput->iconBtn,
				 appInput->fileInfo->imgName);
    }
/*     else if ((WMButton *)self == appInput->browseBtn) */
/*     { */
/* 	WMOpenPanel *panel; */
/* 	char *initial = "/usr/local/bin"; */

/* 	panel = WMGetOpenPanel(appInput->scr); */
	
	/*WMSetFilePanelAutoCompletion(panel, False);*/
	
	/* The 3rd argument for this function is the initial name of the 
	   file, not the name of the window, although it's not implemented 
	   yet  
	*/ 
/* 	if (WMRunModalOpenPanelForDirectory(panel, appInput->win, initial,  */
					    /*title*/ /* NULL, NULL) == True) */
/* 	    printf("%s\n", WMGetFilePanelFileName(panel)); */
/*     } */
}

static char *
FSStoreAppInputData(FSAppInputPanel *appInput)
{
    Bool extnAdded = False;
    char *exec = NULL;

    exec = WMGetTextFieldText(appInput->execField);

    if(!strcmp(exec, ""))
	return NULL;

    /* Set the default viewer of the selected file */
    if( WMGetButtonSelected(appInput->viewBtn) )
    {
	FSSetStringForNameKey(appInput->fileInfo->extn, "viewer", exec);
	extnAdded = True;
    }

    /* Set the default editor of the selected file */
    if( WMGetButtonSelected(appInput->edBtn) )
    {
	FSSetStringForNameKey(appInput->fileInfo->extn, "editor", exec);
	extnAdded = True;
    }
    
    if(extnAdded)
    {
	WMPropList* array = NULL;

	array = FSGetUDObjectForKey(defaultsDB, "EXTN");
	if(array && WMIsPLArray(array))
	    InsertArrayElement(array, WMCreatePLString(appInput->fileInfo->extn));
    }

    if(appInput->exec)
	free(appInput->exec);

    if(appInput->fileInfo->imgName)
	FSSetStringForNameKey(appInput->fileInfo->extn, "icon", 
			   appInput->fileInfo->imgName);

    appInput->exec = exec;
    
    return exec;
}

/* static void */
/* FSStoreAppInputPropListData(FSAppInputPanel *appInput) */
/* { */
/*     int  len; */
/*     char *extn; */
/*     char *txt; */
/*     WMPropList* dictKey = NULL; */
/*     WMPropList* val     = NULL; */
/*     WMPropList* dict    = NULL; */
/*     WMPropList* key     = NULL; */

    /* Get the contents of the textfileds */
/*     txt = WMGetTextFieldText(appInput->execField); */
/*     extn = WMGetTextFieldText(appInput->nameField); */

    /* Update the exec field of the chosen app */
/*     dictKey = WMCreatePLString(extn); */
/*     key = WMCreatePLString("exec"); */
/*     val = WMCreatePLString(txt); */

/*     dict = PLGetDictionaryEntry(filesDB, dictKey); */
/*     if (dict && PLIsDictionary(dict)) */
/*     { */
/* 	WMPropList* tmp = NULL; */
	
	/* 
	   Check to make sure the exec field of the app 
	   doesn't exist. We don't want to overwirte it 
	   here. This might change in the future. 
	*/ 
/* 	tmp = PLGetDictionaryEntry(dict, key); */
/* 	if(!tmp) */
/* 	    PLInsertDictionaryEntry(dict, key, val); */
/*     } */
/*     else */
/*     { */
	/*
	  The app doesn't exist at all so we can create
	  a new entry for it in the dictionary 
	*/ 
/* 	dict = PLMakeDictionaryFromEntries(key, val, NULL);	 */
/* 	PLInsertDictionaryEntry(filesDB, dictKey, dict); */
/*     } */

    /* Add the app to the EXE array if necessary */
/*     if(key) */
/* 	PLRelease(key); */
/*     key = WMCreatePLString("EXE"); */
/*     dict = PLGetDictionaryEntry(filesDB, key); */
/*     if (!dict) */
/*     { */
/* 	dict = PLMakeArrayFromElements(NULL, NULL);	 */
/* 	PLInsertDictionaryEntry(filesDB, key, dict); */
/*     }	 */
/*     InsertArrayElement(dict, dictKey); */

/*     if(key) */
/* 	PLRelease(key); */
/*     if(val) */
/* 	PLRelease(val); */
     
    /* Set the default viewer for the selected file */
/*     if( WMGetButtonSelected(appInput->viewBtn) ) */
/*     { */
/* 	dictKey = WMCreatePLString(GetFileExtn(appInput->fileInfo->name)); */
/* 	key = WMCreatePLString("viewer"); */
/* 	val = WMCreatePLString(extn); */
	
/* 	dict = PLGetDictionaryEntry(filesDB, dictKey); */
/* 	if (!dict) */
/* 	{ */
/* 	    dict = PLMakeDictionaryFromEntries(NULL, NULL, NULL);	 */
/* 	    PLInsertDictionaryEntry(filesDB, dictKey, dict); */
/* 	} */
/* 	PLInsertDictionaryEntry(dict, key, val); */

/* 	if(key) */
/* 	    PLRelease(key); */
/* 	if(val) */
/* 	    PLRelease(val); */
/* 	if(dictKey) */
/* 	    PLRelease(dictKey); */
/*     } */

    /* Set the default editor of the selected file */
/*     if( WMGetButtonSelected(appInput->edBtn) ) */
/*     { */
/* 	dictKey = WMCreatePLString(GetFileExtn(appInput->fileInfo->name)); */
/* 	key = WMCreatePLString("editor"); */
/* 	val = WMCreatePLString(extn); */
	    
/* 	dict = PLGetDictionaryEntry(filesDB, dictKey); */
/* 	if (!dict) */
/* 	{ */
/* 	    dict = PLMakeDictionaryFromEntries(NULL, NULL, NULL);	 */
/* 	    PLInsertDictionaryEntry(filesDB, dictKey, dict); */
/* 	} */
/* 	PLInsertDictionaryEntry(dict, key, val); */

/* 	if(key) */
/* 	    PLRelease(key); */
/* 	if(val) */
/* 	    PLRelease(val); */
/* 	if(dictKey) */
/* 	    PLRelease(dictKey); */
/*     } */

/*      PLSave(filesDB, wdefaultspathfordomain("FSViewer"), YES); */

/*     if(extn) */
/* 	free(extn); */
/*     if(txt) */
/* 	free(txt); */
/* } */

static void
endedEditingObserver(void *observerData, WMNotification *notification)
{
    FSAppInputPanel *appInput = (FSAppInputPanel *)observerData;
    WMTextField *tPtr = (WMTextField *) WMGetNotificationObject(notification);

    if((int)WMGetNotificationClientData(notification) == WMReturnTextMovement
       && tPtr == appInput->nameField) 
    {
	char *txt = WMGetTextFieldText(appInput->nameField);
        if(strcmp("", txt))
	{
	    char *execStr = NULL;

	    execStr = GetExecStringForExtn(GetFileExtn(txt));
	    if(execStr)
	    {
		WMSetTextFieldText(appInput->execField, execStr);
		free(execStr);
		execStr = NULL;
	    }
	    else
	    {
		WMSetTextFieldText(appInput->execField, txt);
		WMInsertTextFieldText(appInput->execField, " %s", -1);
	    }
	    free(txt);
	}
    }
}


static void
alertPanelOnClick(WMWidget *self, void *clientData)
{
    FSAlertPanel *alertPanel = clientData;

    alertPanel->done = 1;
    if (self == alertPanel->defBtn) {
        alertPanel->result = WAPRDefault;
    } else if (self == alertPanel->othBtn) {
        alertPanel->result = WAPROther;
    } else if (self == alertPanel->altBtn) {
        alertPanel->result = WAPRAlternate;
    }
}



static void
handleKeyPress(XEvent *event, void *clientData)
{
    FSAlertPanel *alertPanel = (FSAlertPanel*)clientData;

    if (event->xkey.keycode == alertPanel->retKey) {
        WMPerformButtonClick(alertPanel->defBtn);
    }
}


int
FSRunAlertPanel(FSViewer *app,
                char *title, char *msg, char *defaultButton,
                char *alternateButton, char *otherButton)
{
    int tmp;

    if( !(alertPanel = FSCreateAlertPanel(app, title, msg, 
					  defaultButton, alternateButton, 
					  otherButton) ) ) {return;}

    WMMapWidget(alertPanel->win);

    while (!alertPanel->done) {
        XEvent event;

        WMNextEvent(alertPanel->dpy, &event);
        WMHandleEvent(&event);
    }

    tmp = alertPanel->result;
    FSDestroyAlertPanel(alertPanel);

    return tmp;
}


static void
FSDestroyAlertPanel(FSAlertPanel *alertPanel)
{
    WMUnmapWidget(alertPanel->win);
    WMDestroyWidget(alertPanel->win);
    free(alertPanel);
}


static FSAlertPanel*
FSCreateAlertPanel(FSViewer *app,
                   char *title, char *msg, char *defaultButton,
                   char *alternateButton, char *otherButton)
{
    WMPixmap *appIcon;
    WMFont *normalFont = NULL;
    WMSize pixmapSize;
    FSAlertPanel *alertPanel;
    int x, dw=0, aw=0, ow=0, w;

    alertPanel = wmalloc(sizeof(FSAlertPanel));
    memset(alertPanel, 0, sizeof(FSAlertPanel));

    alertPanel->app = app;
    alertPanel->scr = FSGetFSViewerScreen(app);
    alertPanel->dpy = WMScreenDisplay(alertPanel->scr);
    
    alertPanel->retKey = XKeysymToKeycode(alertPanel->dpy, XK_Return);

    alertPanel->win = WMCreateWindowWithStyle(alertPanel->scr, "alertPanel",
					      WMTitledWindowMask);

    WMSetWindowTitle(alertPanel->win, title);

    normalFont = WMSystemFontOfSize(alertPanel->scr, 10);

    appIcon = WMGetApplicationIconPixmap(alertPanel->scr);
    pixmapSize = WMGetPixmapSize(appIcon);
    if (appIcon) 
    {
        alertPanel->iLbl = WMCreateLabel(alertPanel->win);
        WMResizeWidget(alertPanel->iLbl, pixmapSize.width, pixmapSize.height);
        WMMoveWidget(alertPanel->iLbl, 8 + (64 - pixmapSize.width)/2,
                     (75 - pixmapSize.height)/2);
        WMSetLabelImage(alertPanel->iLbl, appIcon);
        WMSetLabelImagePosition(alertPanel->iLbl, WIPImageOnly);
	
	 WMReleasePixmap(appIcon);
    }


    if (title) 
    {
        WMFont *largeFont;
	
        largeFont = WMBoldSystemFontOfSize(alertPanel->scr, 24);

        alertPanel->tLbl = WMCreateLabel(alertPanel->win);
        WMMoveWidget(alertPanel->tLbl, 80, (80 - WMFontHeight(largeFont))/2);
        WMResizeWidget(alertPanel->tLbl, 400 - 70, WMFontHeight(largeFont)+4);
        WMSetLabelText(alertPanel->tLbl, title);
        WMSetLabelTextAlignment(alertPanel->tLbl, WALeft);
        WMSetLabelFont(alertPanel->tLbl, largeFont);

        WMReleaseFont(largeFont);
    }


    if (msg) 
    {
        alertPanel->mLbl = WMCreateLabel(alertPanel->win);
        WMMoveWidget(alertPanel->mLbl, 10, 83);
        WMResizeWidget(alertPanel->mLbl, 380, 
		       WMFontHeight(normalFont)*4);
        WMSetLabelText(alertPanel->mLbl, msg);
	
	WMSetLabelTextAlignment(alertPanel->mLbl, WACenter);
    }

    /* create divider line */
    alertPanel->line = WMCreateFrame(alertPanel->win);
    WMMoveWidget(alertPanel->line, 0, 80);
    WMResizeWidget(alertPanel->line, 400, 2);
    WMSetFrameRelief(alertPanel->line, WRGroove);

    /* create buttons */
    if (otherButton)
        ow = WMWidthOfString(normalFont, otherButton,
                             strlen(otherButton));

    if (alternateButton)
        aw = WMWidthOfString(normalFont, alternateButton,
                             strlen(alternateButton));

    if (defaultButton)
       dw = WMWidthOfString(normalFont, defaultButton,
                             strlen(defaultButton));

    pixmapSize = WMGetPixmapSize(WMGetSystemPixmap(appInput->scr, 
						   WSIReturnArrow));
    w = dw + pixmapSize.width;
    if (aw > w)
        w = aw;
    if (ow > w)
        w = ow;

    w += 30;
    x = 400;

    if (defaultButton) 
    {
        x -= w + 10;

        alertPanel->defBtn = WMCreateCommandButton(alertPanel->win);
        WMSetButtonAction(alertPanel->defBtn, alertPanelOnClick, alertPanel);
        WMMoveWidget(alertPanel->defBtn, x, 144);
        WMResizeWidget(alertPanel->defBtn, w, 24);
        WMSetButtonText(alertPanel->defBtn, defaultButton);
        WMSetButtonImage(alertPanel->defBtn, 
			 WMGetSystemPixmap(appInput->scr, WSIReturnArrow));

	WMSetButtonAltImage(alertPanel->defBtn,
			    WMGetSystemPixmap(appInput->scr, 
					      WSIHighlightedReturnArrow));
        WMSetButtonImagePosition(alertPanel->defBtn, WIPRight);
    }

    if (alternateButton) 
    {
        x -= w + 10;
	
        alertPanel->altBtn = WMCreateCommandButton(alertPanel->win);
        WMMoveWidget(alertPanel->altBtn, x, 144);
        WMResizeWidget(alertPanel->altBtn, w, 24);
        WMSetButtonAction(alertPanel->altBtn, alertPanelOnClick, alertPanel);
        WMSetButtonText(alertPanel->altBtn, alternateButton);
    }

    if (otherButton) 
    {
        x -= w + 10;

        alertPanel->othBtn = WMCreateCommandButton(alertPanel->win);
        WMSetButtonAction(alertPanel->othBtn, alertPanelOnClick, alertPanel);
        WMMoveWidget(alertPanel->othBtn, x, 144);
        WMResizeWidget(alertPanel->othBtn, w, 24);
        WMSetButtonText(alertPanel->othBtn, otherButton);
    }


    alertPanel->done = 0;

    WMCreateEventHandler(W_VIEW(alertPanel->win), KeyPressMask,
                         handleKeyPress, alertPanel);

    WMRealizeWidget(alertPanel->win);
    FSSetFSViewerTransientWindow(alertPanel->app, 
				 WMWidgetXID(alertPanel->win));
    WMMapSubwidgets(alertPanel->win);

    return alertPanel;
}

char *
FSRunSelectIconPanel(WMWindow *owner, char *title, char *str)
{ 
    char *imgStr = NULL;

    if(selIcon)
	return;

    if (!(selIcon = FSCreateSelectIconPanel(owner, title, str))) 
	return;
    
    WMMapWidget(selIcon->win);
    
    while (!(selIcon->done))
    {
	XEvent event;
	
	WMNextEvent(selIcon->dpy, &event);
	WMHandleEvent(&event);
    }

    imgStr = FSDestroySelectIconPanel(selIcon);
    selIcon = NULL;

    return imgStr;
}

static FSSelectIconPanel*
FSCreateSelectIconPanel(WMWindow *owner, char *title, char *str)
{
    FSSelectIconPanel	  *selIcon;
    int		           height, width, offset;
    WMFrame               *f;
    WMLabel               *l;
    RColor color;
    WMPixmap *pixmap;
    char *txt = NULL;
    WMFont *aFont;

    if( !(selIcon = (FSSelectIconPanel *) wmalloc(sizeof(FSSelectIconPanel))))
	return NULL;
    memset((void *) selIcon, 0, sizeof(FSSelectIconPanel));

/*     selIcon->app = app; */
    selIcon->scr = WMWidgetScreen(owner);
    selIcon->dpy = WMScreenDisplay(selIcon->scr);

    txt = FSGetStringForName("ICONDIR");
    if(!txt) 
	txt = ICONDIR;
	
    selIcon->xpmDir = (char *) wmalloc(strlen(txt)+5);
    strcpy(selIcon->xpmDir, txt);
    strcat(selIcon->xpmDir, "/xpm");
    
    selIcon->tiffDir = (char *) wmalloc(strlen(txt)+6);
    strcpy(selIcon->tiffDir, txt);
    strcat(selIcon->tiffDir, "/tiff");
    
    if(txt != ICONDIR) 
	free(txt);

    height = 422;
    width  = 272;
    selIcon->win = WMCreateWindowWithStyle(selIcon->scr, "selIcon", 
					   WMTitledWindowMask |
					   WMClosableWindowMask |
					   WMBorderlessWindowMask);
    WMResizeWidget(selIcon->win, width, height);
    WMSetWindowTitle(selIcon->win, title);
    WMSetWindowCloseAction(selIcon->win, FSCloseSelectIconPanel, 
			   (void *) selIcon);
    
    selIcon->frame = WMCreateFrame(selIcon->win);
    WMResizeWidget(selIcon->frame, 282, 95);
    WMMoveWidget(selIcon->frame, -5, -5);
    WMSetFrameRelief(selIcon->frame, WRFlat);

    l = WMCreateLabel(selIcon->frame);
    WMResizeWidget(l, width-10, 75);
    WMMoveWidget(l, 10, 10);
    color.red   = 0xae;
    color.green = 0xaa;
    color.blue  = 0xae;
    color.alpha = 0;
           /* FS.. */   
    pixmap = WMCreateBlendedPixmapFromFile(selIcon->scr, LocateImage(str), 
					   &color);
    WMSetLabelImage(l, pixmap);
    WMSetLabelText(l, str); 
    WMSetLabelImagePosition(l, WIPLeft);

    f = WMCreateFrame(selIcon->win);
    WMMoveWidget(f, -5, 90);
    WMResizeWidget(f, width+10, height - 95 - 40);
    WMSetFrameRelief(f, WRGroove);

    aFont = WMBoldSystemFontOfSize(selIcon->scr, 12);
    l = WMCreateLabel(f);
    WMMoveWidget(l, 10, 10);
    WMResizeWidget(l, 252, 20);
    WMSetLabelText(l, _("Directories"));
    WMSetLabelRelief(l, WRSunken);
    WMSetWidgetBackgroundColor(l, WMDarkGrayColor(selIcon->scr));
    WMSetLabelTextColor(l, WMWhiteColor(selIcon->scr));
    WMSetLabelFont(l, aFont);
    WMSetLabelTextAlignment(l, WACenter);

    selIcon->pathList = WMCreateList(f);
    WMMoveWidget(selIcon->pathList, 10, 31);
    WMResizeWidget(selIcon->pathList, 252, 100);
    WMSetListAction(selIcon->pathList, fillIconFileList, selIcon);
    FSLoadIconPaths(selIcon->pathList);
    WMAddListItem(selIcon->pathList, selIcon->xpmDir);
    WMAddListItem(selIcon->pathList, selIcon->tiffDir);
   
    l = WMCreateLabel(f);
    WMMoveWidget(l, 10, 140);
    WMResizeWidget(l, 252, 20);
    WMSetLabelText(l, _("Files"));
    WMSetLabelRelief(l, WRSunken);
    WMSetWidgetBackgroundColor(l, WMDarkGrayColor(selIcon->scr));
    WMSetLabelTextColor(l, WMWhiteColor(selIcon->scr));
    WMSetLabelFont(l, aFont);
    WMSetLabelTextAlignment(l, WACenter);

    selIcon->fileList = WMCreateList(f);
    WMMoveWidget(selIcon->fileList, 10, 161);
    WMResizeWidget(selIcon->fileList, 181, 100);
    WMSetListAction(selIcon->fileList, setIconLabel, selIcon);

    selIcon->iconLabel = WMCreateLabel(f);
    WMMoveWidget(selIcon->iconLabel, 192, 161);
    WMResizeWidget(selIcon->iconLabel, 70, 100);
    WMSetLabelImagePosition(selIcon->iconLabel, WIPImageOnly);
    WMSetLabelRelief(selIcon->iconLabel, WRSunken);

    selIcon->cancelBtn = WMCreateCommandButton(selIcon->win);
    WMMoveWidget(selIcon->cancelBtn, 16, height-35);
    WMResizeWidget(selIcon->cancelBtn, 115, 24);
    WMSetButtonText(selIcon->cancelBtn, _("Cancel"));
    WMSetButtonEnabled(selIcon->cancelBtn, True);
    WMSetButtonAction(selIcon->cancelBtn, selIconButtonClick, selIcon);   
  
    selIcon->okBtn = WMCreateCommandButton(selIcon->win);
    WMMoveWidget(selIcon->okBtn, 141, height-35);
    WMResizeWidget(selIcon->okBtn, 115, 24);
    WMSetButtonText(selIcon->okBtn, _("OK"));
    WMSetButtonEnabled(selIcon->okBtn, True);
    WMSetButtonAction(selIcon->okBtn, selIconButtonClick, selIcon);   

    selIcon->done = 0;
    WMRealizeWidget(selIcon->win);
    
/*     FSSetFSViewerTransientWindow(selIcon->app, WMWidgetXID(selIcon->win)); */
    WMMapSubwidgets(f);
    WMMapSubwidgets(selIcon->frame);
    WMMapSubwidgets(selIcon->win);
    WMReleaseFont(aFont);

    WMChangePanelOwner(selIcon->win, owner);
    return selIcon;
}

static void 
FSCloseSelectIconPanel(WMWidget *self, void *client)
{
    FSSelectIconPanel	*selIcon = (FSSelectIconPanel *) client;
    
    selIcon->done = 1;
}

static char *
FSDestroySelectIconPanel(FSSelectIconPanel *selIcon)
{
    char *imgStr = NULL;

    WMCloseWindow(selIcon->win);
    WMUnmapWidget(selIcon->win);
    WMDestroyWidget(selIcon->win);
    if(selIcon->iconName)
	imgStr = wstrdup(selIcon->iconName);
    free((void *) selIcon);

    selIcon = NULL;

    return imgStr;
}

static void
selIconButtonClick(WMWidget *self, void *data)
{
    FSSelectIconPanel *selIcon = (FSSelectIconPanel *)data;

    if( (WMButton *)self == selIcon->okBtn )
    {
	FSCloseSelectIconPanel(self, selIcon);
    }
    else if( (WMButton *)self == selIcon->cancelBtn )
    {
	if(selIcon->iconName)
	    free(selIcon->iconName);
	selIcon->iconName = NULL;
	FSCloseSelectIconPanel(self, selIcon);
    }
}    

static void
setIconLabel(WMWidget *self, void *data)
{
    RColor    color;
    WMPixmap *pixmap;
    FSSelectIconPanel *panel = (FSSelectIconPanel *) data;
    
    color.red = 0xae;
    color.green = 0xaa;
    color.blue = 0xae;
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
    FSSelectIconPanel *panel = (FSSelectIconPanel *) data;
    char *pathname = NULL;
    WMListItem *listItem;
    FileInfo   *fileList;

    WMClearList(panel->fileList);
    WMSetLabelImage(panel->iconLabel, NULL);

    listItem = WMGetListSelectedItem(panel->pathList);
    pathname = (char *) wmalloc(strlen(listItem->text)+2);
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

}

char *
getSelectedFilename(FSSelectIconPanel *panel)
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
	
	color.red = 0xae;
	color.green = 0xaa;
	color.blue = 0xae;
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
	      /*  FS.. */
 	pixmap = WMCreateBlendedPixmapFromFile(WMWidgetScreen(panel->win),
					       imgName, &color);

	WMSetLabelImage(panel->iconLabel, pixmap);
	WMReleasePixmap(pixmap);
    }
    
    if(imgName)
	free(imgName);

    return filename;
}


